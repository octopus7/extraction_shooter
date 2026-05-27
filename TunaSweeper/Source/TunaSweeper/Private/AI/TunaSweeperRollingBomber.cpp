#include "AI/TunaSweeperRollingBomber.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperRollingBomber
{
	const TCHAR* SphereMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	constexpr float MinFootGroundNormalZ = 0.25f;
}

ATunaSweeperRollingBomber::ATunaSweeperRollingBomber()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = nullptr;
	AutoPossessAI = EAutoPossessAI::Disabled;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 56.0f);
	GetCharacterMovement()->MaxWalkSpeed = ProjectileModeWalkSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);

	MaxHealth = 45.0f;
	MovementSpeed = ProjectileModeWalkSpeed;
	MovementSpeedRandomOffset = FVector2D::ZeroVector;
	ProjectileSpawnOffset = FVector(58.0f, 0.0f, 42.0f);
	ProjectileDamage = 8.0f;
	ExplosionDamageType = UDamageType::StaticClass();
	EyeMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.8f));
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TunaSweeperRollingBomber::SphereMeshPath);
		if (SphereMesh.Succeeded())
		{
			VisualMesh->SetStaticMesh(SphereMesh.Object);
		}
	}

	if (ForwardMarkerMesh)
	{
		ForwardMarkerMesh->SetVisibility(false);
		ForwardMarkerMesh->SetHiddenInGame(true);
		ForwardMarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	LeftFootIKTarget = CreateDefaultSubobject<USceneComponent>(TEXT("LeftFootIKTarget"));
	LeftFootIKTarget->SetupAttachment(RootComponent);
	LeftFootIKTarget->SetRelativeLocation(LeftFootHomeLocalOffset);

	RightFootIKTarget = CreateDefaultSubobject<USceneComponent>(TEXT("RightFootIKTarget"));
	RightFootIKTarget->SetupAttachment(RootComponent);
	RightFootIKTarget->SetRelativeLocation(RightFootHomeLocalOffset);

	LeftKneeIKTarget = CreateDefaultSubobject<USceneComponent>(TEXT("LeftKneeIKTarget"));
	LeftKneeIKTarget->SetupAttachment(RootComponent);
	LeftKneeIKTarget->SetRelativeLocation(FVector(12.0f, -42.0f, -6.0f));

	RightKneeIKTarget = CreateDefaultSubobject<USceneComponent>(TEXT("RightKneeIKTarget"));
	RightKneeIKTarget->SetupAttachment(RootComponent);
	RightKneeIKTarget->SetRelativeLocation(FVector(12.0f, 42.0f, -6.0f));

	EyeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeMesh"));
	EyeMesh->SetupAttachment(RootComponent);
	EyeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EyeMesh->SetRelativeLocation(EyeLocalOffset);
	EyeMesh->SetRelativeScale3D(EyeLocalScale);
	EyeMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> EyeSphereMesh(TunaSweeperRollingBomber::SphereMeshPath);
	if (EyeSphereMesh.Succeeded())
	{
		EyeMesh->SetStaticMesh(EyeSphereMesh.Object);
	}

	EyeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeLight"));
	EyeLight->SetupAttachment(RootComponent);
	EyeLight->SetRelativeLocation(EyeLocalOffset + FVector(18.0f, 0.0f, 0.0f));
	EyeLight->SetCastShadows(false);
	EyeLight->SetLightColor(NormalEyeColor);
	EyeLight->SetIntensity(NormalEyeLightIntensity);
	EyeLight->SetAttenuationRadius(NormalEyeLightRadius);
}

void ATunaSweeperRollingBomber::BeginPlay()
{
	Super::BeginPlay();

	ApplyRollingBomberVisualDefaults();
	ApplyEyeVisualDefaults();
	SetEyeChargeWarningActive(false, true);
	LastActorLocation = GetActorLocation();
	ProjectileFireElapsedSeconds = ProjectileFireIntervalSeconds;
	InitializeLegIKTargets();
	EnterProjectileAttackMode();
}

void ATunaSweeperRollingBomber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bHasSelfDestructed || DeltaSeconds <= 0.0f)
	{
		return;
	}

	UpdateSpawnerLaunchState(DeltaSeconds);

	ATunaSweeperTopDownCharacter* TargetCharacter = ResolvePlayerTarget();

	switch (CurrentMode)
	{
	case ETunaSweeperRollingBomberMode::ProjectileAttack:
		UpdateProjectileAttackMode(DeltaSeconds, TargetCharacter);
		break;
	case ETunaSweeperRollingBomberMode::FoldingLegs:
		UpdateFoldingLegsMode(DeltaSeconds, TargetCharacter);
		break;
	case ETunaSweeperRollingBomberMode::Rolling:
		UpdateRollingMode(DeltaSeconds, TargetCharacter);
		break;
	case ETunaSweeperRollingBomberMode::RecoveringLegs:
		UpdateRecoveringLegsMode(DeltaSeconds);
		break;
	case ETunaSweeperRollingBomberMode::SelfDestructed:
		break;
	default:
		break;
	}

	if (bHasSelfDestructed)
	{
		return;
	}

	UpdateEyeVisualState(DeltaSeconds);

	if (CurrentMode == ETunaSweeperRollingBomberMode::ProjectileAttack)
	{
		FVector PlanarMoveDirection = GetVelocity();
		PlanarMoveDirection.Z = 0.0f;
		if (PlanarMoveDirection.IsNearlyZero())
		{
			PlanarMoveDirection = GetActorLocation() - LastActorLocation;
			PlanarMoveDirection.Z = 0.0f;
		}

		UpdateLegIK(DeltaSeconds, PlanarMoveDirection);
	}
	else
	{
		UpdateFoldedLegSceneComponents();
	}

	LastActorLocation = GetActorLocation();
}

void ATunaSweeperRollingBomber::LaunchFromSpawner(const FVector& LaunchVelocity)
{
	if (bHasSelfDestructed || LaunchVelocity.IsNearlyZero())
	{
		return;
	}

	bSpawnerLaunchActive = true;
	SpawnerLaunchControlRemainingSeconds = FMath::Max(0.0f, SpawnerLaunchControlGraceSeconds);
	ProjectileModeElapsedSeconds = 0.0f;
	ProjectileFireElapsedSeconds = ProjectileFireIntervalSeconds;
	bProjectileModeClosingDistance = false;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Falling);
	}

	LaunchCharacter(LaunchVelocity, true, true);
}

FTunaSweeperRollingBomberFootIKState ATunaSweeperRollingBomber::GetFootIKState(
	ETunaSweeperRollingBomberFoot Foot) const
{
	return Foot == ETunaSweeperRollingBomberFoot::Left
		? BuildFootIKState(LeftFootRuntime)
		: BuildFootIKState(RightFootRuntime);
}

void ATunaSweeperRollingBomber::ApplyRollingBomberVisualDefaults()
{
	if (VisualMesh)
	{
		if (UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TunaSweeperRollingBomber::SphereMeshPath))
		{
			VisualMesh->SetStaticMesh(SphereMesh);
		}

		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.8f));
		VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyVisualBaseRelativeRotation = VisualMesh->GetRelativeRotation();
	}

	if (ForwardMarkerMesh)
	{
		ForwardMarkerMesh->SetVisibility(false);
		ForwardMarkerMesh->SetHiddenInGame(true);
		ForwardMarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ATunaSweeperRollingBomber::EnterProjectileAttackMode()
{
	CurrentMode = ETunaSweeperRollingBomberMode::ProjectileAttack;
	ProjectileModeElapsedSeconds = 0.0f;
	ProjectileFireElapsedSeconds = ProjectileFireIntervalSeconds;
	ModeElapsedSeconds = 0.0f;
	LegFoldAlpha = 0.0f;
	bLegIKEnabled = true;
	bProjectileModeClosingDistance = false;
	RollDistanceTraveled = 0.0f;

	SetEyeChargeWarningActive(false, false);
	ResetBodyRollVisualRotation();
	InitializeLegIKTargets();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = ProjectileModeWalkSpeed;
	}
}

void ATunaSweeperRollingBomber::EnterFoldingLegsMode()
{
	CurrentMode = ETunaSweeperRollingBomberMode::FoldingLegs;
	ModeElapsedSeconds = 0.0f;
	LegFoldAlpha = 0.0f;
	bLegIKEnabled = false;
	bProjectileModeClosingDistance = false;

	SetEyeChargeWarningActive(true, false);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
}

void ATunaSweeperRollingBomber::EnterRollingMode(ATunaSweeperTopDownCharacter* TargetCharacter)
{
	CurrentMode = ETunaSweeperRollingBomberMode::Rolling;
	ModeElapsedSeconds = 0.0f;
	LegFoldAlpha = 1.0f;
	bLegIKEnabled = false;
	RollDistanceTraveled = 0.0f;
	BodyRollDegrees = 0.0f;
	LockedRollDirection = ResolveRollDirection(TargetCharacter);

	SetEyeChargeWarningActive(true, false);
	if (!LockedRollDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, LockedRollDirection.Rotation().Yaw, 0.0f));
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
}

void ATunaSweeperRollingBomber::EnterRecoveringLegsMode()
{
	CurrentMode = ETunaSweeperRollingBomberMode::RecoveringLegs;
	ModeElapsedSeconds = 0.0f;
	LegFoldAlpha = 1.0f;
	bLegIKEnabled = false;

	SetEyeChargeWarningActive(true, false);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
}

void ATunaSweeperRollingBomber::SelfDestruct()
{
	if (bHasSelfDestructed)
	{
		return;
	}

	bHasSelfDestructed = true;
	CurrentMode = ETunaSweeperRollingBomberMode::SelfDestructed;
	bLegIKEnabled = false;
	LegFoldAlpha = 1.0f;

	SetEyeChargeWarningActive(true, true);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	SetActorEnableCollision(false);
	ApplyExplosionDamage();
	Destroy();
}

void ATunaSweeperRollingBomber::UpdateProjectileAttackMode(
	float DeltaSeconds,
	ATunaSweeperTopDownCharacter* TargetCharacter)
{
	if (bSpawnerLaunchActive)
	{
		ProjectileModeElapsedSeconds = 0.0f;
		ProjectileFireElapsedSeconds = ProjectileFireIntervalSeconds;
		bProjectileModeClosingDistance = false;
		return;
	}

	if (!TargetCharacter)
	{
		ProjectileModeElapsedSeconds = 0.0f;
		bProjectileModeClosingDistance = false;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
		return;
	}

	FVector ToTarget = TargetCharacter->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0f;
	const float DistanceToTarget = ToTarget.Size();
	if (DistanceToTarget > TargetTrackingRange || ToTarget.IsNearlyZero())
	{
		ProjectileModeElapsedSeconds = 0.0f;
		bProjectileModeClosingDistance = false;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
		return;
	}

	const FVector DirectionToTarget = ToTarget / DistanceToTarget;
	SetActorRotation(FRotator(0.0f, DirectionToTarget.Rotation().Yaw, 0.0f));
	UpdateProjectileModeMovement(DistanceToTarget, DirectionToTarget);

	if (DistanceToTarget > ProjectileAttackRange)
	{
		ProjectileModeElapsedSeconds = 0.0f;
		ProjectileFireElapsedSeconds = ProjectileFireIntervalSeconds;
		return;
	}

	ProjectileModeElapsedSeconds += DeltaSeconds;
	ProjectileFireElapsedSeconds += DeltaSeconds;
	if (ProjectileFireElapsedSeconds >= ProjectileFireIntervalSeconds)
	{
		FireRollingBomberProjectileAt(TargetCharacter);
		ProjectileFireElapsedSeconds = 0.0f;
	}

	if (ProjectileModeElapsedSeconds >= ProjectileAttackDurationSeconds)
	{
		EnterFoldingLegsMode();
	}
}

void ATunaSweeperRollingBomber::UpdateProjectileModeMovement(
	float DistanceToTarget,
	const FVector& DirectionToTarget)
{
	if (bProjectileModeClosingDistance)
	{
		if (DistanceToTarget <= ProjectileApproachStopRange)
		{
			bProjectileModeClosingDistance = false;
		}
	}
	else if (DistanceToTarget > ProjectileApproachStartRange)
	{
		bProjectileModeClosingDistance = true;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = ProjectileModeWalkSpeed;
		if (!bProjectileModeClosingDistance)
		{
			MovementComponent->StopMovementImmediately();
		}
	}

	if (bProjectileModeClosingDistance && !DirectionToTarget.IsNearlyZero())
	{
		AddMovementInput(DirectionToTarget, 1.0f, true);
	}
}

void ATunaSweeperRollingBomber::UpdateFoldingLegsMode(
	float DeltaSeconds,
	ATunaSweeperTopDownCharacter* TargetCharacter)
{
	ModeElapsedSeconds += DeltaSeconds;
	LegFoldAlpha = FMath::Clamp(ModeElapsedSeconds / FMath::Max(0.01f, LegFoldDurationSeconds), 0.0f, 1.0f);
	if (LegFoldAlpha >= 1.0f)
	{
		EnterRollingMode(TargetCharacter);
	}
}

void ATunaSweeperRollingBomber::UpdateRollingMode(
	float DeltaSeconds,
	ATunaSweeperTopDownCharacter* TargetCharacter)
{
	ModeElapsedSeconds += DeltaSeconds;
	if (TrySelfDestructFromRollingContact(TargetCharacter))
	{
		return;
	}

	const float RemainingDistance = RollMaxDistance > 0.0f
		? FMath::Max(0.0f, RollMaxDistance - RollDistanceTraveled)
		: TNumericLimits<float>::Max();
	const float RequestedDistance = FMath::Min(RollSpeed * DeltaSeconds, RemainingDistance);
	if (RequestedDistance <= 0.0f || LockedRollDirection.IsNearlyZero())
	{
		EnterRecoveringLegsMode();
		return;
	}

	const FVector PreviousLocation = GetActorLocation();
	FHitResult Hit;
	AddActorWorldOffset(LockedRollDirection * RequestedDistance, true, &Hit);
	const float ActualDistance = FVector::Dist2D(PreviousLocation, GetActorLocation());
	RollDistanceTraveled += ActualDistance;
	ApplyBodyRollVisualRotation(ActualDistance);

	if (Hit.bBlockingHit)
	{
		if (IsActorInRollContact(Hit.GetActor()))
		{
			SelfDestruct();
			return;
		}

		EnterRecoveringLegsMode();
		return;
	}

	if (TrySelfDestructFromRollingContact(TargetCharacter))
	{
		return;
	}

	if (ModeElapsedSeconds >= RollDurationSeconds ||
		(RollMaxDistance > 0.0f && RollDistanceTraveled >= RollMaxDistance))
	{
		EnterRecoveringLegsMode();
	}
}

void ATunaSweeperRollingBomber::UpdateRecoveringLegsMode(float DeltaSeconds)
{
	ModeElapsedSeconds += DeltaSeconds;
	const float UnfoldAlpha = FMath::Clamp(ModeElapsedSeconds / FMath::Max(0.01f, LegUnfoldDurationSeconds), 0.0f, 1.0f);
	LegFoldAlpha = 1.0f - UnfoldAlpha;
	if (LegFoldAlpha <= 0.0f)
	{
		EnterProjectileAttackMode();
	}
}

void ATunaSweeperRollingBomber::UpdateLegIK(float DeltaSeconds, const FVector& PlanarMoveDirection)
{
	if (!bLegIKEnabled)
	{
		UpdateFoldedLegSceneComponents();
		return;
	}

	if (!LeftFootRuntime.bInitialized || !RightFootRuntime.bInitialized)
	{
		InitializeLegIKTargets();
	}

	const FVector SafeMoveDirection = PlanarMoveDirection.GetSafeNormal2D();
	const FVector LeftPlannedLocation = CalculatePlannedFootLocation(LeftFootHomeLocalOffset, SafeMoveDirection);
	const FVector RightPlannedLocation = CalculatePlannedFootLocation(RightFootHomeLocalOffset, SafeMoveDirection);
	LeftFootRuntime.PlannedFootWorldLocation = LeftPlannedLocation;
	RightFootRuntime.PlannedFootWorldLocation = RightPlannedLocation;

	const bool bAnyFootStepping = LeftFootRuntime.bIsStepping || RightFootRuntime.bIsStepping;
	if (!bAnyFootStepping)
	{
		const float TriggerDistanceSq = FMath::Square(FMath::Max(0.0f, FootStepTriggerDistance));
		const bool bLeftNeedsStep =
			FVector::DistSquared2D(LeftFootRuntime.EffectorWorldLocation, LeftPlannedLocation) > TriggerDistanceSq;
		const bool bRightNeedsStep =
			FVector::DistSquared2D(RightFootRuntime.EffectorWorldLocation, RightPlannedLocation) > TriggerDistanceSq;

		if (bLeftNeedsStep || bRightNeedsStep)
		{
			if (bLeftNeedsStep && bRightNeedsStep)
			{
				if (NextStepFoot == ETunaSweeperRollingBomberFoot::Left)
				{
					BeginFootStep(LeftFootRuntime, LeftPlannedLocation);
					NextStepFoot = ETunaSweeperRollingBomberFoot::Right;
				}
				else
				{
					BeginFootStep(RightFootRuntime, RightPlannedLocation);
					NextStepFoot = ETunaSweeperRollingBomberFoot::Left;
				}
			}
			else if (bLeftNeedsStep)
			{
				BeginFootStep(LeftFootRuntime, LeftPlannedLocation);
				NextStepFoot = ETunaSweeperRollingBomberFoot::Right;
			}
			else
			{
				BeginFootStep(RightFootRuntime, RightPlannedLocation);
				NextStepFoot = ETunaSweeperRollingBomberFoot::Left;
			}
		}
	}

	AdvanceFootStep(LeftFootRuntime, DeltaSeconds);
	AdvanceFootStep(RightFootRuntime, DeltaSeconds);
	LeftFootRuntime.JointTargetWorldLocation = CalculateJointTargetLocation(LeftFootRuntime.EffectorWorldLocation, -1.0f);
	RightFootRuntime.JointTargetWorldLocation = CalculateJointTargetLocation(RightFootRuntime.EffectorWorldLocation, 1.0f);
	UpdateFootSceneComponents();
}

void ATunaSweeperRollingBomber::UpdateSpawnerLaunchState(float DeltaSeconds)
{
	if (!bSpawnerLaunchActive)
	{
		return;
	}

	SpawnerLaunchControlRemainingSeconds = FMath::Max(
		0.0f,
		SpawnerLaunchControlRemainingSeconds - DeltaSeconds);

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const bool bIsStillAirborne = MovementComponent && MovementComponent->IsFalling();
	if (SpawnerLaunchControlRemainingSeconds > 0.0f || bIsStillAirborne)
	{
		return;
	}

	bSpawnerLaunchActive = false;
	if (MovementComponent)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = ProjectileModeWalkSpeed;
	}
}

ATunaSweeperTopDownCharacter* ATunaSweeperRollingBomber::ResolvePlayerTarget() const
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerPawn);
	if (!PlayerCharacter || PlayerCharacter->IsDead())
	{
		return nullptr;
	}

	return PlayerCharacter;
}

bool ATunaSweeperRollingBomber::FireRollingBomberProjectileAt(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || !TargetActor)
	{
		return false;
	}

	TSubclassOf<ATunaSweeperProjectile> LoadedProjectileClass = ProjectileClass.LoadSynchronous();
	if (!LoadedProjectileClass)
	{
		LoadedProjectileClass = ATunaSweeperProjectile::StaticClass();
	}

	const FVector ActorLocation = GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	const FVector ToTarget = TargetLocation - (ActorLocation + FVector(0.0f, 0.0f, ProjectileSpawnOffset.Z));
	const FVector FireDirection = ToTarget.GetSafeNormal();
	if (FireDirection.IsNearlyZero())
	{
		return false;
	}

	const FRotator FireRotation = FireDirection.Rotation();
	SetActorRotation(FRotator(0.0f, FireRotation.Yaw, 0.0f));
	const FVector SpawnLocation = ActorLocation + GetActorRotation().RotateVector(ProjectileSpawnOffset);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperProjectile* SpawnedProjectile = World->SpawnActor<ATunaSweeperProjectile>(
		LoadedProjectileClass,
		SpawnLocation,
		FireRotation,
		SpawnParameters);
	if (!SpawnedProjectile)
	{
		return false;
	}

	SpawnedProjectile->SetDamageAmount(ProjectileDamage);
	return true;
}

FVector ATunaSweeperRollingBomber::ResolveRollDirection(
	ATunaSweeperTopDownCharacter* TargetCharacter) const
{
	FVector RollDirection = TargetCharacter
		? TargetCharacter->GetActorLocation() - GetActorLocation()
		: GetActorForwardVector();
	RollDirection.Z = 0.0f;
	RollDirection = RollDirection.GetSafeNormal();
	return RollDirection.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : RollDirection;
}

bool ATunaSweeperRollingBomber::TrySelfDestructFromRollingContact(
	ATunaSweeperTopDownCharacter* TargetCharacter)
{
	if (!TargetCharacter)
	{
		return false;
	}

	if (!IsActorInRollContact(TargetCharacter))
	{
		return false;
	}

	SelfDestruct();
	return true;
}

bool ATunaSweeperRollingBomber::IsActorInRollContact(const AActor* Actor) const
{
	const ACharacter* Character = Cast<ACharacter>(Actor);
	if (!Character)
	{
		return false;
	}

	const UCapsuleComponent* OtherCapsule = Character->GetCapsuleComponent();
	const float OtherRadius = OtherCapsule ? OtherCapsule->GetScaledCapsuleRadius() : 0.0f;
	const float OtherHalfHeight = OtherCapsule ? OtherCapsule->GetScaledCapsuleHalfHeight() : 0.0f;
	const float CombinedRadius = FMath::Max(0.0f, RollContactRadius) + OtherRadius;
	const float MaxVerticalDistance = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherHalfHeight;

	const FVector ToActor = Actor->GetActorLocation() - GetActorLocation();
	return FMath::Abs(ToActor.Z) <= MaxVerticalDistance &&
		FVector::DistSquared2D(Actor->GetActorLocation(), GetActorLocation()) <= FMath::Square(CombinedRadius);
}

void ATunaSweeperRollingBomber::ApplyExplosionDamage()
{
	UWorld* World = GetWorld();
	if (!World || ExplosionDamage <= 0.0f || ExplosionRadius <= 0.0f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperRollingBomberExplosionOverlap), false, this);
	QueryParams.AddIgnoredActor(this);
	if (!World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ExplosionRadius),
		QueryParams))
	{
		return;
	}

	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* DamagedActor = Overlap.GetActor();
		if (!IsValid(DamagedActor) || DamagedActor == this || DamagedActors.Contains(DamagedActor))
		{
			continue;
		}

		DamagedActors.Add(DamagedActor);
		const TSubclassOf<UDamageType> DamageTypeClass =
			ExplosionDamageType ? ExplosionDamageType : TSubclassOf<UDamageType>(UDamageType::StaticClass());
		UGameplayStatics::ApplyDamage(
			DamagedActor,
			ExplosionDamage,
			GetController(),
			this,
			DamageTypeClass);
	}
}

void ATunaSweeperRollingBomber::ApplyBodyRollVisualRotation(float DeltaDistance)
{
	if (!bUseBodyRollVisualRotation || !VisualMesh || BodyVisualRadiusCm <= 0.0f || DeltaDistance <= 0.0f)
	{
		return;
	}

	const float Circumference = 2.0f * UE_PI * BodyVisualRadiusCm;
	BodyRollDegrees += (DeltaDistance / Circumference) * 360.0f;
	VisualMesh->SetRelativeRotation(BodyVisualBaseRelativeRotation + FRotator(BodyRollDegrees, 0.0f, 0.0f));
}

void ATunaSweeperRollingBomber::ResetBodyRollVisualRotation()
{
	BodyRollDegrees = 0.0f;
	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(BodyVisualBaseRelativeRotation);
	}
}

void ATunaSweeperRollingBomber::ApplyEyeVisualDefaults()
{
	if (EyeMesh)
	{
		EyeMesh->SetRelativeLocation(EyeLocalOffset);
		EyeMesh->SetRelativeScale3D(EyeLocalScale);
		EyeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EyeMesh->SetCastShadow(false);

		if (UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TunaSweeperRollingBomber::SphereMeshPath))
		{
			EyeMesh->SetStaticMesh(SphereMesh);
		}

		UMaterialInterface* LoadedEyeMaterial = EyeMaterial.LoadSynchronous();
		if (!LoadedEyeMaterial)
		{
			LoadedEyeMaterial = EyeMesh->GetMaterial(0);
		}

		if (LoadedEyeMaterial)
		{
			EyeDynamicMaterial = UMaterialInstanceDynamic::Create(LoadedEyeMaterial, this);
			EyeMesh->SetMaterial(0, EyeDynamicMaterial);
		}
	}

	if (EyeLight)
	{
		EyeLight->SetRelativeLocation(EyeLocalOffset + FVector(18.0f, 0.0f, 0.0f));
		EyeLight->SetCastShadows(false);
		EyeLight->SetVisibility(true);
	}

	ApplyEyeMaterialState(CurrentEyeColor, CurrentEyeEmissiveStrength);
}

void ATunaSweeperRollingBomber::SetEyeChargeWarningActive(bool bActive, bool bInstant)
{
	bEyeChargeWarningActive = bActive;
	if (!bInstant)
	{
		return;
	}

	CurrentEyeColor = bEyeChargeWarningActive ? ChargeWarningEyeColor : NormalEyeColor;
	CurrentEyeEmissiveStrength = bEyeChargeWarningActive
		? ChargeWarningEyeEmissiveStrength
		: NormalEyeEmissiveStrength;
	ApplyEyeMaterialState(CurrentEyeColor, CurrentEyeEmissiveStrength);
}

void ATunaSweeperRollingBomber::UpdateEyeVisualState(float DeltaSeconds)
{
	const FLinearColor TargetColor = bEyeChargeWarningActive ? ChargeWarningEyeColor : NormalEyeColor;
	const float TargetStrength = bEyeChargeWarningActive
		? ChargeWarningEyeEmissiveStrength
		: NormalEyeEmissiveStrength;

	const float InterpAlpha = EyeWarningInterpSpeed <= 0.0f
		? 1.0f
		: FMath::Clamp(DeltaSeconds * EyeWarningInterpSpeed, 0.0f, 1.0f);
	CurrentEyeColor = FLinearColor(
		FMath::Lerp(CurrentEyeColor.R, TargetColor.R, InterpAlpha),
		FMath::Lerp(CurrentEyeColor.G, TargetColor.G, InterpAlpha),
		FMath::Lerp(CurrentEyeColor.B, TargetColor.B, InterpAlpha),
		FMath::Lerp(CurrentEyeColor.A, TargetColor.A, InterpAlpha));
	CurrentEyeEmissiveStrength = FMath::Lerp(CurrentEyeEmissiveStrength, TargetStrength, InterpAlpha);

	ApplyEyeMaterialState(CurrentEyeColor, CurrentEyeEmissiveStrength);
}

void ATunaSweeperRollingBomber::ApplyEyeMaterialState(
	const FLinearColor& EyeColor,
	float EmissiveStrength)
{
	const FLinearColor EmissiveColor = EyeColor * FMath::Max(0.0f, EmissiveStrength);
	if (EyeDynamicMaterial)
	{
		EyeDynamicMaterial->SetVectorParameterValue(TEXT("Color"), EyeColor);
		EyeDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), EyeColor);
		EyeDynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), EyeColor);
		EyeDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
		EyeDynamicMaterial->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
		EyeDynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
		EyeDynamicMaterial->SetScalarParameterValue(TEXT("Emissive Strength"), EmissiveStrength);
		EyeDynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), EmissiveStrength);
	}

	if (EyeLight)
	{
		const float ChargeAlpha = CalculateEyeChargeAlpha(EmissiveStrength);
		EyeLight->SetLightColor(EyeColor);
		EyeLight->SetIntensity(FMath::Lerp(NormalEyeLightIntensity, ChargeWarningEyeLightIntensity, ChargeAlpha));
		EyeLight->SetAttenuationRadius(FMath::Lerp(NormalEyeLightRadius, ChargeWarningEyeLightRadius, ChargeAlpha));
	}
}

float ATunaSweeperRollingBomber::CalculateEyeChargeAlpha(float EmissiveStrength) const
{
	const float MinStrength = FMath::Min(NormalEyeEmissiveStrength, ChargeWarningEyeEmissiveStrength);
	const float MaxStrength = FMath::Max(NormalEyeEmissiveStrength, ChargeWarningEyeEmissiveStrength);
	if (FMath::IsNearlyEqual(MinStrength, MaxStrength))
	{
		return bEyeChargeWarningActive ? 1.0f : 0.0f;
	}

	return FMath::Clamp((EmissiveStrength - MinStrength) / (MaxStrength - MinStrength), 0.0f, 1.0f);
}

void ATunaSweeperRollingBomber::InitializeLegIKTargets()
{
	InitializeFootRuntime(LeftFootRuntime, LeftFootHomeLocalOffset);
	InitializeFootRuntime(RightFootRuntime, RightFootHomeLocalOffset);
	UpdateFootSceneComponents();
}

void ATunaSweeperRollingBomber::InitializeFootRuntime(
	FFootRuntime& FootRuntime,
	const FVector& FootHomeLocalOffset)
{
	FootRuntime.EffectorWorldLocation = CalculatePlannedFootLocation(FootHomeLocalOffset, FVector::ZeroVector);
	FootRuntime.PlannedFootWorldLocation = FootRuntime.EffectorWorldLocation;
	FootRuntime.StepStartWorldLocation = FootRuntime.EffectorWorldLocation;
	FootRuntime.JointTargetWorldLocation = CalculateJointTargetLocation(
		FootRuntime.EffectorWorldLocation,
		FootHomeLocalOffset.Y >= 0.0f ? 1.0f : -1.0f);
	FootRuntime.StepElapsedSeconds = 0.0f;
	FootRuntime.StepAlpha = 0.0f;
	FootRuntime.bIsStepping = false;
	FootRuntime.bInitialized = true;
}

void ATunaSweeperRollingBomber::BeginFootStep(
	FFootRuntime& FootRuntime,
	const FVector& PlannedFootWorldLocation)
{
	FootRuntime.StepStartWorldLocation = FootRuntime.EffectorWorldLocation;
	FootRuntime.PlannedFootWorldLocation = PlannedFootWorldLocation;
	FootRuntime.StepElapsedSeconds = 0.0f;
	FootRuntime.StepAlpha = 0.0f;
	FootRuntime.bIsStepping = true;
}

void ATunaSweeperRollingBomber::AdvanceFootStep(FFootRuntime& FootRuntime, float DeltaSeconds)
{
	if (!FootRuntime.bIsStepping)
	{
		FootRuntime.StepAlpha = 0.0f;
		return;
	}

	FootRuntime.StepElapsedSeconds += DeltaSeconds;
	const float RawAlpha = FMath::Clamp(
		FootRuntime.StepElapsedSeconds / FMath::Max(0.01f, FootStepDurationSeconds),
		0.0f,
		1.0f);
	const float SmoothAlpha = RawAlpha * RawAlpha * (3.0f - 2.0f * RawAlpha);
	FootRuntime.StepAlpha = RawAlpha;
	FootRuntime.EffectorWorldLocation = FMath::Lerp(
		FootRuntime.StepStartWorldLocation,
		FootRuntime.PlannedFootWorldLocation,
		SmoothAlpha);
	FootRuntime.EffectorWorldLocation.Z += FMath::Sin(RawAlpha * UE_PI) * FootStepHeight;

	if (RawAlpha >= 1.0f)
	{
		FootRuntime.EffectorWorldLocation = FootRuntime.PlannedFootWorldLocation;
		FootRuntime.StepAlpha = 0.0f;
		FootRuntime.bIsStepping = false;
	}
}

FVector ATunaSweeperRollingBomber::CalculatePlannedFootLocation(
	const FVector& FootHomeLocalOffset,
	const FVector& PlanarMoveDirection) const
{
	FVector PlannedLocalOffset = FootHomeLocalOffset;
	FVector PlannedWorldLocation = GetActorTransform().TransformPosition(PlannedLocalOffset);
	if (!PlanarMoveDirection.IsNearlyZero())
	{
		PlannedWorldLocation += PlanarMoveDirection.GetSafeNormal2D() * FootMoveLeadDistance;
	}

	return ResolveGroundedFootLocation(PlannedWorldLocation);
}

FVector ATunaSweeperRollingBomber::CalculateJointTargetLocation(
	const FVector& FootWorldLocation,
	float SideSign) const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	return FootWorldLocation +
		Forward * KneeForwardOffset +
		Right * (SideSign * KneeSideOffset) +
		FVector(0.0f, 0.0f, KneeHeightOffset);
}

FVector ATunaSweeperRollingBomber::ResolveGroundedFootLocation(
	const FVector& DesiredWorldLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return DesiredWorldLocation;
	}

	const FVector TraceStart(
		DesiredWorldLocation.X,
		DesiredWorldLocation.Y,
		GetActorLocation().Z + FootGroundTraceUp);
	const FVector TraceEnd(
		DesiredWorldLocation.X,
		DesiredWorldLocation.Y,
		GetActorLocation().Z - FootGroundTraceDown);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperRollingBomberFootGroundTrace), false, this);
	QueryParams.AddIgnoredActor(this);

	FHitResult GroundHit;
	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) &&
		GroundHit.bBlockingHit &&
		GroundHit.ImpactNormal.Z >= TunaSweeperRollingBomber::MinFootGroundNormalZ)
	{
		return GroundHit.ImpactPoint + FVector(0.0f, 0.0f, FootGroundClearance);
	}

	return DesiredWorldLocation;
}

void ATunaSweeperRollingBomber::UpdateFootSceneComponents()
{
	if (LeftFootIKTarget)
	{
		LeftFootIKTarget->SetWorldLocation(LeftFootRuntime.EffectorWorldLocation);
	}
	if (RightFootIKTarget)
	{
		RightFootIKTarget->SetWorldLocation(RightFootRuntime.EffectorWorldLocation);
	}
	if (LeftKneeIKTarget)
	{
		LeftKneeIKTarget->SetWorldLocation(LeftFootRuntime.JointTargetWorldLocation);
	}
	if (RightKneeIKTarget)
	{
		RightKneeIKTarget->SetWorldLocation(RightFootRuntime.JointTargetWorldLocation);
	}
}

void ATunaSweeperRollingBomber::UpdateFoldedLegSceneComponents()
{
	const FTransform ActorTransform = GetActorTransform();
	const FVector LeftFoldedFootLocation = ActorTransform.TransformPosition(FVector(8.0f, -17.0f, -8.0f));
	const FVector RightFoldedFootLocation = ActorTransform.TransformPosition(FVector(8.0f, 17.0f, -8.0f));
	const FVector LeftFoldedKneeLocation = ActorTransform.TransformPosition(FVector(0.0f, -24.0f, 10.0f));
	const FVector RightFoldedKneeLocation = ActorTransform.TransformPosition(FVector(0.0f, 24.0f, 10.0f));

	LeftFootRuntime.EffectorWorldLocation = LeftFoldedFootLocation;
	LeftFootRuntime.PlannedFootWorldLocation = LeftFoldedFootLocation;
	LeftFootRuntime.JointTargetWorldLocation = LeftFoldedKneeLocation;
	LeftFootRuntime.bIsStepping = false;
	LeftFootRuntime.StepAlpha = 0.0f;
	LeftFootRuntime.bInitialized = true;

	RightFootRuntime.EffectorWorldLocation = RightFoldedFootLocation;
	RightFootRuntime.PlannedFootWorldLocation = RightFoldedFootLocation;
	RightFootRuntime.JointTargetWorldLocation = RightFoldedKneeLocation;
	RightFootRuntime.bIsStepping = false;
	RightFootRuntime.StepAlpha = 0.0f;
	RightFootRuntime.bInitialized = true;

	UpdateFootSceneComponents();
}

FTunaSweeperRollingBomberFootIKState ATunaSweeperRollingBomber::BuildFootIKState(
	const FFootRuntime& FootRuntime) const
{
	FTunaSweeperRollingBomberFootIKState IKState;
	IKState.EffectorWorldLocation = FootRuntime.EffectorWorldLocation;
	IKState.JointTargetWorldLocation = FootRuntime.JointTargetWorldLocation;
	IKState.PlannedFootWorldLocation = FootRuntime.PlannedFootWorldLocation;
	IKState.bIsStepping = FootRuntime.bIsStepping;
	IKState.StepAlpha = FootRuntime.StepAlpha;
	return IKState;
}
