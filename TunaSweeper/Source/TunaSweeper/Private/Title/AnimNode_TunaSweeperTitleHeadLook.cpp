#include "Title/AnimNode_TunaSweeperTitleHeadLook.h"

#include "Animation/AnimInstance.h"
#include "Title/TunaSweeperTitlePresentationActor.h"

void FAnimNode_TunaSweeperTitleHeadLook::PreUpdate(const UAnimInstance* InAnimInstance)
{
	// UObject/component access stays on the game thread. Evaluation only uses this snapshot.
	const auto* Mesh = InAnimInstance
		? Cast<UTunaSweeperTitleSkeletalMeshComponent>(InAnimInstance->GetSkelMeshComponent()) : nullptr;
	bHasTarget = Mesh && Mesh->GetDirectHeadLookRequest(Yaw, Pitch, HeadName, RootName)
		&& FMath::IsFinite(Yaw) && FMath::IsFinite(Pitch);
}

bool FAnimNode_TunaSweeperTitleHeadLook::ResolveBones(const FBoneContainer& Bones,
	FCompactPoseBoneIndex& Head, FCompactPoseBoneIndex& Root) const
{
	if (!bHasTarget || !Bones.IsValid()) return false;
	int32 HeadIndex = Bones.GetPoseBoneIndexForBoneName(HeadName);
	if (HeadIndex == INDEX_NONE) HeadIndex = Bones.GetPoseBoneIndexForBoneName(TEXT("head"));
	if (HeadIndex == INDEX_NONE) return false;
	int32 RootIndex = Bones.GetPoseBoneIndexForBoneName(RootName);
	if (RootIndex == INDEX_NONE) RootIndex = HeadIndex;
	Head = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(HeadIndex));
	Root = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(RootIndex));
	return Head != INDEX_NONE && Root != INDEX_NONE
		&& (Head == Root || Bones.BoneIsChildOf(Head, Root));
}

bool FAnimNode_TunaSweeperTitleHeadLook::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	FCompactPoseBoneIndex Head(INDEX_NONE), Root(INDEX_NONE);
	return ResolveBones(RequiredBones, Head, Root);
}

void FAnimNode_TunaSweeperTitleHeadLook::EvaluateSkeletalControl_AnyThread(
	FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	FCompactPoseBoneIndex Head(INDEX_NONE), Root(INDEX_NONE);
	if (!ResolveBones(Output.Pose.GetPose().GetBoneContainer(), Head, Root)) return;
	const float YawRadians = FMath::DegreesToRadians(Yaw);
	const float PitchRadians = FMath::DegreesToRadians(Pitch);
	const FVector DesiredAim(FMath::Sin(YawRadians) * FMath::Cos(PitchRadians),
		FMath::Cos(YawRadians) * FMath::Cos(PitchRadians), FMath::Sin(PitchRadians));
	const FVector AnimatedAim = Output.Pose.GetComponentSpaceTransform(Head).GetRotation()
		.RotateVector(FVector::RightVector).GetSafeNormal();
	const FQuat Delta = FQuat::FindBetweenNormals(AnimatedAim, DesiredAim).GetNormalized();
	FTransform RootTransform = Output.Pose.GetComponentSpaceTransform(Root);
	RootTransform.SetRotation((Delta * RootTransform.GetRotation()).GetNormalized());
	// The skeletal-control base propagates the neck rotation through its children.
	// RBAN receives the corrected head anchor and then solves the hair independently.
	OutBoneTransforms.Emplace(Root, RootTransform);
}
