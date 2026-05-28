#include "AI/TunaSweeperRollingBomber.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperMeleeImpactBurstActor.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TunaSweeperCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperRollingBomber
{
	const TCHAR* SphereMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* CylinderMeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* BodyMaterialPath = TEXT("/Game/Characters/Enemy/M_RollingBomberBodyGray.M_RollingBomberBodyGray");
	const TCHAR* LegMetalMaterialPath = TEXT("/Game/Characters/Enemy/M_RollingBomberLegMetal.M_RollingBomberLegMetal");
	const TCHAR* LegMetalFallbackMaterialPath = TEXT("/Game/Interaction/M_Container_Metal.M_Container_Metal");
	const TCHAR* RollChargeCylinderMeshPath = TEXT("/Game/Effects/SM_RollingBomberChargeCylinder_Open.SM_RollingBomberChargeCylinder_Open");
	const TCHAR* RollChargeCylinderMaterialPath = TEXT("/Game/Effects/M_RollingBomberChargeCylinder.M_RollingBomberChargeCylinder");
	constexpr float MinFootGroundNormalZ = 0.25f;
	constexpr float SpawnGroundTraceExtraDistance = 9.0f;
	constexpr float SpawnPhysicsLinearDamping = 0.18f;
	constexpr float SpawnPhysicsAngularDamping = 0.35f;
}

ATunaSweeperRollingBomber::ATunaSweeperRollingBomber()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = nullptr;
	AutoPossessAI = EAutoPossessAI::Disabled;

	GetCapsuleComponent()->InitCapsuleSize(21.0f, 28.0f);
	GetCharacterMovement()->MaxWalkSpeed = ProjectileModeWalkSpeed;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);

	ProjectileHurtbox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ProjectileHurtbox"));
	ProjectileHurtbox->SetupAttachment(RootComponent);
	ProjectileHurtbox->InitCapsuleSize(ProjectileHurtboxRadiusCm, ProjectileHurtboxHalfHeightCm);
	ProjectileHurtbox->SetRelativeLocation(ProjectileHurtboxLocalOffset);
	ProjectileHurtbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileHurtbox->SetCollisionObjectType(ECC_Pawn);
	ProjectileHurtbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileHurtbox->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	ProjectileHurtbox->SetGenerateOverlapEvents(false);
	ProjectileHurtbox->SetCanEverAffectNavigation(false);

	MaxHealth = 20.0f;
	ExperienceValue = 35;
	MovementSpeed = ProjectileModeWalkSpeed;
	MovementSpeedRandomOffset = FVector2D::ZeroVector;
	ProjectileSpawnOffset = FVector(29.0f, 0.0f, 21.0f);
	ProjectileDamage = 1.0f;
	ExplosionDamageType = UDamageType::StaticClass();
	BodyMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TunaSweeperRollingBomber::BodyMaterialPath));
	EyeMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	LegMetalMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TunaSweeperRollingBomber::LegMetalMaterialPath));
	RollChargeCylinderMeshAsset = TSoftObjectPtr<UStaticMesh>(
		FSoftObjectPath(TunaSweeperRollingBomber::RollChargeCylinderMeshPath));
	RollChargeCylinderMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TunaSweeperRollingBomber::RollChargeCylinderMaterialPath));
	RollingBomberProjectileMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive")));
	RollingBomberProjectileTrailMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Effects/M_LumberjackMeleeSwingArc.M_LumberjackMeleeSwingArc")));
	SelfDestructBurstActorClass = TSoftClassPtr<ATunaSweeperMeleeImpactBurstActor>(
		FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperMeleeImpactBurstActor")));

	BodyVisualPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyVisualPivot"));
	BodyVisualPivot->SetupAttachment(RootComponent);

	RollChargeCylinderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RollChargeCylinderMesh"));
	RollChargeCylinderMesh->SetupAttachment(RootComponent);
	RollChargeCylinderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RollChargeCylinderMesh->SetGenerateOverlapEvents(false);
	RollChargeCylinderMesh->SetCastShadow(false);
	RollChargeCylinderMesh->SetVisibility(false);
	RollChargeCylinderMesh->SetHiddenInGame(true);

	if (VisualMesh)
	{
		VisualMesh->SetupAttachment(BodyVisualPivot);
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.4f));
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
	LeftKneeIKTarget->SetRelativeLocation(FVector(6.0f, -21.0f, -3.0f));

	RightKneeIKTarget = CreateDefaultSubobject<USceneComponent>(TEXT("RightKneeIKTarget"));
	RightKneeIKTarget->SetupAttachment(RootComponent);
	RightKneeIKTarget->SetRelativeLocation(FVector(6.0f, 21.0f, -3.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> LegSegmentMesh(TunaSweeperRollingBomber::CylinderMeshPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FootPadMesh(TunaSweeperRollingBomber::CubeMeshPath);
	auto ConfigureLegMesh = [this](UStaticMeshComponent* MeshComponent, bool bUseFootPad)
	{
		if (!MeshComponent)
		{
			return;
		}

		MeshComponent->SetupAttachment(RootComponent);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetGenerateOverlapEvents(false);
		MeshComponent->SetRelativeScale3D(FVector(0.03f));
		if (bUseFootPad)
		{
			if (FootPadMesh.Succeeded())
			{
				MeshComponent->SetStaticMesh(FootPadMesh.Object);
			}
		}
		else if (LegSegmentMesh.Succeeded())
		{
			MeshComponent->SetStaticMesh(LegSegmentMesh.Object);
		}
	};

	LeftUpperLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftUpperLegMesh"));
	ConfigureLegMesh(LeftUpperLegMesh, false);
	LeftLowerLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLowerLegMesh"));
	ConfigureLegMesh(LeftLowerLegMesh, false);
	LeftFootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFootMesh"));
	ConfigureLegMesh(LeftFootMesh, true);
	RightUpperLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightUpperLegMesh"));
	ConfigureLegMesh(RightUpperLegMesh, false);
	RightLowerLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLowerLegMesh"));
	ConfigureLegMesh(RightLowerLegMesh, false);
	RightFootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFootMesh"));
	ConfigureLegMesh(RightFootMesh, true);

	EyeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeMesh"));
	EyeMesh->SetupAttachment(BodyVisualPivot);
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
	EyeLight->SetupAttachment(BodyVisualPivot);
	EyeLight->SetRelativeLocation(EyeLocalOffset + FVector(9.0f, 0.0f, 0.0f));
	EyeLight->SetCastShadows(false);
	EyeLight->SetLightColor(NormalEyeColor);
	EyeLight->SetIntensity(NormalEyeLightIntensity);
	EyeLight->SetAttenuationRadius(NormalEyeLightRadius);

	SpawnBouncePhysicalMaterial = CreateDefaultSubobject<UPhysicalMaterial>(TEXT("SpawnBouncePhysicalMaterial"));
	if (SpawnBouncePhysicalMaterial)
	{
		SpawnBouncePhysicalMaterial->Friction = SpawnBounceFriction;
		SpawnBouncePhysicalMaterial->Restitution = SpawnBounceRestitution;
	}
}

void ATunaSweeperRollingBomber::BeginPlay()
{
	Super::BeginPlay();

	ApplyRollingBomberVisualDefaults();
	ApplyRollChargeCylinderVisualDefaults();
	ApplyLegVisualMaterial();
	ApplyEyeVisualDefaults();
	SetEyeChargeWarningActive(false, true);
	LastActorLocation = GetActorLocation();
	ProjectileFireElapsedSeconds = 0.0f;
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

	ATunaSweeperTopDownCharacter* TargetCharacter = ResolvePlayerTarget();

	switch (CurrentMode)
	{
	case ETunaSweeperRollingBomberMode::SpawnPhysics:
		UpdateSpawnPhysicsMode(DeltaSeconds);
		break;
	case ETunaSweeperRollingBomberMode::StandingUpFromSpawn:
		UpdateStandingUpFromSpawnMode(DeltaSeconds);
		break;
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
	else if (CurrentMode == ETunaSweeperRollingBomberMode::StandingUpFromSpawn && bLegIKEnabled)
	{
		UpdateLegIK(DeltaSeconds, FVector::ZeroVector);
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

	EnterSpawnPhysicsMode(LaunchVelocity);
}

FTunaSweeperRollingBomberFootIKState ATunaSweeperRollingBomber::GetFootIKState(
	ETunaSweeperRollingBomberFoot Foot) const
{
	return Foot == ETunaSweeperRollingBomberFoot::Left
		? BuildFootIKState(LeftFootRuntime)
		: BuildFootIKState(RightFootRuntime);
}

FVector ATunaSweeperRollingBomber::ResolveProjectileHitEffectLocation(const FHitResult& Hit) const
{
	FVector EffectLocation = Hit.ImpactPoint;
	if (!VisualMesh)
	{
		return EffectLocation;
	}

	const FBoxSphereBounds VisualBounds = VisualMesh->Bounds;
	const FVector VisualCenter = VisualBounds.Origin;
	const float MaxPlanarDistance = FMath::Max(1.0f, BodyVisualRadiusCm * 0.9f);
	FVector PlanarOffset = EffectLocation - VisualCenter;
	PlanarOffset.Z = 0.0f;
	if (PlanarOffset.SizeSquared() > FMath::Square(MaxPlanarDistance))
	{
		PlanarOffset = PlanarOffset.GetSafeNormal2D() * MaxPlanarDistance;
		EffectLocation.X = VisualCenter.X + PlanarOffset.X;
		EffectLocation.Y = VisualCenter.Y + PlanarOffset.Y;
	}

	EffectLocation.Z = VisualCenter.Z;
	return EffectLocation;
}

void ATunaSweeperRollingBomber::ApplyRollingBomberVisualDefaults()
{
	if (BodyVisualPivot)
	{
		BodyVisualPivot->SetRelativeLocation(FVector::ZeroVector);
		BodyVisualPivot->SetRelativeRotation(FRotator::ZeroRotator);
		BodyVisualPivotBaseRelativeLocation = BodyVisualPivot->GetRelativeLocation();
		BodyVisualPivotBaseRelativeRotation = BodyVisualPivot->GetRelativeRotation();
	}

	if (VisualMesh)
	{
		if (UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TunaSweeperRollingBomber::SphereMeshPath))
		{
			VisualMesh->SetStaticMesh(SphereMesh);
		}

		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.4f));
		VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UMaterialInterface* LoadedBodyMaterial = BodyMaterial.LoadSynchronous())
		{
			VisualMesh->SetMaterial(0, LoadedBodyMaterial);
		}
		BodyVisualBaseRelativeRotation = VisualMesh->GetRelativeRotation();
	}

	if (ForwardMarkerMesh)
	{
		ForwardMarkerMesh->SetVisibility(false);
		ForwardMarkerMesh->SetHiddenInGame(true);
		ForwardMarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileHurtbox)
	{
		ProjectileHurtbox->SetCapsuleSize(
			FMath::Max(0.0f, ProjectileHurtboxRadiusCm),
			FMath::Max(0.0f, ProjectileHurtboxHalfHeightCm),
			true);
		ProjectileHurtbox->SetRelativeLocation(ProjectileHurtboxLocalOffset);
		ProjectileHurtbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ProjectileHurtbox->SetCollisionResponseToAllChannels(ECR_Ignore);
		ProjectileHurtbox->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	}
}

void ATunaSweeperRollingBomber::ApplyRollChargeCylinderVisualDefaults()
{
	if (!RollChargeCylinderMesh)
	{
		return;
	}

	RollChargeCylinderMesh->SetRelativeLocation(RollChargeCylinderLocalOffset);
	RollChargeCylinderMesh->SetRelativeRotation(FRotator::ZeroRotator);
	RollChargeCylinderMesh->SetRelativeScale3D(RollChargeCylinderLocalScale);
	RollChargeCylinderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RollChargeCylinderMesh->SetGenerateOverlapEvents(false);
	RollChargeCylinderMesh->SetCastShadow(false);

	if (UStaticMesh* ChargeMesh = RollChargeCylinderMeshAsset.LoadSynchronous())
	{
		RollChargeCylinderMesh->SetStaticMesh(ChargeMesh);
	}

	UMaterialInterface* ChargeMaterial = RollChargeCylinderMaterial.LoadSynchronous();
	if (ChargeMaterial)
	{
		RollChargeCylinderDynamicMaterial = UMaterialInstanceDynamic::Create(ChargeMaterial, this);
		RollChargeCylinderMesh->SetMaterial(0, RollChargeCylinderDynamicMaterial);
	}

	UpdateRollChargeCylinderEffect(0.0f);
	SetRollChargeCylinderEffectActive(false);
}

void ATunaSweeperRollingBomber::EnterSpawnPhysicsMode(const FVector& LaunchVelocity)
{
	CurrentMode = ETunaSweeperRollingBomberMode::SpawnPhysics;
	ProjectileModeElapsedSeconds = 0.0f;
	ProjectileFireElapsedSeconds = 0.0f;
	ModeElapsedSeconds = 0.0f;
	SpawnerLaunchControlRemainingSeconds = FMath::Max(0.0f, SpawnerLaunchControlGraceSeconds);
	SpawnPhysicsElapsedSeconds = 0.0f;
	SpawnStandUpAlpha = 0.0f;
	LegFoldAlpha = 1.0f;
	bLegIKEnabled = false;
	bProjectileModeClosingDistance = false;
	RollDistanceTraveled = 0.0f;
	BodyRollDegrees = 0.0f;

	SetEyeChargeWarningActive(false, false);
	SetRollChargeCylinderEffectActive(false);
	ResetBodyRollVisualRotation();
	UpdateFoldedLegSceneComponents();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		EnterStandingUpFromSpawnMode();
		return;
	}

	if (SpawnBouncePhysicalMaterial)
	{
		SpawnBouncePhysicalMaterial->Friction = FMath::Max(0.0f, SpawnBounceFriction);
		SpawnBouncePhysicalMaterial->Restitution = FMath::Clamp(SpawnBounceRestitution, 0.0f, 1.0f);
		Capsule->SetPhysMaterialOverride(SpawnBouncePhysicalMaterial);
	}

	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionObjectType(ECC_PhysicsBody);
	Capsule->SetSimulatePhysics(true);
	Capsule->SetEnableGravity(true);
	Capsule->SetLinearDamping(TunaSweeperRollingBomber::SpawnPhysicsLinearDamping);
	Capsule->SetAngularDamping(TunaSweeperRollingBomber::SpawnPhysicsAngularDamping);
	Capsule->SetPhysicsLinearVelocity(LaunchVelocity, false);

	FVector AngularAxis = FVector::CrossProduct(LaunchVelocity.GetSafeNormal(), FVector::UpVector).GetSafeNormal();
	if (AngularAxis.IsNearlyZero())
	{
		AngularAxis = GetActorRightVector().GetSafeNormal();
	}
	Capsule->SetPhysicsAngularVelocityInDegrees(
		AngularAxis * FMath::Max(0.0f, SpawnPhysicsAngularVelocityDegrees),
		false);
	Capsule->WakeRigidBody();
	bSpawnPhysicsSimulationActive = true;
}

void ATunaSweeperRollingBomber::EnterStandingUpFromSpawnMode()
{
	CurrentMode = ETunaSweeperRollingBomberMode::StandingUpFromSpawn;
	ModeElapsedSeconds = 0.0f;
	SpawnStandUpAlpha = 0.0f;
	LegFoldAlpha = 1.0f;
	bLegIKEnabled = false;
	bProjectileModeClosingDistance = false;

	SetEyeChargeWarningActive(false, false);
	SetRollChargeCylinderEffectActive(false);
	ResetBodyRollVisualRotation();
	UpdateFoldedLegSceneComponents();
}

void ATunaSweeperRollingBomber::EnterProjectileAttackMode()
{
	if (bSpawnPhysicsSimulationActive)
	{
		FinishSpawnPhysicsSimulation();
	}

	CurrentMode = ETunaSweeperRollingBomberMode::ProjectileAttack;
	ProjectileModeElapsedSeconds = 0.0f;
	ResetProjectileFireTimer(true);
	ModeElapsedSeconds = 0.0f;
	LegFoldAlpha = 0.0f;
	SpawnStandUpAlpha = 1.0f;
	bLegIKEnabled = true;
	bProjectileModeClosingDistance = false;
	ProjectileOrbitDirectionSign = FMath::RandBool() ? 1.0f : -1.0f;
	RollDistanceTraveled = 0.0f;

	SetEyeChargeWarningActive(false, false);
	SetRollChargeCylinderEffectActive(false);
	ResetBodyRollVisualRotation();
	InitializeLegIKTargets();

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = ProjectileModeWalkSpeed;
	}
}

void ATunaSweeperRollingBomber::EnterFoldingLegsMode(ATunaSweeperTopDownCharacter* TargetCharacter)
{
	CurrentMode = ETunaSweeperRollingBomberMode::FoldingLegs;
	ModeElapsedSeconds = 0.0f;
	LegFoldAlpha = 0.0f;
	bLegIKEnabled = false;
	bProjectileModeClosingDistance = false;
	BodyRollDegrees = 0.0f;
	LockedRollDirection = ResolveRollDirection(TargetCharacter);

	SetEyeChargeWarningActive(true, false);
	SetRollChargeCylinderEffectActive(true);
	UpdateRollChargeCylinderEffect(0.0f);
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

void ATunaSweeperRollingBomber::EnterRollingMode(ATunaSweeperTopDownCharacter* TargetCharacter)
{
	CurrentMode = ETunaSweeperRollingBomberMode::Rolling;
	ModeElapsedSeconds = 0.0f;
	LegFoldAlpha = 1.0f;
	bLegIKEnabled = false;
	RollDistanceTraveled = 0.0f;
	if (LockedRollDirection.IsNearlyZero())
	{
		LockedRollDirection = ResolveRollDirection(TargetCharacter);
	}
	else
	{
		LockedRollDirection = LockedRollDirection.GetSafeNormal2D();
	}

	SetEyeChargeWarningActive(true, false);
	SetRollChargeCylinderEffectActive(false);
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
	SetRollChargeCylinderEffectActive(false);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (BodyVisualPivot)
	{
		BodyVisualPivot->SetRelativeLocation(BodyVisualPivotBaseRelativeLocation);
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

	if (bSpawnPhysicsSimulationActive)
	{
		FinishSpawnPhysicsSimulation();
	}

	SetEyeChargeWarningActive(true, true);
	SetRollChargeCylinderEffectActive(false);
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	SetActorEnableCollision(false);
	SpawnSelfDestructBurst();
	ApplyExplosionDamage();
	Destroy();
}

void ATunaSweeperRollingBomber::UpdateProjectileAttackMode(
	float DeltaSeconds,
	ATunaSweeperTopDownCharacter* TargetCharacter)
{
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
		ProjectileFireElapsedSeconds = 0.0f;
		return;
	}

	ProjectileModeElapsedSeconds += DeltaSeconds;
	ProjectileFireElapsedSeconds += DeltaSeconds;
	if (ProjectileFireElapsedSeconds >= CurrentProjectileFireIntervalSeconds)
	{
		FireRollingBomberProjectileAt(TargetCharacter);
		ResetProjectileFireTimer(false);
	}

	if (ProjectileModeElapsedSeconds >= ProjectileAttackDurationSeconds)
	{
		EnterFoldingLegsMode(TargetCharacter);
	}
}

void ATunaSweeperRollingBomber::UpdateProjectileModeMovement(
	float DistanceToTarget,
	const FVector& DirectionToTarget)
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = ProjectileModeWalkSpeed;
	}

	if (DirectionToTarget.IsNearlyZero())
	{
		bProjectileModeClosingDistance = false;
		return;
	}

	const FVector StrafeDirection =
		FVector::CrossProduct(FVector::UpVector, DirectionToTarget).GetSafeNormal() * ProjectileOrbitDirectionSign;
	const float PreferredRange = FMath::Max(0.0f, ProjectileOrbitPreferredRange);
	const float MinimumRange = FMath::Clamp(ProjectileOrbitMinimumRange, 0.0f, PreferredRange);
	const float ApproachRange = FMath::Max(1.0f, ProjectileAttackRange - PreferredRange);

	float ApproachWeight = 0.0f;
	if (DistanceToTarget <= MinimumRange)
	{
		ApproachWeight = -FMath::Max(0.0f, ProjectileOrbitRetreatWeight);
	}
	else if (DistanceToTarget <= PreferredRange)
	{
		ApproachWeight = FMath::Max(0.0f, ProjectileOrbitCloseApproachWeight);
	}
	else
	{
		const float ApproachAlpha = FMath::Clamp((DistanceToTarget - PreferredRange) / ApproachRange, 0.0f, 1.0f);
		ApproachWeight = FMath::Lerp(
			FMath::Max(0.0f, ProjectileOrbitCloseApproachWeight),
			FMath::Max(0.0f, ProjectileOrbitApproachWeight),
			ApproachAlpha);
	}

	const FVector MoveDirection = (
		StrafeDirection * FMath::Max(0.0f, ProjectileOrbitStrafeWeight) +
		DirectionToTarget * ApproachWeight).GetSafeNormal();
	bProjectileModeClosingDistance = ApproachWeight > 0.0f;

	if (!MoveDirection.IsNearlyZero())
	{
		AddMovementInput(MoveDirection, 1.0f, true);
	}
}

void ATunaSweeperRollingBomber::UpdateFoldingLegsMode(
	float DeltaSeconds,
	ATunaSweeperTopDownCharacter* TargetCharacter)
{
	ModeElapsedSeconds += DeltaSeconds;
	const float ChargeAlpha = FMath::Clamp(ModeElapsedSeconds / FMath::Max(0.01f, LegFoldDurationSeconds), 0.0f, 1.0f);
	LegFoldAlpha = ChargeAlpha;
	UpdateRollChargeCylinderEffect(ChargeAlpha);

	const float SpinAlpha = ChargeAlpha * ChargeAlpha;
	const float SpinSpeedDegreesPerSecond = FMath::Lerp(
		FMath::Max(0.0f, RollChargeSpinStartDegreesPerSecond),
		FMath::Max(0.0f, RollChargeSpinEndDegreesPerSecond),
		SpinAlpha);
	ApplyBodyRollVisualRotationDegrees(SpinSpeedDegreesPerSecond * DeltaSeconds);

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
	ApplyRollLaunchVisualHop();

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
	const FTransform ActorTransform = GetActorTransform();
	const FVector LeftHipWorldLocation = ActorTransform.TransformPosition(LeftHipLocalOffset);
	const FVector RightHipWorldLocation = ActorTransform.TransformPosition(RightHipLocalOffset);
	LeftFootRuntime.EffectorWorldLocation = ClampFootLocationToLegReach(
		LeftHipWorldLocation,
		LeftFootRuntime.EffectorWorldLocation);
	RightFootRuntime.EffectorWorldLocation = ClampFootLocationToLegReach(
		RightHipWorldLocation,
		RightFootRuntime.EffectorWorldLocation);
	const FVector LeftPlannedLocation = ClampFootLocationToLegReach(
		LeftHipWorldLocation,
		CalculatePlannedFootLocation(LeftFootHomeLocalOffset, SafeMoveDirection));
	const FVector RightPlannedLocation = ClampFootLocationToLegReach(
		RightHipWorldLocation,
		CalculatePlannedFootLocation(RightFootHomeLocalOffset, SafeMoveDirection));
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
	LeftFootRuntime.EffectorWorldLocation = ClampFootLocationToLegReach(
		LeftHipWorldLocation,
		LeftFootRuntime.EffectorWorldLocation);
	RightFootRuntime.EffectorWorldLocation = ClampFootLocationToLegReach(
		RightHipWorldLocation,
		RightFootRuntime.EffectorWorldLocation);
	LeftFootRuntime.JointTargetWorldLocation = CalculateJointTargetLocation(
		LeftHipWorldLocation,
		LeftFootRuntime.EffectorWorldLocation,
		-1.0f);
	RightFootRuntime.JointTargetWorldLocation = CalculateJointTargetLocation(
		RightHipWorldLocation,
		RightFootRuntime.EffectorWorldLocation,
		1.0f);
	UpdateFootSceneComponents();
}

void ATunaSweeperRollingBomber::UpdateSpawnPhysicsMode(float DeltaSeconds)
{
	ModeElapsedSeconds += DeltaSeconds;
	SpawnPhysicsElapsedSeconds += DeltaSeconds;
	SpawnerLaunchControlRemainingSeconds = FMath::Max(
		0.0f,
		SpawnerLaunchControlRemainingSeconds - DeltaSeconds);

	const bool bMinimumTimeElapsed =
		SpawnPhysicsElapsedSeconds >= FMath::Max(0.0f, SpawnerPhysicsMinimumSeconds);
	if (!bMinimumTimeElapsed)
	{
		return;
	}

	const bool bGraceElapsed = SpawnerLaunchControlRemainingSeconds <= 0.0f;
	const bool bGrounded = IsSpawnPhysicsGrounded();
	if (IsSpawnPhysicsSettled() || (bGraceElapsed && bGrounded))
	{
		FinishSpawnPhysicsSimulation();
		EnterStandingUpFromSpawnMode();
	}
}

void ATunaSweeperRollingBomber::UpdateStandingUpFromSpawnMode(float DeltaSeconds)
{
	ModeElapsedSeconds += DeltaSeconds;
	SpawnStandUpAlpha = FMath::Clamp(
		ModeElapsedSeconds / FMath::Max(0.01f, SpawnStandUpDurationSeconds),
		0.0f,
		1.0f);
	LegFoldAlpha = 1.0f - SpawnStandUpAlpha;

	if (!bLegIKEnabled && SpawnStandUpAlpha >= 0.65f)
	{
		bLegIKEnabled = true;
		InitializeLegIKTargets();
	}

	if (SpawnStandUpAlpha >= 1.0f)
	{
		EnterProjectileAttackMode();
	}
}

void ATunaSweeperRollingBomber::FinishSpawnPhysicsSimulation()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		bSpawnPhysicsSimulationActive = false;
		return;
	}

	const FVector StopLocation = GetActorLocation();
	FVector PlanarVelocity = Capsule->GetPhysicsLinearVelocity();
	PlanarVelocity.Z = 0.0f;
	const float TargetYaw = PlanarVelocity.IsNearlyZero()
		? GetActorRotation().Yaw
		: PlanarVelocity.Rotation().Yaw;

	if (Capsule->IsSimulatingPhysics())
	{
		Capsule->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
		Capsule->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false);
		Capsule->SetSimulatePhysics(false);
	}

	Capsule->SetPhysMaterialOverride(nullptr);
	Capsule->SetCollisionObjectType(ECC_Pawn);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetActorLocation(StopLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.0f, TargetYaw, 0.0f), ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = ProjectileModeWalkSpeed;
	}

	bSpawnPhysicsSimulationActive = false;
}

bool ATunaSweeperRollingBomber::IsSpawnPhysicsGrounded() const
{
	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}

	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(
		0.0f,
		0.0f,
		Capsule->GetScaledCapsuleHalfHeight() + TunaSweeperRollingBomber::SpawnGroundTraceExtraDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperRollingBomberSpawnGroundTrace), false, this);
	QueryParams.AddIgnoredActor(this);

	FHitResult GroundHit;
	return World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) &&
		GroundHit.bBlockingHit &&
		GroundHit.ImpactNormal.Z >= TunaSweeperRollingBomber::MinFootGroundNormalZ;
}

bool ATunaSweeperRollingBomber::IsSpawnPhysicsSettled() const
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule || !IsSpawnPhysicsGrounded())
	{
		return false;
	}

	const FVector LinearVelocity = Capsule->GetPhysicsLinearVelocity();
	return LinearVelocity.Size() <= FMath::Max(0.0f, SpawnPhysicsSettleSpeed);
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

float ATunaSweeperRollingBomber::ResolveProjectileFireIntervalSeconds() const
{
	const float JitterSeconds = FMath::Max(0.0f, ProjectileFireIntervalJitterSeconds);
	return FMath::Max(
		0.05f,
		ProjectileFireIntervalSeconds + FMath::FRandRange(-JitterSeconds, JitterSeconds));
}

void ATunaSweeperRollingBomber::ResetProjectileFireTimer(bool bUseInitialDelay)
{
	CurrentProjectileFireIntervalSeconds = ResolveProjectileFireIntervalSeconds();
	if (!bUseInitialDelay)
	{
		ProjectileFireElapsedSeconds = 0.0f;
		return;
	}

	const float MinDelay = FMath::Max(0.0f, ProjectileInitialFireDelayMinSeconds);
	const float MaxDelay = FMath::Max(MinDelay, ProjectileInitialFireDelayMaxSeconds);
	const float InitialDelay = FMath::Clamp(
		FMath::FRandRange(MinDelay, MaxDelay),
		0.0f,
		CurrentProjectileFireIntervalSeconds);
	ProjectileFireElapsedSeconds = CurrentProjectileFireIntervalSeconds - InitialDelay;
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

	const float SafeProjectileScale = FMath::Max(0.05f, RollingBomberProjectileScale);
	const float ProjectileCollisionRadius = 12.0f * SafeProjectileScale;
	const float CapsuleRadius = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
	FVector EffectiveProjectileSpawnOffset = ProjectileSpawnOffset;
	EffectiveProjectileSpawnOffset.X = FMath::Max(
		EffectiveProjectileSpawnOffset.X,
		CapsuleRadius + ProjectileCollisionRadius + FMath::Max(0.0f, ProjectileSpawnClearance));
	const FVector SpawnLocation = ActorLocation + GetActorRotation().RotateVector(EffectiveProjectileSpawnOffset);
	const FTransform SpawnTransform(FireRotation, SpawnLocation, FVector(SafeProjectileScale));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperProjectile* SpawnedProjectile = World->SpawnActor<ATunaSweeperProjectile>(
		LoadedProjectileClass,
		SpawnTransform,
		SpawnParameters);
	if (!SpawnedProjectile)
	{
		return false;
	}

	SpawnedProjectile->IgnoreActor(this);
	SpawnedProjectile->SetSpeedMultiplier(RollingBomberProjectileSpeedMultiplier);
	UMaterialInterface* ProjectileMaterial = RollingBomberProjectileMaterial.LoadSynchronous();
	if (ProjectileMaterial)
	{
		SpawnedProjectile->ApplyVisualMaterial(
			ProjectileMaterial,
			RollingBomberProjectileColor,
			RollingBomberProjectileEmissiveStrength);
	}

	UMaterialInterface* TrailMaterial = RollingBomberProjectileTrailMaterial.LoadSynchronous();
	if (!TrailMaterial)
	{
		TrailMaterial = ProjectileMaterial;
	}
	if (TrailMaterial)
	{
		const float TrailScaleCompensation = 1.0f / SafeProjectileScale;
		SpawnedProjectile->ApplyTrailVisual(
			TrailMaterial,
			RollingBomberProjectileTrailColor,
			RollingBomberProjectileTrailEmissiveStrength,
			RollingBomberProjectileTrailLengthCm * TrailScaleCompensation,
			RollingBomberProjectileTrailRadiusCm * TrailScaleCompensation,
			RollingBomberProjectileTrailOpacity,
			RollingBomberProjectileTrailEndFade);
	}
	SpawnedProjectile->SetCameraHitReactionScale(RollingBomberProjectileCameraHitReactionScale);
	SpawnedProjectile->SetDamageAmount(FMath::Min(ProjectileDamage, ProjectileDamageCap));
	SpawnedProjectile->SetHitEffectId(ProjectileHitEffectId);
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

void ATunaSweeperRollingBomber::SpawnSelfDestructBurst()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ATunaSweeperMeleeImpactBurstActor> LoadedBurstClass = SelfDestructBurstActorClass.LoadSynchronous();
	if (!LoadedBurstClass)
	{
		LoadedBurstClass = ATunaSweeperMeleeImpactBurstActor::StaticClass();
	}

	FVector BurstLocation = GetActorLocation();
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		BurstLocation.Z += Capsule->GetScaledCapsuleHalfHeight() * 0.2f;
	}

	FRotator BurstRotation = LockedRollDirection.IsNearlyZero()
		? GetActorRotation()
		: LockedRollDirection.Rotation();
	BurstRotation.Pitch = 0.0f;
	BurstRotation.Roll = 0.0f;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperMeleeImpactBurstActor* BurstActor = World->SpawnActor<ATunaSweeperMeleeImpactBurstActor>(
		LoadedBurstClass,
		BurstLocation,
		BurstRotation,
		SpawnParameters);
	if (BurstActor)
	{
		BurstActor->SetBurstColor(SelfDestructBurstColor);
		BurstActor->SetActorScale3D(FVector(FMath::Max(0.0f, SelfDestructBurstVisualScale)));
	}
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
	if (!bUseBodyRollVisualRotation || BodyVisualRadiusCm <= 0.0f || DeltaDistance <= 0.0f)
	{
		return;
	}

	const float Circumference = 2.0f * UE_PI * BodyVisualRadiusCm;
	ApplyBodyRollVisualRotationDegrees((DeltaDistance / Circumference) * 360.0f);
}

void ATunaSweeperRollingBomber::ApplyBodyRollVisualRotationDegrees(float DeltaDegrees)
{
	if (!bUseBodyRollVisualRotation || DeltaDegrees <= 0.0f)
	{
		return;
	}

	BodyRollDegrees += DeltaDegrees;
	const float WobbleDegrees = BodyRollWobbleDegrees *
		FMath::Sin(FMath::DegreesToRadians(BodyRollDegrees * 0.5f));
	const FRotator RollingRotation(BodyRollDegrees, 0.0f, WobbleDegrees);

	if (BodyVisualPivot)
	{
		BodyVisualPivot->SetRelativeRotation(BodyVisualPivotBaseRelativeRotation + RollingRotation);
	}
	else if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(BodyVisualBaseRelativeRotation + RollingRotation);
	}
}

void ATunaSweeperRollingBomber::ApplyRollLaunchVisualHop()
{
	if (!BodyVisualPivot || RollLaunchVisualHopHeightCm <= 0.0f || RollLaunchVisualHopDurationSeconds <= 0.0f)
	{
		return;
	}

	const float HopAlpha = FMath::Clamp(
		ModeElapsedSeconds / FMath::Max(0.01f, RollLaunchVisualHopDurationSeconds),
		0.0f,
		1.0f);
	const float HopHeight = FMath::Sin(HopAlpha * UE_PI) * RollLaunchVisualHopHeightCm;
	BodyVisualPivot->SetRelativeLocation(BodyVisualPivotBaseRelativeLocation + FVector(0.0f, 0.0f, HopHeight));
}

void ATunaSweeperRollingBomber::SetRollChargeCylinderEffectActive(bool bActive)
{
	if (!RollChargeCylinderMesh)
	{
		return;
	}

	RollChargeCylinderMesh->SetVisibility(bActive, true);
	RollChargeCylinderMesh->SetHiddenInGame(!bActive, true);
}

void ATunaSweeperRollingBomber::UpdateRollChargeCylinderEffect(float ChargeAlpha)
{
	if (!RollChargeCylinderMesh)
	{
		return;
	}

	const float SafeChargeAlpha = FMath::Clamp(ChargeAlpha, 0.0f, 1.0f);
	const float Pulse = 1.0f +
		FMath::Sin(SafeChargeAlpha * UE_PI * 6.0f) *
		FMath::Max(0.0f, RollChargeCylinderPulseScale) *
		SafeChargeAlpha;
	RollChargeCylinderMesh->SetRelativeLocation(RollChargeCylinderLocalOffset);
	RollChargeCylinderMesh->SetRelativeScale3D(RollChargeCylinderLocalScale * Pulse);

	if (RollChargeCylinderDynamicMaterial)
	{
		const float Opacity = FMath::Lerp(
			FMath::Max(0.0f, RollChargeCylinderMinOpacity),
			FMath::Max(0.0f, RollChargeCylinderMaxOpacity),
			SafeChargeAlpha);
		RollChargeCylinderDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
}

void ATunaSweeperRollingBomber::ResetBodyRollVisualRotation()
{
	BodyRollDegrees = 0.0f;
	if (BodyVisualPivot)
	{
		BodyVisualPivot->SetRelativeLocation(BodyVisualPivotBaseRelativeLocation);
		BodyVisualPivot->SetRelativeRotation(BodyVisualPivotBaseRelativeRotation);
	}
	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(BodyVisualBaseRelativeRotation);
	}
}

void ATunaSweeperRollingBomber::ApplyLegVisualMaterial()
{
	UMaterialInterface* LoadedMaterial = LegMetalMaterial.LoadSynchronous();
	if (!LoadedMaterial)
	{
		LoadedMaterial = LoadObject<UMaterialInterface>(nullptr, TunaSweeperRollingBomber::LegMetalFallbackMaterialPath);
	}

	if (!LoadedMaterial)
	{
		return;
	}

	auto ApplyMaterial = [LoadedMaterial](UStaticMeshComponent* MeshComponent)
	{
		if (MeshComponent)
		{
			MeshComponent->SetMaterial(0, LoadedMaterial);
		}
	};

	ApplyMaterial(LeftUpperLegMesh);
	ApplyMaterial(LeftLowerLegMesh);
	ApplyMaterial(LeftFootMesh);
	ApplyMaterial(RightUpperLegMesh);
	ApplyMaterial(RightLowerLegMesh);
	ApplyMaterial(RightFootMesh);
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
		EyeLight->SetRelativeLocation(EyeLocalOffset + FVector(9.0f, 0.0f, 0.0f));
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
	InitializeFootRuntime(LeftFootRuntime, LeftFootHomeLocalOffset, LeftHipLocalOffset);
	InitializeFootRuntime(RightFootRuntime, RightFootHomeLocalOffset, RightHipLocalOffset);
	UpdateFootSceneComponents();
}

void ATunaSweeperRollingBomber::InitializeFootRuntime(
	FFootRuntime& FootRuntime,
	const FVector& FootHomeLocalOffset,
	const FVector& HipLocalOffset)
{
	const FVector HipWorldLocation = GetActorTransform().TransformPosition(HipLocalOffset);
	FootRuntime.EffectorWorldLocation = ClampFootLocationToLegReach(
		HipWorldLocation,
		CalculatePlannedFootLocation(FootHomeLocalOffset, FVector::ZeroVector));
	FootRuntime.PlannedFootWorldLocation = FootRuntime.EffectorWorldLocation;
	FootRuntime.StepStartWorldLocation = FootRuntime.EffectorWorldLocation;
	FootRuntime.JointTargetWorldLocation = CalculateJointTargetLocation(
		HipWorldLocation,
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

FVector ATunaSweeperRollingBomber::ClampFootLocationToLegReach(
	const FVector& HipWorldLocation,
	const FVector& DesiredFootWorldLocation) const
{
	const float UpperLength = FMath::Max(0.1f, UpperLegLengthCm);
	const float LowerLength = FMath::Max(0.1f, LowerLegLengthCm);
	const float Slack = FMath::Max(0.0f, LegReachSlackCm);
	const float MaxReach = FMath::Max(0.1f, UpperLength + LowerLength - Slack);
	const float MinReach = FMath::Max(0.0f, FMath::Abs(UpperLength - LowerLength) + Slack);
	const FVector HipToFoot = DesiredFootWorldLocation - HipWorldLocation;
	const float Distance = HipToFoot.Size();

	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return HipWorldLocation - FVector(0.0f, 0.0f, MinReach);
	}

	const FVector Direction = HipToFoot / Distance;
	if (Distance > MaxReach)
	{
		return HipWorldLocation + Direction * MaxReach;
	}

	if (MinReach > 0.0f && Distance < MinReach)
	{
		return HipWorldLocation + Direction * MinReach;
	}

	return DesiredFootWorldLocation;
}

FVector ATunaSweeperRollingBomber::CalculateJointTargetLocation(
	const FVector& HipWorldLocation,
	const FVector& FootWorldLocation,
	float SideSign) const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	const float UpperLength = FMath::Max(0.1f, UpperLegLengthCm);
	const float LowerLength = FMath::Max(0.1f, LowerLegLengthCm);

	const FVector HipToFoot = FootWorldLocation - HipWorldLocation;
	const float RawDistance = HipToFoot.Size();
	const FVector LegAxis = RawDistance > KINDA_SMALL_NUMBER
		? HipToFoot / RawDistance
		: -FVector::UpVector;
	const float MinDistance = FMath::Max(0.01f, FMath::Abs(UpperLength - LowerLength) + 0.01f);
	const float MaxDistance = FMath::Max(MinDistance, UpperLength + LowerLength - 0.01f);
	const float Distance = FMath::Clamp(RawDistance, MinDistance, MaxDistance);
	const float KneeAlongAxis = FMath::Clamp(
		(FMath::Square(UpperLength) - FMath::Square(LowerLength) + FMath::Square(Distance)) / (2.0f * Distance),
		0.0f,
		UpperLength);
	const float KneeBendHeight = FMath::Sqrt(FMath::Max(0.0f, FMath::Square(UpperLength) - FMath::Square(KneeAlongAxis)));

	const FVector DesiredPole =
		Forward * KneeForwardOffset +
		Right * (SideSign * KneeSideOffset) +
		FVector::UpVector * KneeHeightOffset;
	FVector BendDirection = DesiredPole - LegAxis * FVector::DotProduct(DesiredPole, LegAxis);
	if (BendDirection.IsNearlyZero())
	{
		BendDirection = Right * SideSign;
		BendDirection -= LegAxis * FVector::DotProduct(BendDirection, LegAxis);
	}
	if (BendDirection.IsNearlyZero())
	{
		BendDirection = FVector::CrossProduct(LegAxis, FVector::UpVector);
	}
	BendDirection = BendDirection.GetSafeNormal();

	return HipWorldLocation + LegAxis * KneeAlongAxis + BendDirection * KneeBendHeight;
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

	UpdateVisibleLegMeshes();
}

void ATunaSweeperRollingBomber::UpdateFoldedLegSceneComponents()
{
	const FTransform ActorTransform = GetActorTransform();
	const FVector LeftFoldedFootLocation = ActorTransform.TransformPosition(FVector(4.0f, -8.5f, -4.0f));
	const FVector RightFoldedFootLocation = ActorTransform.TransformPosition(FVector(4.0f, 8.5f, -4.0f));
	const FVector LeftFoldedKneeLocation = ActorTransform.TransformPosition(FVector(0.0f, -12.0f, 5.0f));
	const FVector RightFoldedKneeLocation = ActorTransform.TransformPosition(FVector(0.0f, 12.0f, 5.0f));

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

void ATunaSweeperRollingBomber::UpdateVisibleLegMeshes()
{
	const FTransform ActorTransform = GetActorTransform();
	UpdateVisibleLegMeshForFoot(
		LeftUpperLegMesh,
		LeftLowerLegMesh,
		LeftFootMesh,
		ActorTransform.TransformPosition(LeftHipLocalOffset),
		LeftFootRuntime);
	UpdateVisibleLegMeshForFoot(
		RightUpperLegMesh,
		RightLowerLegMesh,
		RightFootMesh,
		ActorTransform.TransformPosition(RightHipLocalOffset),
		RightFootRuntime);
}

void ATunaSweeperRollingBomber::UpdateVisibleLegMeshForFoot(
	UStaticMeshComponent* UpperLegMesh,
	UStaticMeshComponent* LowerLegMesh,
	UStaticMeshComponent* FootMesh,
	const FVector& HipWorldLocation,
	const FFootRuntime& FootRuntime) const
{
	PositionLegSegmentMesh(
		UpperLegMesh,
		HipWorldLocation,
		FootRuntime.JointTargetWorldLocation,
		UpperLegThicknessCm,
		UpperLegLengthCm);
	PositionLegSegmentMesh(
		LowerLegMesh,
		FootRuntime.JointTargetWorldLocation,
		FootRuntime.EffectorWorldLocation,
		LowerLegThicknessCm,
		LowerLegLengthCm);
	PositionFootMesh(FootMesh, FootRuntime.EffectorWorldLocation);
}

void ATunaSweeperRollingBomber::PositionLegSegmentMesh(
	UStaticMeshComponent* SegmentMesh,
	const FVector& StartWorldLocation,
	const FVector& EndWorldLocation,
	float ThicknessCm,
	float TargetLengthCm) const
{
	if (!SegmentMesh)
	{
		return;
	}

	const FVector SegmentVector = EndWorldLocation - StartWorldLocation;
	const float SegmentLength = SegmentVector.Size();
	if (SegmentLength <= KINDA_SMALL_NUMBER)
	{
		SegmentMesh->SetVisibility(false);
		return;
	}

	const float VisualLength = TargetLengthCm > 0.0f
		? FMath::Min(SegmentLength, TargetLengthCm)
		: SegmentLength;
	const FVector Direction = SegmentVector / SegmentLength;
	const FVector VisualEndLocation = StartWorldLocation + Direction * VisualLength;

	SegmentMesh->SetVisibility(true);
	SegmentMesh->SetWorldLocation((StartWorldLocation + VisualEndLocation) * 0.5f);
	SegmentMesh->SetWorldRotation(FRotationMatrix::MakeFromZ(Direction).Rotator());
	const float RadiusScale = FMath::Max(0.1f, ThicknessCm) / 50.0f;
	SegmentMesh->SetWorldScale3D(FVector(RadiusScale, RadiusScale, VisualLength / 100.0f));
}

void ATunaSweeperRollingBomber::PositionFootMesh(
	UStaticMeshComponent* FootMesh,
	const FVector& FootWorldLocation) const
{
	if (!FootMesh)
	{
		return;
	}

	FootMesh->SetVisibility(true);
	FootMesh->SetWorldLocation(FootWorldLocation + FVector(0.0f, 0.0f, FootVisualHeightCm * 0.5f));
	FootMesh->SetWorldRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f));
	FootMesh->SetWorldScale3D(FVector(
		FMath::Max(0.1f, FootVisualLengthCm) / 100.0f,
		FMath::Max(0.1f, FootVisualWidthCm) / 100.0f,
		FMath::Max(0.1f, FootVisualHeightCm) / 100.0f));
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
