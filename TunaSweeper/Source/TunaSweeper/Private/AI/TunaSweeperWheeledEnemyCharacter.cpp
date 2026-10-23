#include "AI/TunaSweeperWheeledEnemyCharacter.h"

#include "AI/TunaSweeperWheeledMovementComponent.h"
#include "AIController.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "Weapon/TunaSweeperWeapon.h"

ATunaSweeperWheeledEnemyCharacter::ATunaSweeperWheeledEnemyCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UTunaSweeperWheeledMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    GetCapsuleComponent()->InitCapsuleSize(58, 60);
    auto MakePart = [this](const TCHAR* Name)
    {
        auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Part->SetupAttachment(RootComponent);
        // Set these before assigning meshes: registration must never cut the navmesh.
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        return Part;
    };
    RobotBody = MakePart(TEXT("RobotBody"));
    LeftWheel = MakePart(TEXT("LeftWheel"));
    RightWheel = MakePart(TEXT("RightWheel"));
    FrontWheel = MakePart(TEXT("FrontWheel"));
    VisualMesh->SetVisibility(false);
    VisualMesh->SetHiddenInGame(true);
    ForwardMarkerMesh->SetVisibility(false);
    ForwardMarkerMesh->SetHiddenInGame(true);
    FrontWheel->SetVisibility(false);
    FrontWheel->SetHiddenInGame(true);
    MovementSpeedRandomOffset = FVector2D::ZeroVector;
    MaxHealth = 60;
    FootstepNoiseLoudness = 0;
    DeathNiagaraEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
        TEXT("/Game/Effects/ExplosionTuna/NS_Explosion_SmokeRobot.NS_Explosion_SmokeRobot")));
    ConfigureDefaultCombat();
}

void ATunaSweeperWheeledEnemyCharacter::ConfigureDefaultCombat()
{
    // Raid spawn profiles take precedence. Unprofiled, directly placed BPs still fight.
    if (!GetCombatProfileId().IsNone()) return;
    FTunaSweeperEnemyCombatProfile Profile;
    Profile.AttackMode = bTricycle ? ETunaSweeperEnemyAttackMode::Ranged : ETunaSweeperEnemyAttackMode::Melee;
    Profile.Role = bTricycle ? ETunaSweeperEnemyCombatRole::Flanker : ETunaSweeperEnemyCombatRole::Melee;
    Profile.MovementSpeed = bTricycle ? 320.0f : 230.0f;
    Profile.TrackingRange = 1800;
    // Stop outside the combined delivery/trike capsule radii (58 + 64 cm).
    Profile.MeleeApproachStopRange = 125;
    Profile.MeleeApproachStartRange = 145;
    Profile.MeleeAttackDamage = 7;
    Profile.AttackCooldownSeconds = 1.25f;
    ConfigureCombatProfile(Profile, FactionComponent->GetFactionId(), FactionComponent->GetSquadId(), FactionComponent->GetSquadSlot());
}

void ATunaSweeperWheeledEnemyCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ConfigureDefaultCombat();
    ApplyRobotPresentation();
}

void ATunaSweeperWheeledEnemyCharacter::BeginPlay()
{
    ConfigureDefaultCombat();
    ApplyRobotPresentation();
    Super::BeginPlay();
    // The base refreshes its prototype visuals during BeginPlay.
    ApplyRobotPresentation();
    AlignBuiltInMuzzle();
    PreviousRobotLocation = GetActorLocation();
    PreviousRobotRotation = GetActorRotation();
}

void ATunaSweeperWheeledEnemyCharacter::ApplyRobotPresentation()
{
    const float HalfHeight = bTricycle ? 78.0f : 60.0f;
    GetCapsuleComponent()->SetCapsuleSize(bTricycle ? 64.0f : 58.0f, HalfHeight);
    RobotBody->SetRelativeLocation(FVector(0, 0, -HalfHeight));
    RobotBody->SetStaticMesh(RobotBodyMesh.LoadSynchronous());
    if (UMaterialInterface* Eyes = EyeMaterial.LoadSynchronous()) RobotBody->SetMaterial(1, Eyes);
    UStaticMesh* Wheels = WheelMesh.LoadSynchronous();
    for (auto* Wheel : { LeftWheel.Get(), RightWheel.Get(), FrontWheel.Get() })
        Wheel->SetStaticMesh(Wheels);
    const float AxleX = bTricycle ? -65.0f : 0.0f;
    const float AxleY = bTricycle ? 46.0f : 48.0f;
    const float Radius = bTricycle ? 30.0f : 32.0f;
    LeftWheel->SetRelativeLocation(FVector(AxleX, -AxleY, Radius - HalfHeight));
    RightWheel->SetRelativeLocation(FVector(AxleX, AxleY, Radius - HalfHeight));
    FrontWheel->SetRelativeLocation(FVector(77, 0, Radius - HalfHeight));
    FrontWheel->SetVisibility(bTricycle);
    FrontWheel->SetHiddenInGame(!bTricycle);
    VisualMesh->SetVisibility(false);
    VisualMesh->SetHiddenInGame(true);
    ForwardMarkerMesh->SetVisibility(false);
    ForwardMarkerMesh->SetHiddenInGame(true);
    // Ranged robot meshes contain their own barrel; the hidden weapon supplies firing logic.
    ProjectileSpawnOffset = FVector(100, 0, 55 - HalfHeight);
    EnemyWeaponAttachPoint->SetRelativeLocation(FVector(20, 0, 55 - HalfHeight));
    const float Overhead = (bTricycle ? 170.0f : 125.0f) - HalfHeight;
    AlertIndicatorMesh->SetRelativeLocation(FVector(0, 0, Overhead));
    EnemyReloadWidgetComponent->SetRelativeLocation(FVector(0, 0, Overhead));
    EnemyStatusBubbleWidgetComponent->SetRelativeLocation(FVector(0, 0, Overhead + 20));
}

void ATunaSweeperWheeledEnemyCharacter::AlignBuiltInMuzzle()
{
    if (!bTricycle || UsesMeleeAttack()) return;
    TArray<AActor*> Attached;
    GetAttachedActors(Attached);
    for (AActor* Actor : Attached)
    {
        if (auto* Weapon = Cast<ATunaSweeperWeapon>(Actor); Weapon && Weapon->GetOwner() == this)
        {
            const FVector Desired = RobotBody->GetComponentTransform().TransformPosition(FVector(100, 0, 55));
            EnemyWeaponAttachPoint->AddWorldOffset(Desired - Weapon->GetMuzzleWorldLocation());
            break;
        }
    }
}

ETunaSweeperEnemyFireResult ATunaSweeperWheeledEnemyCharacter::TryFireProjectileAt(AActor* TargetActor)
{
    if (IsDead() || UsesMeleeAttack()) return ETunaSweeperEnemyFireResult::Blocked;
    if (bTricycle)
    {
        if (!TargetActor || IsDriving()) return ETunaSweeperEnemyFireResult::Blocked;
        const FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        if (Direction.IsNearlyZero() || FMath::Abs(FMath::FindDeltaAngleDegrees(
            GetActorRotation().Yaw, Direction.Rotation().Yaw)) > GetCombatProfile().AttackFacingToleranceDegrees)
            return ETunaSweeperEnemyFireResult::Blocked;
    }
    GetEnemyWeaponRuntimeStatus();
    AlignBuiltInMuzzle();
    return Super::TryFireProjectileAt(TargetActor);
}

bool ATunaSweeperWheeledEnemyCharacter::IsDriving() const
{
    if (!bTricycle) return false;
    if (GetVelocity().SizeSquared2D() > FMath::Square(5.0f)) return true;
    const auto* Movement = Cast<UTunaSweeperWheeledMovementComponent>(GetCharacterMovement());
    if (Movement && Movement->HasSteeringInput()) return true;
    const auto* AI = Cast<AAIController>(GetController());
    return AI && AI->GetMoveStatus() == EPathFollowingStatus::Moving;
}

void ATunaSweeperWheeledEnemyCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsDead() || DeltaSeconds <= 0) return;
    const FVector Location = GetActorLocation();
    const FRotator Rotation = GetActorRotation();
    const FVector AverageForward = (Rotation.Vector() + PreviousRobotRotation.Vector()).GetSafeNormal2D();
    const double Distance = FVector::DotProduct(Location - PreviousRobotLocation, AverageForward);
    const double YawRadians = FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(PreviousRobotRotation.Yaw, Rotation.Yaw));
    const double Radius = bTricycle ? 30.0 : 32.0;
    const double HalfTrack = bTricycle ? 46.0 : 48.0;
    LeftRollRadians = FMath::UnwindRadians(LeftRollRadians + (Distance + YawRadians * HalfTrack) / Radius);
    RightRollRadians = FMath::UnwindRadians(RightRollRadians + (Distance - YawRadians * HalfTrack) / Radius);
    if (bTricycle)
    {
        // Rear axle X=-65, front axle X=77: steer with the actual turn radius.
        // A stopped robot can pivot on its driven rear wheels with the front caster sideways.
        const float SteeringLimit = FMath::Abs(Distance) > 0.5 ? 45.0f : 90.0f;
        const float DesiredSteering = FMath::IsNearlyZero(YawRadians) ? 0.0f :
            FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(142.0 * YawRadians,
                FMath::Max(FMath::Abs(Distance), 0.01)))), -SteeringLimit, SteeringLimit);
        FrontSteeringDegrees = FMath::FInterpConstantTo(FrontSteeringDegrees, DesiredSteering, DeltaSeconds, 240.0f);
    }
    const double SteeringRadians = FMath::DegreesToRadians(FrontSteeringDegrees);
    const double FrontDistance = Distance * FMath::Cos(SteeringRadians) +
        YawRadians * 77.0 * FMath::Sin(SteeringRadians);
    FrontRollRadians = FMath::UnwindRadians(FrontRollRadians + FrontDistance / Radius);
    LeftWheel->SetRelativeRotation(FQuat(FVector::RightVector, LeftRollRadians));
    RightWheel->SetRelativeRotation(FQuat(FVector::RightVector, RightRollRadians));
    FrontWheel->SetRelativeRotation(FQuat(FVector::UpVector, SteeringRadians) *
        FQuat(FVector::RightVector, FrontRollRadians));
    PreviousRobotLocation = Location;
    PreviousRobotRotation = Rotation;
}

void ATunaSweeperWheeledEnemyCharacter::OnDeathPresentationStarted()
{
    Super::OnDeathPresentationStarted();
    for (auto* Part : { RobotBody.Get(), LeftWheel.Get(), RightWheel.Get(), FrontWheel.Get() })
    {
        Part->SetVisibility(false);
        Part->SetHiddenInGame(true);
    }
}
