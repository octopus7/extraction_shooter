#include "AI/TunaSweeperRollingBomberAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"

void UTunaSweeperRollingBomberAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RefreshOwner();
}

void UTunaSweeperRollingBomberAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	RefreshOwner();

	ATunaSweeperRollingBomber* RollingBomber = RollingBomberOwner;
	if (!RollingBomber)
	{
		ResetAnimState();
		return;
	}

	bHasRollingBomberOwner = true;
	Mode = RollingBomber->GetRollingBomberMode();
	bIsSpawnPhysicsMode = Mode == ETunaSweeperRollingBomberMode::SpawnPhysics;
	bIsStandingUpFromSpawnMode = Mode == ETunaSweeperRollingBomberMode::StandingUpFromSpawn;
	bIsSpawnTransitionMode = bIsSpawnPhysicsMode || bIsStandingUpFromSpawnMode;
	bIsProjectileAttackMode = Mode == ETunaSweeperRollingBomberMode::ProjectileAttack;
	bIsFoldingLegsMode = Mode == ETunaSweeperRollingBomberMode::FoldingLegs;
	bIsRollingMode = Mode == ETunaSweeperRollingBomberMode::Rolling;
	bIsRecoveringLegsMode = Mode == ETunaSweeperRollingBomberMode::RecoveringLegs;
	bIsSelfDestructedMode = Mode == ETunaSweeperRollingBomberMode::SelfDestructed;

	FVector Velocity = RollingBomber->GetVelocity();
	Velocity.Z = 0.0f;
	GroundSpeed = Velocity.Size();
	RollDirectionWorld = RollingBomber->GetLockedRollDirection();
	RollDistanceTraveled = RollingBomber->GetRollDistanceTraveled();
	BodyRollDegrees = RollingBomber->GetBodyRollDegrees();

	bLegIKEnabled = RollingBomber->IsLegIKEnabled();
	LegFoldAlpha = RollingBomber->GetLegFoldAlpha();
	SpawnStandUpAlpha = RollingBomber->GetSpawnStandUpAlpha();
	UpdateFootIKState(LeftFootIK, RollingBomber->GetFootIKState(ETunaSweeperRollingBomberFoot::Left));
	UpdateFootIKState(RightFootIK, RollingBomber->GetFootIKState(ETunaSweeperRollingBomberFoot::Right));

	bEyeChargeWarningActive = RollingBomber->IsEyeChargeWarningActive();
	EyeEmissiveColor = RollingBomber->GetEyeEmissiveColor();
	EyeEmissiveStrength = RollingBomber->GetEyeEmissiveStrength();
}

void UTunaSweeperRollingBomberAnimInstance::RefreshOwner()
{
	if (IsValid(RollingBomberOwner))
	{
		return;
	}

	RollingBomberOwner = Cast<ATunaSweeperRollingBomber>(TryGetPawnOwner());
	if (!IsValid(RollingBomberOwner))
	{
		RollingBomberOwner = Cast<ATunaSweeperRollingBomber>(GetOwningActor());
	}
}

void UTunaSweeperRollingBomberAnimInstance::ResetAnimState()
{
	RollingBomberOwner = nullptr;
	bHasRollingBomberOwner = false;
	Mode = ETunaSweeperRollingBomberMode::ProjectileAttack;
	bIsSpawnPhysicsMode = false;
	bIsStandingUpFromSpawnMode = false;
	bIsSpawnTransitionMode = false;
	bIsProjectileAttackMode = false;
	bIsFoldingLegsMode = false;
	bIsRollingMode = false;
	bIsRecoveringLegsMode = false;
	bIsSelfDestructedMode = false;
	GroundSpeed = 0.0f;
	RollDirectionWorld = FVector::ForwardVector;
	RollDistanceTraveled = 0.0f;
	BodyRollDegrees = 0.0f;
	bLegIKEnabled = false;
	LegFoldAlpha = 0.0f;
	SpawnStandUpAlpha = 0.0f;
	LeftFootIK = FTunaSweeperRollingBomberAnimFootIKState();
	RightFootIK = FTunaSweeperRollingBomberAnimFootIKState();
	bEyeChargeWarningActive = false;
	EyeEmissiveColor = FLinearColor::White;
	EyeEmissiveStrength = 0.0f;
}

void UTunaSweeperRollingBomberAnimInstance::UpdateFootIKState(
	FTunaSweeperRollingBomberAnimFootIKState& OutAnimFootState,
	const FTunaSweeperRollingBomberFootIKState& SourceFootState) const
{
	OutAnimFootState.EffectorWorldLocation = SourceFootState.EffectorWorldLocation;
	OutAnimFootState.JointTargetWorldLocation = SourceFootState.JointTargetWorldLocation;
	OutAnimFootState.bIsStepping = SourceFootState.bIsStepping;
	OutAnimFootState.StepAlpha = SourceFootState.StepAlpha;

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (!MeshComponent)
	{
		OutAnimFootState.EffectorComponentLocation = SourceFootState.EffectorWorldLocation;
		OutAnimFootState.JointTargetComponentLocation = SourceFootState.JointTargetWorldLocation;
		return;
	}

	const FTransform ComponentTransform = MeshComponent->GetComponentTransform();
	OutAnimFootState.EffectorComponentLocation =
		ComponentTransform.InverseTransformPosition(SourceFootState.EffectorWorldLocation);
	OutAnimFootState.JointTargetComponentLocation =
		ComponentTransform.InverseTransformPosition(SourceFootState.JointTargetWorldLocation);
}
