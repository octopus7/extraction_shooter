#include "AI/Hopper/HopperEnemyCharacter.h"
#include "Combat/TunaSweeperCombatValue.h"
#include "AI/Hopper/HopperArmModule.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "AI/TunaSweeperEnemyAIController.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Weapon/TunaSweeperProjectile.h"

DEFINE_LOG_CATEGORY_STATIC(LogHopperCombat, Log, All);

ATunaSweeperHopperEnemyCharacter::ATunaSweeperHopperEnemyCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(65.f,105.f);
    // The Hopper must never carve navigation around its own movement capsule.
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);
    Presentation = CreateDefaultSubobject<UHopperPresentationComponent>(TEXT("HopperPresentation"));
    VisualData = TSoftObjectPtr<UHopperVisualData>(FSoftObjectPath(TEXT("/Game/Characters/Hopper/DA_HopperVisual.DA_HopperVisual")));
    MechDestructionEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Effects/ExplosionTuna/NS_Explosion_SmokeRobot.NS_Explosion_SmokeRobot")));
    LeftArmClass = RightArmClass = AHopperArmModule::StaticClass();
    MaxHealth = 30.f;
    MovementSpeedRandomOffset = FVector2D::ZeroVector;
    FTunaSweeperEnemyCombatProfile Profile;
    Profile.ProfileId = TEXT("hopper");
    Profile.AttackMode = ETunaSweeperEnemyAttackMode::Melee;
    Profile.MovementSpeed = MechMoveSpeed;
    Profile.TurnSpeedDegreesPerSecond = 35.f;
    Profile.AttackFacingToleranceDegrees = 10.f;
    Profile.MeleeAttackDamage = 8.f;
    Profile.ShotIntervalSecondsMin = .3f;
    Profile.ShotIntervalSecondsMax = .4f;
    ConfigureCombatProfile(Profile, TunaSweeperFactionIds::Enemy, NAME_None, INDEX_NONE);
}

void ATunaSweeperHopperEnemyCharacter::InitializePresentation()
{
    LoadedVisualData = VisualData.LoadSynchronous();
    Presentation->Initialize(LoadedVisualData);
    VisualMesh->SetVisibility(false, true);
    ForwardMarkerMesh->SetVisibility(false, true);
}

void ATunaSweeperHopperEnemyCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);
    if (!HasActorBegunPlay()) InitializePresentation();
}

void ATunaSweeperHopperEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    // Correct cached-profile speed resets after AI callbacks; phase transitions also update speed immediately.
    if (NewController) AddTickPrerequisiteActor(NewController);
    ApplyPhaseMovementSpeed();
}

void ATunaSweeperHopperEnemyCharacter::BeginPlay()
{
    // The base class must never initialize its renewable weapon magazine for this enemy.
    bInitializingBase = true;
    Super::BeginPlay();
    bInitializingBase = false;
    InitializePresentation();
    PreviousGaitYaw = GetActorRotation().Yaw;
    MechMaxHealth = FMath::Max(1.f, TunaSweeperCombatValue::Round(MechMaxHealth));
    MechHealth = MechMaxHealth;
    PilotAmmo = FMath::Max(0, PilotAmmoCapacity);
    FTunaSweeperEnemyCombatProfile Profile = GetCombatProfile();
    Profile.AttackMode = ETunaSweeperEnemyAttackMode::Ranged;
    Profile.MovementSpeed = MechMoveSpeed;
    Profile.TurnSpeedDegreesPerSecond = 35.f;
    Profile.MeleeAttackDamage = 8.f;
    ConfigureCombatProfile(Profile, FactionComponent->GetFactionId(), FactionComponent->GetSquadId(), FactionComponent->GetSquadSlot());
    SetArmModule(true, LeftArmClass);
    SetArmModule(false, RightArmClass);
    PhaseSeconds = 0;
    Presentation->SetGunVisible(false);
    GetCharacterMovement()->MaxAcceleration = 100.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 140.f;
    if (bStartMounted || BoardingSeconds <= 0)
    {
        Phase = EHopperCombatPhase::Mech;
        Presentation->SeatPilot();
        Presentation->SetMechActive(true);
    }
    else
    {
        Phase = EHopperCombatPhase::Boarding;
        GetCharacterMovement()->DisableMovement();
        Presentation->SetMechActive(false);
        Presentation->StartBoarding(BoardingSeconds);
    }
    ApplyPhaseMovementSpeed();
}

void ATunaSweeperHopperEnemyCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsDead()) return;
    const float AngularRate = DeltaSeconds > UE_SMALL_NUMBER ? FMath::Abs(FMath::FindDeltaAngleDegrees(PreviousGaitYaw,GetActorRotation().Yaw))/DeltaSeconds : 0.f;
    TurningFootSpeed = FMath::DegreesToRadians(AngularRate)*45.f;
    PreviousGaitYaw = GetActorRotation().Yaw;
    if (Phase == EHopperCombatPhase::Boarding)
    {
        PhaseSeconds += DeltaSeconds;
        if (PhaseSeconds >= FMath::Max(0.f, BoardingSeconds))
        {
            Phase = EHopperCombatPhase::Mech;
            ApplyPhaseMovementSpeed();
            Presentation->SeatPilot();
            Presentation->SetMechActive(true);
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
    else if (Phase == EHopperCombatPhase::Disembarking) UpdateDisembark(DeltaSeconds);
    else if (Phase == EHopperCombatPhase::PilotRanged || Phase == EHopperCombatPhase::PilotMelee)
    {
        UpdatePilotAmmoPhase();
        Presentation->SetPilotLocomotion(GetVelocity().SizeSquared2D() > FMath::Square(8.f));
    }
    ApplyPhaseMovementSpeed();
}

void ATunaSweeperHopperEnemyCharacter::ApplyPhaseMovementSpeed()
{
    float Speed = 0.f;
    if (!IsDead() && Phase == EHopperCombatPhase::Mech)
        Speed = FMath::Min(MechMoveSpeed, FMath::Max(0.f,Presentation->GetMaxStableWalkSpeed()-TurningFootSpeed));
    else if (!IsDead() && (Phase == EHopperCombatPhase::PilotRanged || Phase == EHopperCombatPhase::PilotMelee))
        Speed = PilotMoveSpeed;
    GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, Speed);
}

bool ATunaSweeperHopperEnemyCharacter::UsesMeleeAttack() const
{
    return bInitializingBase || Phase == EHopperCombatPhase::PilotMelee;
}

bool ATunaSweeperHopperEnemyCharacter::IsStandardCombatSuppressed() const
{
    return IsDead() || Phase == EHopperCombatPhase::Boarding || Phase == EHopperCombatPhase::Disembarking
        || Super::IsStandardCombatSuppressed();
}

float ATunaSweeperHopperEnemyCharacter::TakeDamage(float Amount, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer)
{
    if (IsDead() || !FMath::IsFinite(Amount) || Amount <= 0.f) return 0;
    if (Phase == EHopperCombatPhase::PilotRanged || Phase == EHopperCombatPhase::PilotMelee)
        return Super::TakeDamage(Amount, Event, InstigatorController, Causer);
    // A destroyed chassis owns transition hits. They cannot damage the seated pilot or trigger loot.
    if (Phase == EHopperCombatPhase::Disembarking || MechHealth <= 0) return 0;
    AActor* Source = Causer ? Causer : (InstigatorController ? InstigatorController->GetPawn() : nullptr);
    if (UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
        Factions && !Factions->CanApplyCombatEffect(Source, this)) return 0;
    const float Applied = FMath::Min(MechHealth, TunaSweeperCombatValue::Round(Amount));
    if (Applied <= 0.f) return 0.f;
    MechHealth = FMath::Max(0.f, MechHealth - Applied);
    if (ATunaSweeperEnemyAIController* AI = Cast<ATunaSweeperEnemyAIController>(GetController()))
    {
        AActor* Suspect = InstigatorController ? InstigatorController->GetPawn() : (Causer && Causer->GetOwner() ? Causer->GetOwner() : Causer);
        if (Suspect) AI->NotifyDamageTaken(Suspect);
    }
    if (MechHealth <= 0) BeginMechDestruction();
    return Applied;
}

bool ATunaSweeperHopperEnemyCharacter::CanAttackHopperTarget(AActor* Target) const
{
    if (!GetWorld() || !IsValid(Target) || Target == this || IsStandardCombatSuppressed()) return false;
    if (const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
        Factions && !Factions->CanTargetActor(this, Target)) return false;
    const FVector Delta = Target->GetActorLocation() - GetActorLocation();
    if (Delta.IsNearlyZero()) return false;
    const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, Delta.Rotation().Yaw));
    if (YawError > GetCombatProfile().AttackFacingToleranceDegrees) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HopperTargetSight), true, this);
    if (LeftArm) Query.AddIgnoredActor(LeftArm);
    if (RightArm) Query.AddIgnoredActor(RightArm);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Query)
        || Hit.GetActor() == Target;
}

ETunaSweeperEnemyFireResult ATunaSweeperHopperEnemyCharacter::TryFireProjectileAt(AActor* Target)
{
    if (IsStandardCombatSuppressed()) return ETunaSweeperEnemyFireResult::Blocked;
    if (Phase == EHopperCombatPhase::PilotMelee) return ETunaSweeperEnemyFireResult::OutOfAmmo;
    if (GetWorld() && IsValid(Target))
        if (const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
            Factions && Factions->AreActorsFriendly(this, Target)) return ETunaSweeperEnemyFireResult::FriendlyTarget;
    if (!CanAttackHopperTarget(Target)) return ETunaSweeperEnemyFireResult::Blocked;
    if (Phase == EHopperCombatPhase::Mech)
    {
        AHopperArmModule* First = GetArmModule(bNextArmLeft);
        AHopperArmModule* Second = GetArmModule(!bNextArmLeft);
        if (!IsValid(First) && !IsValid(Second)) return ETunaSweeperEnemyFireResult::OutOfAmmo;
        if ((IsValid(First) && First->TryFireAt(Target)) || (IsValid(Second) && Second->TryFireAt(Target)))
        {
            bNextArmLeft = !bNextArmLeft;
            return ETunaSweeperEnemyFireResult::Fired;
        }
        return ETunaSweeperEnemyFireResult::Cooldown;
    }
    if (Phase != EHopperCombatPhase::PilotRanged) return ETunaSweeperEnemyFireResult::Blocked;
    if (PilotAmmo <= 0)
    {
        UpdatePilotAmmoPhase();
        return ETunaSweeperEnemyFireResult::OutOfAmmo;
    }
    if (GetWorld()->GetTimeSeconds() < NextPilotShotTime) return ETunaSweeperEnemyFireResult::Cooldown;
    if (!FirePilotProjectile(Target)) return ETunaSweeperEnemyFireResult::Blocked;
    --PilotAmmo;
    NextPilotShotTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, PilotShotCooldown);
    Presentation->PlayPilotFire();
    UpdatePilotAmmoPhase();
    return ETunaSweeperEnemyFireResult::Fired;
}

bool ATunaSweeperHopperEnemyCharacter::FirePilotProjectile(AActor* Target)
{
    const FVector Location = Presentation->GetPilotMuzzleLocation();
    const FVector AimPoint = Target->GetActorLocation();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HopperPilotFire), true, this);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Location, AimPoint, ECC_Visibility, Query) && Hit.GetActor() != Target) return false;
    UClass* Class = ProjectileClass.LoadSynchronous();
    if (!Class) Class = ATunaSweeperProjectile::StaticClass();
    const FTransform Transform((AimPoint - Location).Rotation(), Location);
    ATunaSweeperProjectile* Projectile = GetWorld()->SpawnActorDeferred<ATunaSweeperProjectile>(Class, Transform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Projectile) return false;
    Projectile->SetDamageAmount(ProjectileDamage);
    Projectile->SetHitEffectId(ProjectileHitEffectId);
    Projectile->IgnoreActor(this);
    Projectile->SetAimIntent(Target, nullptr, AimPoint, true);
    Projectile->FinishSpawning(Transform);
    return true;
}

FTunaSweeperEnemyWeaponRuntimeStatus ATunaSweeperHopperEnemyCharacter::GetEnemyWeaponRuntimeStatus()
{
    FTunaSweeperEnemyWeaponRuntimeStatus Status;
    Status.FireMode = ETunaSweeperWeaponFireMode::SemiAutomatic;
    if (!IsDead() && Phase == EHopperCombatPhase::Mech)
        Status.LoadedAmmo = Status.MagazineCapacity = (IsValid(LeftArm) || IsValid(RightArm)) ? 1 : 0;
    else if (!IsDead() && Phase == EHopperCombatPhase::PilotRanged)
    {
        Status.MagazineCapacity = FMath::Max(0, PilotAmmoCapacity);
        Status.LoadedAmmo = PilotAmmo;
    }
    return Status;
}

bool ATunaSweeperHopperEnemyCharacter::AttackTarget(AActor* Target)
{
    if (Phase != EHopperCombatPhase::PilotMelee) return TryFireProjectileAt(Target) == ETunaSweeperEnemyFireResult::Fired;
    if (!CanAttackHopperTarget(Target) || GetWorld()->GetTimeSeconds() < NextPilotMeleeTime) return false;
    if (!Super::AttackTarget(Target)) return false;
    NextPilotMeleeTime = GetWorld()->GetTimeSeconds() + GetMeleeAttackCooldownSeconds();
    Presentation->PlayPilotMelee();
    return true;
}

void ATunaSweeperHopperEnemyCharacter::UpdatePilotAmmoPhase()
{
    if (Phase == EHopperCombatPhase::PilotRanged && PilotAmmo <= 0)
    {
        PilotAmmo = 0;
        Phase = EHopperCombatPhase::PilotMelee;
        ApplyPhaseMovementSpeed();
        Presentation->SetGunVisible(false);
    }
}

AHopperArmModule* ATunaSweeperHopperEnemyCharacter::GetArmModule(bool bLeft) const
{
    return bLeft ? LeftArm.Get() : RightArm.Get();
}

bool ATunaSweeperHopperEnemyCharacter::SetArmModule(bool bLeft, TSubclassOf<AHopperArmModule> Class)
{
    if (IsDead() || Phase == EHopperCombatPhase::Disembarking || Phase == EHopperCombatPhase::PilotRanged || Phase == EHopperCombatPhase::PilotMelee) return false;
    TObjectPtr<AHopperArmModule>& Slot = bLeft ? LeftArm : RightArm;
    AHopperArmModule* Replacement = nullptr;
    if (Class)
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.Instigator = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Replacement = GetWorld()->SpawnActor<AHopperArmModule>(Class, GetActorTransform(), Params);
        if (!Replacement) return false;
        USceneComponent* Socket = Presentation->GetArmSocket(bLeft);
        Replacement->AttachToComponent(Socket ? Socket : GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        Replacement->ConfigureVisual(LoadedVisualData, bLeft);
        if (VisionSubjectComponent) VisionSubjectComponent->AddLinkedActor(Replacement);
    }
    if (IsValid(Slot))
    {
        if (VisionSubjectComponent) VisionSubjectComponent->RemoveLinkedActor(Slot);
        Slot->Destroy();
    }
    Slot = Replacement;
    return true;
}

void ATunaSweeperHopperEnemyCharacter::BeginMechDestruction()
{
    Phase = EHopperCombatPhase::Disembarking;
    ApplyPhaseMovementSpeed();
    PhaseSeconds = 0;
    ExitRetrySeconds = 0;
    bExitInProgress = false;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    if (AController* AI = GetController()) AI->StopMovement();
    Presentation->SetMechActive(false);
    if (HasActorBegunPlay())
    {
        if (UNiagaraSystem* Effect = MechDestructionEffect.LoadSynchronous())
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, GetActorLocation());
    }
}

bool ATunaSweeperHopperEnemyCharacter::IsPilotExitClear(const FVector& Ground) const
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HopperExitClearance), false, this);
    if (LeftArm) Query.AddIgnoredActor(LeftArm);
    if (RightArm) Query.AddIgnoredActor(RightArm);
    FHitResult Floor;
    if (!GetWorld()->LineTraceSingleByChannel(Floor, Ground + FVector(0,0,8), Ground - FVector(0,0,12), ECC_Visibility, Query)
        || Floor.ImpactNormal.Z < GetCharacterMovement()->GetWalkableFloorZ()
        || FMath::Abs(Floor.ImpactPoint.Z - Ground.Z) > 5.f) return false;
    const FVector Center = Ground + FVector(0,0,PilotHalfHeight + 2.f);
    return !GetWorld()->OverlapBlockingTestByChannel(Center, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeCapsule(PilotRadius, PilotHalfHeight), Query);
}

bool ATunaSweeperHopperEnemyCharacter::IsPilotTransferClear(const FVector& StartFeet, const FVector& Ground, float StartAlpha) const
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HopperTransferClearance), false, this);
    if (LeftArm) Query.AddIgnoredActor(LeftArm);
    if (RightArm) Query.AddIgnoredActor(RightArm);
    auto CenterAt = [&StartFeet, &Ground](float T)
    {
        FVector Feet = FMath::Lerp(StartFeet, Ground, T*T*(3.f-2.f*T));
        Feet.Z += FMath::Sin(PI*T)*30.f;
        return Feet + FVector(0,0,PilotHalfHeight+2.f);
    };
    const float FirstAlpha = FMath::Clamp(StartAlpha,0.f,1.f);
    FVector Previous = CenterAt(FirstAlpha);
    for (int32 Segment=1; Segment<=16; ++Segment)
    {
        const FVector Next = CenterAt(FMath::Lerp(FirstAlpha,1.f,Segment/16.f));
        FHitResult Hit;
        if (GetWorld()->SweepSingleByChannel(Hit, Previous, Next, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(PilotRadius, PilotHalfHeight), Query))
        {
            return false;
        }
        Previous = Next;
    }
    return true;
}

bool ATunaSweeperHopperEnemyCharacter::FindPilotExit(FVector& OutGround) const
{
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Navigation)
    {
        UE_LOG(LogHopperCombat, Verbose, TEXT("Exit search waiting: no navigation system"));
        return false;
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HopperExitGround), false, this);
    if (LeftArm) Query.AddIgnoredActor(LeftArm);
    if (RightArm) Query.AddIgnoredActor(RightArm);
    const FVector Origin = GetActorLocation() - FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const FVector StartFeet = Presentation->GetPilotMesh() ? Presentation->GetPilotMesh()->GetComponentLocation() : GetActorLocation();
    FNavAgentProperties PilotAgent = GetNavAgentPropertiesRef();
    PilotAgent.AgentRadius = PilotRadius;
    PilotAgent.AgentHeight = PilotHalfHeight * 2.f;
    const ANavigationData* PilotNavData = Navigation->GetNavDataForProps(PilotAgent, Origin);
    FNavLocation NavOrigin;
    if (!PilotNavData || !Navigation->ProjectPointToNavigation(Origin, NavOrigin, FVector(100,100,180), PilotNavData))
    {
        UE_LOG(LogHopperCombat, Verbose, TEXT("Exit waiting: pilotNav=%s origin projection unavailable"), *GetNameSafe(PilotNavData));
        return false;
    }
    for (int32 Ring = 0; Ring < 3; ++Ring)
    {
        for (int32 Index = 0; Index < 8; ++Index)
        {
            const float Yaw = GetActorRotation().Yaw + Index * 45.f;
            const FVector Candidate = Origin + FRotator(0,Yaw,0).Vector() * (130.f + Ring * 60.f);
            FNavLocation NavLocation;
            if (!Navigation->ProjectPointToNavigation(Candidate, NavLocation, FVector(45,45,160), PilotNavData)) continue;
            FHitResult GroundHit;
            if (!GetWorld()->LineTraceSingleByChannel(GroundHit, NavLocation.Location + FVector(0,0,100),
                NavLocation.Location - FVector(0,0,180), ECC_Visibility, Query)
                || GroundHit.ImpactNormal.Z < GetCharacterMovement()->GetWalkableFloorZ()) continue;
            if (!IsPilotExitClear(GroundHit.ImpactPoint)) continue;
            if (!IsPilotTransferClear(StartFeet, GroundHit.ImpactPoint)) continue;
            FPathFindingQuery PathQuery(this, *PilotNavData, NavOrigin.Location, NavLocation.Location);
            PathQuery.SetAllowPartialPaths(false);
            PathQuery.SetNavAgentProperties(PilotAgent);
            const FPathFindingResult Path = Navigation->FindPathSync(PilotAgent, PathQuery);
            if (!Path.IsSuccessful() || !Path.Path.IsValid() || !Path.Path->IsValid() || Path.IsPartial()) continue;
            OutGround = GroundHit.ImpactPoint;
            return true;
        }
    }
    UE_LOG(LogHopperCombat, Verbose, TEXT("Exit waiting: no reachable destination with a clear pilot transfer"));
    return false;
}

void ATunaSweeperHopperEnemyCharacter::UpdateDisembark(float DeltaSeconds)
{
    if (!bExitInProgress)
    {
        ExitRetrySeconds -= DeltaSeconds;
        if (ExitRetrySeconds > 0) return;
        ExitRetrySeconds = .35f;
        if (!FindPilotExit(ExitGroundDestination)) return;
        bExitInProgress = true;
        PhaseSeconds = 0;
        ExitStartFeet = Presentation->GetPilotMesh() ? Presentation->GetPilotMesh()->GetComponentLocation() : GetActorLocation();
        Presentation->StartDisembark(ExitGroundDestination, DisembarkSeconds);
    }
    if (!IsPilotTransferClear(ExitStartFeet, ExitGroundDestination, PhaseSeconds / FMath::Max(.05f, DisembarkSeconds)))
    {
        bExitInProgress = false;
        Presentation->PauseTransfer();
        return;
    }
    PhaseSeconds += DeltaSeconds;
    if (PhaseSeconds < FMath::Max(0.f, DisembarkSeconds)) return;
    // Re-check dynamic obstacles at handoff; never teleport the smaller capsule into another actor.
    if (!IsPilotExitClear(ExitGroundDestination))
    {
        bExitInProgress = false;
        return;
    }
    EnterPilotCombat();
}

void ATunaSweeperHopperEnemyCharacter::EnterPilotCombat()
{
    GetCapsuleComponent()->SetCapsuleSize(PilotRadius, PilotHalfHeight);
    GetCharacterMovement()->UpdateNavAgent(*GetCapsuleComponent());
    SetActorLocation(ExitGroundDestination + FVector(0,0,PilotHalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
    Presentation->FinishDisembark();
    Phase = PilotAmmo > 0 ? EHopperCombatPhase::PilotRanged : EHopperCombatPhase::PilotMelee;
    ApplyPhaseMovementSpeed();
    Presentation->SetGunVisible(PilotAmmo > 0);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->MaxAcceleration = 600.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 800.f;
}

void ATunaSweeperHopperEnemyCharacter::OnDeathPresentationStarted()
{
    Super::OnDeathPresentationStarted();
    ApplyPhaseMovementSpeed();
    Presentation->SetMechActive(false);
    Presentation->SetGunVisible(false);
    Presentation->SetPilotLocomotion(false);
}

void ATunaSweeperHopperEnemyCharacter::EndPlay(EEndPlayReason::Type Reason)
{
    if (IsValid(LeftArm))
    {
        if (VisionSubjectComponent) VisionSubjectComponent->RemoveLinkedActor(LeftArm);
        LeftArm->Destroy();
    }
    if (IsValid(RightArm))
    {
        if (VisionSubjectComponent) VisionSubjectComponent->RemoveLinkedActor(RightArm);
        RightArm->Destroy();
    }
    LeftArm = RightArm = nullptr;
    Super::EndPlay(Reason);
}
