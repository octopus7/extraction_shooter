#pragma once

#include "Animation/BoneReference.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Containers/StaticArray.h"
#include "CoreMinimal.h"
#include "QuadrupedRigProfile.h"
#include "AnimNode_QuadrupedRobotIK.generated.h"

struct FQuadrupedRobotIKCachedLimb
{
	EQuadrupedLegSlot Slot = EQuadrupedLegSlot::FrontLeft;
	FBoneReference UpperBone;
	FBoneReference LowerBone;
	FBoneReference FootBone;
	FCompactPoseBoneIndex UpperIndex = FCompactPoseBoneIndex(INDEX_NONE);
	FCompactPoseBoneIndex LowerIndex = FCompactPoseBoneIndex(INDEX_NONE);
	FCompactPoseBoneIndex FootIndex = FCompactPoseBoneIndex(INDEX_NONE);
	FVector FallbackPoleDirection = FVector::ForwardVector;
	float PoleDistance = 75.0f;
	bool bDerivePoleFromInputPose = true;
	bool bValid = false;
};

/** Applies four analytic two-bone solves using targets published by UQuadrupedComponent. */
USTRUCT(BlueprintInternalUseOnly)
struct MIYAKOVCHARACTERSYSTEM_API FAnimNode_QuadrupedRobotIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	FAnimNode_QuadrupedRobotIK();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped", meta = (PinHiddenByDefault))
	TObjectPtr<UQuadrupedRigProfile> RigProfile;

	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

protected:
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void EvaluateSkeletalControl_AnyThread(
		FComponentSpacePoseContext& Output,
		TArray<FBoneTransform>& OutBoneTransforms) override;

private:
	TArray<FQuadrupedRobotIKCachedLimb> CachedLimbs;
	TStaticArray<FVector, 4> CachedEffectorLocations;
	const USkeleton* CachedTargetSkeleton = nullptr;
	bool bHasValidTargets = false;
	bool bMaintainFootRelativeRotation = true;
	bool bAllowStretching = false;
	double StartStretchRatio = 1.0;
	double MaxStretchScale = 1.0;
};
