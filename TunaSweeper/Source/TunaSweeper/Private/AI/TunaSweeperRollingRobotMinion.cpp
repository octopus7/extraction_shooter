#include "AI/TunaSweeperRollingRobotMinion.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "TunaSweeperCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperRollingRobotMinion::ATunaSweeperRollingRobotMinion()
{
	GetCapsuleComponent()->InitCapsuleSize(BallRadius, BallRadius);
	GetCapsuleComponent()->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ThisClass::HandleCapsuleHit);
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	MovementSpeedRandomOffset = FVector2D::ZeroVector;
	MaxHealth = 20.0f;
	ExperienceValue = 10;
	EnemyId = TEXT("rolling_robot_minion");

	FTunaSweeperEnemyCombatProfile MinionProfile;
	MinionProfile.ProfileId = TEXT("rolling_robot_minion");
	MinionProfile.AttackMode = ETunaSweeperEnemyAttackMode::Melee;
	MinionProfile.Role = ETunaSweeperEnemyCombatRole::Melee;
	MinionProfile.MovementSpeed = 300.0f;
	MinionProfile.TrackingRange = 2500.0f;
	MinionProfile.AlertSeconds = 0.15f;
	MinionProfile.MeleeAttackDamage = 7.0f;
	MinionProfile.AttackCooldownSeconds = 1.1f;
	ConfigureCombatProfile(MinionProfile, TunaSweeperFactionIds::Enemy, NAME_None, INDEX_NONE);

	RobotBodyPivot = CreateDefaultSubobject<USceneComponent>(TEXT("RobotBodyPivot"));
	RobotBodyPivot->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto ConfigureMesh = [](UStaticMeshComponent* VisualPart, USceneComponent* Parent, UStaticMesh* Asset)
	{
		VisualPart->SetupAttachment(Parent);
		VisualPart->SetStaticMesh(Asset);
		VisualPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VisualPart->SetGenerateOverlapEvents(false);
		VisualPart->SetCanEverAffectNavigation(false);
	};
	RobotShell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RobotShell"));
	ConfigureMesh(RobotShell, RobotBodyPivot, SphereMesh.Object);
	RobotEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RobotEye"));
	ConfigureMesh(RobotEye, RobotBodyPivot, CubeMesh.Object);
	LeftLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLeg"));
	ConfigureMesh(LeftLeg, RootComponent, CubeMesh.Object);
	RightLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLeg"));
	ConfigureMesh(RightLeg, RootComponent, CubeMesh.Object);
	LeftFoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFoot"));
	ConfigureMesh(LeftFoot, RootComponent, CubeMesh.Object);
	RightFoot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFoot"));
	ConfigureMesh(RightFoot, RootComponent, CubeMesh.Object);

	ShellMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/Characters/Enemy/M_RollingBomberBodyGray.M_RollingBomberBodyGray")));
	LegMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/Characters/Enemy/M_RollingBomberLegMetal.M_RollingBomberLegMetal")));
	VisualMesh->SetHiddenInGame(true);
	VisualMesh->SetVisibility(false);
	ForwardMarkerMesh->SetHiddenInGame(true);
	ForwardMarkerMesh->SetVisibility(false);
	UpdateRobotPresentation(0.0f, 0.0f);
}

bool ATunaSweeperRollingRobotMinion::IsDeployingFromRoll() const
{
	return DeploymentPhase == ETunaSweeperRollingRobotPhase::Rolling ||
		DeploymentPhase == ETunaSweeperRollingRobotPhase::Unfolding;
}

bool ATunaSweeperRollingRobotMinion::IsStandardCombatSuppressed() const
{
	return IsDeployingFromRoll() || Super::IsStandardCombatSuppressed();
}

void ATunaSweeperRollingRobotMinion::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!HasActorBegunPlay())
	{
		const float Radius = FMath::Max(10.0f, BallRadius);
		GetCapsuleComponent()->SetCapsuleSize(Radius, Radius);
		UpdateRobotPresentation(0.0f, 0.0f);
		ApplyRobotMaterials();
	}
}

void ATunaSweeperRollingRobotMinion::BeginPlay()
{
	Super::BeginPlay();
	ApplyRobotMaterials();
	if (!bRollWasInitialized)
	{
		RollDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	StartRollMovement();
}

void ATunaSweeperRollingRobotMinion::InitializeRoll(const FVector& Direction, AActor* TargetActor)
{
	if (IsDead())
	{
		return;
	}
	RollDirection = Direction.ContainsNaN() ? FVector::ZeroVector : Direction.GetSafeNormal2D();
	if (RollDirection.IsNearlyZero())
	{
		RollDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	DeploymentTarget = TargetActor;
	bRollWasInitialized = true;
	if (HasActorBegunPlay())
	{
		StartRollMovement();
	}
}

void ATunaSweeperRollingRobotMinion::StartRollMovement()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (IsDead() || !Movement)
	{
		return;
	}
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
	}
	if (!bMovementSettingsSaved)
	{
		SavedWalkSpeed = Movement->MaxWalkSpeed;
		SavedGroundFriction = Movement->GroundFriction;
		SavedBrakingDeceleration = Movement->BrakingDecelerationWalking;
		SavedFootstepLoudness = FootstepNoiseLoudness;
		bMovementSettingsSaved = true;
	}
	const float Radius = FMath::Max(10.0f, BallRadius);
	const float OldHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	GetCapsuleComponent()->SetCapsuleSize(Radius, Radius, true);
	// Preserve the bottom of the collision capsule when an already walking robot is relaunched.
	if (OldHalfHeight > Radius)
	{
		AddActorWorldOffset(FVector(0.0f, 0.0f,
			-(OldHalfHeight - Radius) * GetCapsuleComponent()->GetShapeScale()), true);
	}
	DeploymentPhase = ETunaSweeperRollingRobotPhase::Rolling;
	DeploymentElapsedSeconds = 0.0f;
	BodyRollDegrees = 0.0f;
	bRollBlocked = false;
	FootstepNoiseLoudness = 0.0f;
	Movement->StopMovementImmediately();
	Movement->GroundFriction = 0.0f;
	Movement->BrakingDecelerationWalking = 0.0f;
	Movement->MaxWalkSpeed = FMath::Max(1.0f, RollSpeed);
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = RollDirection * FMath::Max(1.0f, RollSpeed) +
		FVector::UpVector * FMath::Max(0.0f, LaunchUpwardSpeed);
	LastPresentationLocation = GetActorLocation();
	UpdateRobotPresentation(0.0f, 0.0f);
}

void ATunaSweeperRollingRobotMinion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsDead() || DeltaSeconds <= 0.0f)
	{
		return;
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	DeploymentElapsedSeconds += DeltaSeconds;
	if (DeploymentPhase == ETunaSweeperRollingRobotPhase::Rolling)
	{
		const float Duration = FMath::Max(0.05f, RollDurationSeconds);
		const float RollAlpha = FMath::Clamp(DeploymentElapsedSeconds / Duration, 0.0f, 1.0f);
		const float Speed = bRollBlocked || RollAlpha >= 1.0f
			? 0.0f : FMath::Max(1.0f, RollSpeed) * FMath::Lerp(1.0f, 0.25f, RollAlpha);
		Movement->Velocity.X = RollDirection.X * Speed;
		Movement->Velocity.Y = RollDirection.Y * Speed;
		UpdateRobotPresentation(0.0f, DeltaSeconds);
		if ((bRollBlocked || RollAlpha >= 1.0f) && Movement->IsMovingOnGround())
		{
			TryBeginUnfolding();
		}
	}
	else if (DeploymentPhase == ETunaSweeperRollingRobotPhase::Unfolding)
	{
		const float Alpha = FMath::Clamp(DeploymentElapsedSeconds / FMath::Max(0.05f, UnfoldDurationSeconds), 0.0f, 1.0f);
		UpdateRobotPresentation(Alpha, DeltaSeconds);
		if (Alpha >= 1.0f)
		{
			FinishUnfolding();
		}
	}
	else if (DeploymentPhase == ETunaSweeperRollingRobotPhase::Walking)
	{
		UpdateRobotPresentation(1.0f, DeltaSeconds);
	}
}

bool ATunaSweeperRollingRobotMinion::TryBeginUnfolding()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = FMath::Max(10.0f, BallRadius);
	const float HalfHeight = FMath::Max(Radius, StandingHalfHeight);
	const float Scale = Capsule->GetShapeScale();
	const FVector StandingLocation = GetActorLocation() + FVector::UpVector * (HalfHeight - Radius) * Scale;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RollingRobotStandUp), false, this);
	FCollisionResponseParams ResponseParams;
	Capsule->InitSweepCollisionParams(QueryParams, ResponseParams);
	// Remain damageable in ball form while a low ceiling or another pawn occupies standing space.
	if (GetWorld()->OverlapBlockingTestByChannel(StandingLocation, GetActorQuat(),
		Capsule->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius * Scale, HalfHeight * Scale),
		QueryParams, ResponseParams))
	{
		return false;
	}
	FHitResult MoveHit;
	SetActorLocation(StandingLocation, true, &MoveHit);
	if (MoveHit.bBlockingHit)
	{
		return false;
	}
	Capsule->SetCapsuleSize(Radius, HalfHeight, true);
	GetCharacterMovement()->StopMovementImmediately();
	DeploymentPhase = ETunaSweeperRollingRobotPhase::Unfolding;
	DeploymentElapsedSeconds = 0.0f;
	LastPresentationLocation = GetActorLocation();
	UpdateRobotPresentation(0.0f, 0.0f);
	return true;
}

void ATunaSweeperRollingRobotMinion::FinishUnfolding()
{
	DeploymentPhase = ETunaSweeperRollingRobotPhase::Walking;
	DeploymentElapsedSeconds = 0.0f;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->GroundFriction = SavedGroundFriction;
	Movement->BrakingDecelerationWalking = SavedBrakingDeceleration;
	Movement->MaxWalkSpeed = SavedWalkSpeed;
	FootstepNoiseLoudness = SavedFootstepLoudness;
	bMovementSettingsSaved = false;
	BodyRollDegrees = 0.0f;
	UpdateRobotPresentation(1.0f, 0.0f);
	if (AActor* Target = DeploymentTarget.Get())
	{
		UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
		if (Factions && Factions->CanTargetActor(this, Target))
		{
			if (ATunaSweeperEnemyAIController* AI = Cast<ATunaSweeperEnemyAIController>(GetController()))
			{
				AI->NotifyDamageTaken(Target);
			}
		}
	}
	DeploymentTarget.Reset();
}

void ATunaSweeperRollingRobotMinion::HandleCapsuleHit(UPrimitiveComponent* HitComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (DeploymentPhase == ETunaSweeperRollingRobotPhase::Rolling &&
		OtherComponent && OtherComponent->GetCollisionObjectType() != TunaSweeperCollisionChannels::Projectile &&
		Hit.ImpactNormal.Z < GetCharacterMovement()->GetWalkableFloorZ() &&
		FVector::DotProduct(Hit.ImpactNormal, RollDirection) < -0.2f)
	{
		bRollBlocked = true;
	}
}

void ATunaSweeperRollingRobotMinion::UpdateRobotPresentation(float UnfoldAlpha, float DeltaSeconds)
{
	const float Radius = FMath::Max(10.0f, BallRadius);
	const float HeightDifference = FMath::Max(Radius, StandingHalfHeight) - Radius;
	const bool bRolling = DeploymentPhase == ETunaSweeperRollingRobotPhase::Rolling;
	const FVector Position = GetActorLocation();
	if (DeltaSeconds > 0.0f)
	{
		const float Distance = FVector::Dist2D(Position, LastPresentationLocation);
		if (bRolling)
		{
			BodyRollDegrees = FMath::Fmod(BodyRollDegrees + FMath::RadiansToDegrees(Distance / Radius), 360.0f);
		}
		else if (DeploymentPhase == ETunaSweeperRollingRobotPhase::Walking)
		{
			WalkCycleRadians = FMath::Fmod(WalkCycleRadians + Distance / 18.0f, 2.0f * PI);
		}
	}
	LastPresentationLocation = Position;
	const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, UnfoldAlpha);
	RobotBodyPivot->SetRelativeLocation(FVector(0.0f, 0.0f,
		bRolling ? 0.0f : FMath::Lerp(-HeightDifference, HeightDifference, SmoothAlpha)));
	// Rotation is applied about the actual launch axis, independently from the AI's facing yaw.
	const FVector LocalRollAxis = GetActorQuat().UnrotateVector(FVector::CrossProduct(FVector::UpVector, RollDirection));
	const FQuat RollRotation(LocalRollAxis.GetSafeNormal(), FMath::DegreesToRadians(BodyRollDegrees));
	RobotBodyPivot->SetRelativeRotation(bRolling ? RollRotation : FQuat::Slerp(RollRotation, FQuat::Identity, SmoothAlpha));
	RobotShell->SetRelativeScale3D(FVector(Radius / 50.0f));
	RobotEye->SetRelativeLocation(FVector(Radius * 0.94f, 0.0f, 2.0f));
	RobotEye->SetRelativeScale3D(FVector(0.07f, Radius / 110.0f, 0.12f));
	const bool bWalking = DeploymentPhase == ETunaSweeperRollingRobotPhase::Walking;
	const float Gait = bWalking && GetVelocity().SizeSquared2D() > FMath::Square(10.0f)
		? FMath::Sin(WalkCycleRadians) : 0.0f;
	const float LegLength = FMath::Max(4.0f, HeightDifference * 2.0f - 8.0f);
	const float HalfHeight = FMath::Max(Radius, StandingHalfHeight);
	auto PositionLeg = [&](UStaticMeshComponent* Leg, UStaticMeshComponent* Foot, float Side)
	{
		const float Step = Gait * Side;
		const float Lift = FMath::Max(0.0f, Step) * 9.0f;
		Leg->SetVisibility(!bRolling && UnfoldAlpha > 0.01f);
		Foot->SetVisibility(!bRolling && UnfoldAlpha > 0.01f);
		Leg->SetRelativeLocation(FVector(Step * 5.0f, Side * Radius * 0.46f,
			-HalfHeight + 8.0f + LegLength * 0.5f + Lift * 0.5f));
		Leg->SetRelativeRotation(FRotator(Step * 22.0f, 0.0f, 0.0f));
		Leg->SetRelativeScale3D(FVector(0.12f, 0.12f, LegLength * SmoothAlpha / 100.0f));
		Foot->SetRelativeLocation(FVector(8.0f + Step * 13.0f, Side * Radius * 0.46f,
			FMath::Lerp(-HeightDifference, -HalfHeight + 4.0f + Lift, SmoothAlpha)));
		Foot->SetRelativeScale3D(FVector(0.30f, 0.22f, 0.08f) * SmoothAlpha);
	};
	PositionLeg(LeftLeg, LeftFoot, -1.0f);
	PositionLeg(RightLeg, RightFoot, 1.0f);
}

void ATunaSweeperRollingRobotMinion::ApplyRobotMaterials()
{
	VisualMesh->SetVisibility(false);
	VisualMesh->SetHiddenInGame(true);
	ForwardMarkerMesh->SetVisibility(false);
	ForwardMarkerMesh->SetHiddenInGame(true);
	if (UMaterialInterface* Material = ShellMaterial.LoadSynchronous())
	{
		RobotShell->SetMaterial(0, Material);
	}
	if (UMaterialInterface* Material = LegMaterial.LoadSynchronous())
	{
		RobotEye->SetMaterial(0, Material);
		LeftLeg->SetMaterial(0, Material);
		RightLeg->SetMaterial(0, Material);
		LeftFoot->SetMaterial(0, Material);
		RightFoot->SetMaterial(0, Material);
	}
}

void ATunaSweeperRollingRobotMinion::OnDeathPresentationStarted()
{
	DeploymentPhase = ETunaSweeperRollingRobotPhase::Dead;
	DeploymentTarget.Reset();
	Super::OnDeathPresentationStarted();
}
