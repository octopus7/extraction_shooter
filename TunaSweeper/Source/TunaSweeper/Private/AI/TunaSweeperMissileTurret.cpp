#include "AI/TunaSweeperMissileTurret.h"

#include "AI/TunaSweeperAttackTelegraph.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Component/TunaSweeperScratchComponent.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperCombatPatternEffectActor.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperMissileTurret::ATunaSweeperMissileTurret()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(true);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	Hurtbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hurtbox"));
	Hurtbox->SetupAttachment(SceneRoot);
	Hurtbox->InitBoxExtent(FVector(44.0f, 44.0f, 60.0f));
	Hurtbox->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	Hurtbox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Hurtbox->SetCollisionObjectType(ECC_Pawn);
	Hurtbox->SetCollisionResponseToAllChannels(ECR_Block);
	Hurtbox->SetCanEverAffectNavigation(false);

	FactionComponent = CreateDefaultSubobject<UTunaSweeperFactionComponent>(TEXT("FactionComponent"));
	VisionSubjectComponent = CreateDefaultSubobject<UTunaSweeperVisionSubjectComponent>(TEXT("VisionSubjectComponent"));
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	LauncherMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LauncherMesh"));
	LeftLaunchTube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLaunchTube"));
	RightLaunchTube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLaunchTube"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BaseAsset(TEXT("/Game/Characters/CombatPatterns/Meshes/SM_CP_TurretBase.SM_CP_TurretBase"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadAsset(TEXT("/Game/Characters/CombatPatterns/Meshes/SM_CP_TurretHead.SM_CP_TurretHead"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TubeAsset(TEXT("/Game/Characters/CombatPatterns/Meshes/SM_CP_TurretTube.SM_CP_TurretTube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MissileAsset(TEXT("/Game/Characters/CombatPatterns/Meshes/SM_CP_Missile.SM_CP_Missile"));
	for (UStaticMeshComponent* Part : {BaseMesh.Get(), LauncherMesh.Get(), LeftLaunchTube.Get(), RightLaunchTube.Get()})
	{
		Part->SetupAttachment(SceneRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
	}
	BaseMesh->SetStaticMesh(BaseAsset.Object);
	LauncherMesh->SetStaticMesh(HeadAsset.Object);
	LeftLaunchTube->SetStaticMesh(TubeAsset.Object);
	RightLaunchTube->SetStaticMesh(TubeAsset.Object);
	LauncherMesh->SetRelativeLocation(FVector(0, 0, 69));
	LeftLaunchTube->SetRelativeLocation(FVector(0, -28, 65));
	RightLaunchTube->SetRelativeLocation(FVector(0, 28, 65));
	MissileVisualAsset = MissileAsset.Object;
	TelegraphClass = ATunaSweeperAttackTelegraph::StaticClass();
}

void ATunaSweeperMissileTurret::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = FMath::Max(1.0f, MaxHealth);
}

void ATunaSweeperMissileTurret::InitializeTurret(AActor* Source, AActor* Target)
{
	ClearWarning();
	SourceActor = Source;
	TargetActor = Target;
	bInitialized = IsValid(Source);
	Age = 0.0f;
	TimeToNextWarning = FMath::Max(0.0f, InitialFireDelay);
	SetOwner(Source);
	SetInstigator(Cast<APawn>(Source) ? Cast<APawn>(Source) : (Source ? Source->GetInstigator() : nullptr));
	if (const UWorld* World = GetWorld())
	{
		if (const UTunaSweeperFactionSubsystem* Factions = World->GetSubsystem<UTunaSweeperFactionSubsystem>())
		{
			FactionComponent->SetFactionId(Factions->GetFactionIdForActor(Source));
		}
	}
}

bool ATunaSweeperMissileTurret::IsSourceActive() const
{
	const AActor* Source = SourceActor.Get();
	if (!IsValid(Source))
	{
		return false;
	}
	if (const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(Source); Enemy && Enemy->IsDead())
	{
		return false;
	}
	if (const ATunaSweeperTopDownCharacter* Player = Cast<ATunaSweeperTopDownCharacter>(Source); Player && Player->IsDead())
	{
		return false;
	}
	const UTunaSweeperFactionComponent* SourceFaction = Source->FindComponentByClass<UTunaSweeperFactionComponent>();
	return !SourceFaction || SourceFaction->CanBeCombatTarget();
}

bool ATunaSweeperMissileTurret::IsValidTarget(AActor* Candidate) const
{
	if (!IsValid(Candidate) || !GetWorld() || !Candidate->CanBeDamaged() ||
		FVector::DistSquared2D(Candidate->GetActorLocation(), GetActorLocation()) > FMath::Square(FMath::Max(1.0f, TargetRange)))
	{
		return false;
	}
	if (const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(Candidate); Enemy && Enemy->IsDead())
	{
		return false;
	}
	if (const ATunaSweeperTopDownCharacter* Player = Cast<ATunaSweeperTopDownCharacter>(Candidate); Player && Player->IsDead())
	{
		return false;
	}
	const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
	return Factions && Factions->CanTargetActor(this, Candidate);
}

AActor* ATunaSweeperMissileTurret::ResolveTarget() const
{
	if (IsValidTarget(TargetActor.Get()))
	{
		return TargetActor.Get();
	}
	const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
	TArray<AActor*> Candidates;
	if (Factions)
	{
		Factions->GetActorsWithAttitude(this, ETunaSweeperFactionAttitude::Hostile, Candidates);
	}
	AActor* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (AActor* Candidate : Candidates)
	{
		const double Distance = FVector::DistSquared2D(Candidate->GetActorLocation(), GetActorLocation());
		if (Distance < BestDistance && IsValidTarget(Candidate))
		{
			Best = Candidate;
			BestDistance = Distance;
		}
	}
	return Best;
}

bool ATunaSweeperMissileTurret::BeginWarning()
{
	AActor* Target = ResolveTarget();
	if (!Target)
	{
		return false;
	}
	FVector GroundLocation = Target->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		GroundLocation.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TunaSweeperMissileGround), false, this);
	Query.AddIgnoredActor(Target);
	Query.AddIgnoredActor(SourceActor.Get());
	FHitResult GroundHit;
	if (!GetWorld()->LineTraceSingleByChannel(GroundHit,
		GroundLocation + FVector(0.0f, 0.0f, 100.0f), GroundLocation - FVector(0.0f, 0.0f, 300.0f), ECC_Visibility, Query) ||
		GroundHit.ImpactNormal.Z < 0.5f)
	{
		return false;
	}
	LockedImpactLocation = GroundHit.ImpactPoint;
	ActiveWarningDuration = FMath::Max(0.1f, WarningDuration);
	ActiveImpactRadius = FMath::Max(1.0f, ImpactRadius);
	FActorSpawnParameters Parameters;
	Parameters.Owner = this;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveWarning = GetWorld()->SpawnActor<ATunaSweeperAttackTelegraph>(
		TelegraphClass ? TelegraphClass.Get() : ATunaSweeperAttackTelegraph::StaticClass(),
		LockedImpactLocation, FRotator::ZeroRotator, Parameters);
	if (!ActiveWarning)
	{
		return false;
	}
	ActiveWarning->InitCircle(LockedImpactLocation, ActiveImpactRadius, ActiveWarningDuration);
	MissileMesh = NewObject<UStaticMeshComponent>(ActiveWarning, TEXT("FallingMissile"));
	ActiveWarning->AddInstanceComponent(MissileMesh);
	MissileMesh->SetupAttachment(ActiveWarning->GetRootComponent());
	MissileMesh->SetStaticMesh(MissileVisualAsset);
	MissileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MissileMesh->SetGenerateOverlapEvents(false);
	MissileMesh->SetCanEverAffectNavigation(false);
	MissileMesh->SetCastShadow(false);
	MissileMesh->SetRelativeScale3D(FVector::OneVector);
	MissileMesh->SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));
	MissileMesh->SetVisibility(false);
	if (MissileMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(MissileMaterial, ActiveWarning);
		Material->SetScalarParameterValue(TEXT("Intensity"), 3.0f);
		MissileMesh->SetMaterial(0, Material);
	}
	MissileMesh->RegisterComponent();
	ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::MissileLaunch,
		GetActorLocation() + FVector(0, 0, 120), 45.0f, FVector::UpVector, this);
	MissileTrailCountdown = 0.0f;
	WarningElapsed = 0.0f;
	bWarningActive = true;
	TargetActor = Target;
	return true;
}

float ATunaSweeperMissileTurret::GetWarningProgress() const
{
	return bWarningActive ? FMath::Clamp(WarningElapsed / ActiveWarningDuration, 0.0f, 1.0f) : 0.0f;
}

void ATunaSweeperMissileTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInitialized || bDead)
	{
		return;
	}
	Age += FMath::Max(0.0f, DeltaSeconds);
	if (!IsSourceActive() || Age >= FMath::Max(1.0f, Lifetime))
	{
		ClearWarning();
		Destroy();
		return;
	}
	if (!bWarningActive)
	{
		TimeToNextWarning -= DeltaSeconds;
		if (TimeToNextWarning <= 0.0f && !BeginWarning())
		{
			TimeToNextWarning = 0.3f;
		}
		return;
	}
	if (!IsValid(ActiveWarning))
	{
		// Never damage an area after its warning has been removed externally.
		ClearWarning();
		TimeToNextWarning = FMath::Max(0.1f, SalvoInterval);
		return;
	}
	WarningElapsed = FMath::Min(WarningElapsed + FMath::Max(0.0f, DeltaSeconds), ActiveWarningDuration);
	ActiveWarning->SetProgress(GetWarningProgress());
	const float DescentDuration = FMath::Clamp(MissileDescentDuration, 0.01f, ActiveWarningDuration);
	const float DescentProgress = FMath::Clamp((WarningElapsed - (ActiveWarningDuration - DescentDuration)) / DescentDuration, 0.0f, 1.0f);
	MissileMesh->SetVisibility(DescentProgress > 0.0f);
	MissileMesh->SetWorldLocation(LockedImpactLocation + FVector(0.0f, 0.0f, 95.0f + FMath::Max(1.0f, MissileDropHeight) * (1.0f - DescentProgress)));
	if (DescentProgress > 0.0f)
	{
		MissileTrailCountdown -= DeltaSeconds;
		if (MissileTrailCountdown <= 0.0f)
		{
			ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::MissileTrail,
				MissileMesh->GetComponentLocation(), 24.0f, FVector::UpVector, ActiveWarning);
			MissileTrailCountdown = 0.07f;
		}
	}
	if (WarningElapsed >= ActiveWarningDuration)
	{
		Impact();
		ClearWarning();
		TimeToNextWarning = FMath::Max(0.1f, SalvoInterval);
	}
}

void ATunaSweeperMissileTurret::Impact()
{
	SpawnExplosion(LockedImpactLocation, ActiveImpactRadius);
	UWorld* World = GetWorld();
	if (!World || ImpactDamage <= 0.0f)
	{
		return;
	}
	const UTunaSweeperFactionSubsystem* Factions = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TunaSweeperMissileImpact), false, this);
	Query.AddIgnoredActor(SourceActor.Get());
	const float Height = FMath::Max(1.0f, ImpactHeight);
	float MaximumNearMissMargin = 0.0f;
	if (Factions)
	{
		TArray<AActor*> HostileActors;
		Factions->GetActorsWithAttitude(this, ETunaSweeperFactionAttitude::Hostile, HostileActors);
		for (const AActor* Hostile : HostileActors)
		{
			if (const UTunaSweeperScratchComponent* Scratch = Hostile->FindComponentByClass<UTunaSweeperScratchComponent>())
			{
				MaximumNearMissMargin = FMath::Max(MaximumNearMissMargin, Scratch->GetProjectileNearMissMarginCm());
			}
		}
	}
	const float QueryRadius = ActiveImpactRadius + MaximumNearMissMargin;
	World->OverlapMultiByObjectType(Overlaps, LockedImpactLocation + FVector(0.0f, 0.0f, Height * 0.5f),
		FQuat::Identity, Objects, FCollisionShape::MakeBox(FVector(QueryRadius, QueryRadius, Height * 0.5f)), Query);
	FCollisionObjectQueryParams CoverObjects;
	CoverObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	CoverObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Victim = Overlap.GetActor();
		UPrimitiveComponent* Primitive = Overlap.GetComponent();
		if (!IsValid(Victim) || !Primitive || DamagedActors.Contains(Victim) ||
			!Factions || !Factions->CanTargetActor(this, Victim))
		{
			continue;
		}
		FVector ClosestPoint;
		if (Primitive->GetClosestPointOnCollision(LockedImpactLocation + FVector(0.0f, 0.0f, 35.0f), ClosestPoint) < 0.0f)
		{
			continue;
		}
		const float Clearance = FVector::Dist2D(ClosestPoint, LockedImpactLocation) - ActiveImpactRadius;
		UTunaSweeperScratchComponent* Scratch = Victim->FindComponentByClass<UTunaSweeperScratchComponent>();
		if (Clearance > 0.0f && (!Scratch || Clearance > Scratch->GetProjectileNearMissMarginCm()))
		{
			continue;
		}
		float FeetZ = Primitive->Bounds.Origin.Z - Primitive->Bounds.BoxExtent.Z;
		if (const ACharacter* Character = Cast<ACharacter>(Victim))
		{
			FeetZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
		if (FMath::Abs(FeetZ - LockedImpactLocation.Z) > 100.0f)
		{
			continue;
		}
		FHitResult Obstruction;
		// Other pawn capsules do not shield an area explosion; physical world cover still does.
		if (World->LineTraceSingleByObjectType(Obstruction, LockedImpactLocation + FVector(0.0f, 0.0f, 35.0f),
			Primitive->Bounds.Origin, CoverObjects, Query) && Obstruction.GetActor() != Victim)
		{
			continue;
		}
		DamagedActors.Add(Victim);
		if (Scratch && IsValid(ActiveWarning))
		{
			// The warning actor identifies this individual salvo; the same floor and LOS rules apply.
			Scratch->TryRegisterNearMiss(ActiveWarning, 0, ETunaSweeperNearMissAttackType::Projectile, Clearance, Clearance <= 0.0f);
		}
		if (Clearance <= 0.0f)
		{
			// The standard damage path retains the player's dodge invulnerability and armor handling.
			UGameplayStatics::ApplyDamage(Victim, ImpactDamage, GetInstigatorController(), this,
				ImpactDamageType ? ImpactDamageType.Get() : UDamageType::StaticClass());
		}
	}
}

void ATunaSweeperMissileTurret::SpawnExplosion(const FVector& Location, float Radius) const
{
	ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::Impact,
		Location, Radius, FVector::UpVector, const_cast<ATunaSweeperMissileTurret*>(this));
}
float ATunaSweeperMissileTurret::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || !CanBeDamaged() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	const AActor* DamageSource = DamageCauser ? DamageCauser : EventInstigator;
	if (const UWorld* World = GetWorld())
	{
		if (const UTunaSweeperFactionSubsystem* Factions = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
			Factions && !Factions->CanApplyCombatEffect(DamageSource, this))
		{
			return 0.0f;
		}
	}
	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth -= AppliedDamage;
	Super::TakeDamage(AppliedDamage, DamageEvent, EventInstigator, DamageCauser);
	if (CurrentHealth <= 0.0f)
	{
		bDead = true;
		FactionComponent->SetCanBeCombatTarget(false);
		ClearWarning();
		SpawnExplosion(GetActorLocation(), 85.0f);
		Destroy();
	}
	return AppliedDamage;
}

void ATunaSweeperMissileTurret::ClearWarning()
{
	if (IsValid(ActiveWarning))
	{
		ActiveWarning->Destroy();
	}
	ActiveWarning = nullptr;
	bWarningActive = false;
	WarningElapsed = 0.0f;
	MissileMesh = nullptr;
}

void ATunaSweeperMissileTurret::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearWarning();
	Super::EndPlay(EndPlayReason);
}
