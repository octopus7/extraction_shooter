#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_TunaSweeperTitleHeadLook.generated.h"

// Applies the title's absolute (+Y forward) gaze to the input pose, before RBAN.
USTRUCT(BlueprintInternalUseOnly)
struct TUNASWEEPER_API FAnimNode_TunaSweeperTitleHeadLook : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
		TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override {}

private:
	bool ResolveBones(const FBoneContainer& Bones, FCompactPoseBoneIndex& Head, FCompactPoseBoneIndex& Root) const;
	bool bHasTarget = false;
	float Yaw = 0.0f;
	float Pitch = 0.0f;
	FName HeadName;
	FName RootName;
};
