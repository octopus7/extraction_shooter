#include "AnimNode_QuadrupedRobotIK.h"

#include "Animation/AnimInstance.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "QuadrupedComponent.h"
#include "TwoBoneIK.h"

FAnimNode_QuadrupedRobotIK::FAnimNode_QuadrupedRobotIK()
{
	for (FVector& EffectorLocation : CachedEffectorLocations)
	{
		EffectorLocation = FVector::ZeroVector;
	}
}

void FAnimNode_QuadrupedRobotIK::PreUpdate(const UAnimInstance* InAnimInstance)
{
	bHasValidTargets = false;

	if (!InAnimInstance)
	{
		return;
	}

	USkeletalMeshComponent* SkeletalMeshComponent = InAnimInstance->GetSkelMeshComponent();
	AActor* Owner = SkeletalMeshComponent ? SkeletalMeshComponent->GetOwner() : nullptr;
	const UQuadrupedComponent* QuadrupedComponent = Owner ? Owner->FindComponentByClass<UQuadrupedComponent>() : nullptr;
	if (!SkeletalMeshComponent || !QuadrupedComponent || QuadrupedComponent->Legs.Num() != 4)
	{
		return;
	}

	const FTransform MeshWorldTransform = SkeletalMeshComponent->GetComponentTransform();
	for (int32 LegIndex = 0; LegIndex < CachedEffectorLocations.Num(); ++LegIndex)
	{
		const FVector WorldTarget = QuadrupedComponent->GetFootPosition(LegIndex);
		if (WorldTarget.ContainsNaN())
		{
			return;
		}

		CachedEffectorLocations[LegIndex] = MeshWorldTransform.InverseTransformPosition(WorldTarget);
	}

	bHasValidTargets = true;
}

void FAnimNode_QuadrupedRobotIK::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += TEXT("(");
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(
		TEXT(" Profile: %s, Targets: %s)"),
		*GetNameSafe(RigProfile),
		bHasValidTargets ? TEXT("valid") : TEXT("missing"));
	DebugData.AddDebugItem(DebugLine);
	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_QuadrupedRobotIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	CachedLimbs.Reset();
	CachedTargetSkeleton = nullptr;
	bMaintainFootRelativeRotation = true;
	bAllowStretching = false;
	StartStretchRatio = 1.0;
	MaxStretchScale = 1.0;

	if (!RigProfile || !RigProfile->HasCompleteFourLegBinding())
	{
		return;
	}

	bMaintainFootRelativeRotation = RigProfile->bMaintainFootRelativeRotation;
	CachedTargetSkeleton = RigProfile->TargetSkeleton;
	bAllowStretching = RigProfile->bAllowStretching;
	StartStretchRatio = RigProfile->StartStretchRatio;
	MaxStretchScale = RigProfile->MaxStretchScale;
	CachedLimbs.Reserve(4);

	for (uint8 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		const FQuadrupedLimbRigBinding* Binding = RigProfile->FindLimb(static_cast<EQuadrupedLegSlot>(SlotIndex));
		if (!Binding)
		{
			continue;
		}

		FQuadrupedRobotIKCachedLimb& CachedLimb = CachedLimbs.AddDefaulted_GetRef();
		CachedLimb.Slot = Binding->Slot;
		CachedLimb.UpperBone.BoneName = Binding->UpperBone;
		CachedLimb.LowerBone.BoneName = Binding->LowerBone;
		CachedLimb.FootBone.BoneName = Binding->FootBone;
		CachedLimb.FallbackPoleDirection = Binding->FallbackPoleDirection;
		CachedLimb.PoleDistance = Binding->PoleDistance;
		CachedLimb.bDerivePoleFromInputPose = Binding->bDerivePoleFromInputPose;

		InitializeAndValidateBoneRef(CachedLimb.UpperBone, RequiredBones);
		InitializeAndValidateBoneRef(CachedLimb.LowerBone, RequiredBones);
		InitializeAndValidateBoneRef(CachedLimb.FootBone, RequiredBones);

		CachedLimb.UpperIndex = CachedLimb.UpperBone.GetCompactPoseIndex(RequiredBones);
		CachedLimb.LowerIndex = CachedLimb.LowerBone.GetCompactPoseIndex(RequiredBones);
		CachedLimb.FootIndex = CachedLimb.FootBone.GetCompactPoseIndex(RequiredBones);
		CachedLimb.bValid =
			CachedLimb.UpperBone.IsValidToEvaluate(RequiredBones) &&
			CachedLimb.LowerBone.IsValidToEvaluate(RequiredBones) &&
			CachedLimb.FootBone.IsValidToEvaluate(RequiredBones) &&
			RequiredBones.GetParentBoneIndex(CachedLimb.FootIndex) == CachedLimb.LowerIndex &&
			RequiredBones.GetParentBoneIndex(CachedLimb.LowerIndex) == CachedLimb.UpperIndex;
	}
}

bool FAnimNode_QuadrupedRobotIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (!RigProfile || CachedLimbs.Num() != 4)
	{
		return false;
	}

	if (CachedTargetSkeleton && Skeleton != CachedTargetSkeleton)
	{
		return false;
	}

	return CachedLimbs.ContainsByPredicate([](const FQuadrupedRobotIKCachedLimb& Limb)
	{
		return !Limb.bValid;
	}) == false;
}

void FAnimNode_QuadrupedRobotIK::EvaluateSkeletalControl_AnyThread(
	FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	if (!bHasValidTargets)
	{
		return;
	}

	OutBoneTransforms.Reserve(12);
	for (const FQuadrupedRobotIKCachedLimb& Limb : CachedLimbs)
	{
		if (!Limb.bValid)
		{
			continue;
		}

		const FTransform OriginalFootLocalTransform = Output.Pose.GetLocalSpaceTransform(Limb.FootIndex);
		FTransform UpperTransform = Output.Pose.GetComponentSpaceTransform(Limb.UpperIndex);
		FTransform LowerTransform = Output.Pose.GetComponentSpaceTransform(Limb.LowerIndex);
		FTransform FootTransform = Output.Pose.GetComponentSpaceTransform(Limb.FootIndex);

		const FVector UpperLocation = UpperTransform.GetLocation();
		const FVector LowerLocation = LowerTransform.GetLocation();
		const FVector FootLocation = FootTransform.GetLocation();
		FVector PoleDirection = Limb.FallbackPoleDirection.GetSafeNormal();

		if (Limb.bDerivePoleFromInputPose)
		{
			const FVector ChainDirection = (FootLocation - UpperLocation).GetSafeNormal();
			const FVector ProjectedLower = UpperLocation +
				ChainDirection * FVector::DotProduct(LowerLocation - UpperLocation, ChainDirection);
			const FVector PoseBendDirection = (LowerLocation - ProjectedLower).GetSafeNormal();
			if (!PoseBendDirection.IsNearlyZero())
			{
				PoleDirection = PoseBendDirection;
			}
		}

		if (PoleDirection.IsNearlyZero())
		{
			PoleDirection = FVector::ForwardVector;
		}

		const FVector JointTarget = UpperLocation + PoleDirection * FMath::Max(1.0f, Limb.PoleDistance);
		const int32 TargetIndex = static_cast<int32>(Limb.Slot);
		AnimationCore::SolveTwoBoneIK(
			UpperTransform,
			LowerTransform,
			FootTransform,
			JointTarget,
			CachedEffectorLocations[TargetIndex],
			bAllowStretching,
			StartStretchRatio,
			MaxStretchScale);

		if (bMaintainFootRelativeRotation)
		{
			FootTransform = OriginalFootLocalTransform * LowerTransform;
		}

		OutBoneTransforms.Add(FBoneTransform(Limb.UpperIndex, UpperTransform));
		OutBoneTransforms.Add(FBoneTransform(Limb.LowerIndex, LowerTransform));
		OutBoneTransforms.Add(FBoneTransform(Limb.FootIndex, FootTransform));
	}

	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}
