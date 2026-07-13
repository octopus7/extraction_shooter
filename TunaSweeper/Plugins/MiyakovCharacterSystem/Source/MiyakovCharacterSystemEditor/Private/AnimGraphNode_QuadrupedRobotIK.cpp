#include "AnimGraphNode_QuadrupedRobotIK.h"

#include "Animation/Skeleton.h"
#include "Kismet2/CompilerResultsLog.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_QuadrupedRobotIK"

FText UAnimGraphNode_QuadrupedRobotIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
	{
		return GetControllerDescription();
	}

	if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		const FText ProfileName = Node.RigProfile
			? FText::FromString(Node.RigProfile->GetName())
			: LOCTEXT("NoProfile", "No Rig Profile");
		CachedNodeTitles.SetCachedTitle(
			TitleType,
			FText::Format(LOCTEXT("NodeTitle", "Quadruped Robot IK\n{0}"), ProfileName),
			this);
	}

	return CachedNodeTitles[TitleType];
}

FText UAnimGraphNode_QuadrupedRobotIK::GetTooltipText() const
{
	return LOCTEXT(
		"Tooltip",
		"Solves four two-link robot legs from the foot targets published by a UQuadrupedComponent on the owning actor.");
}

void UAnimGraphNode_QuadrupedRobotIK::ValidateAnimNodeDuringCompilation(
	USkeleton* ForSkeleton,
	FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);

	if (!Node.RigProfile)
	{
		MessageLog.Error(TEXT("@@ requires a Quadruped Rig Profile."), this);
		return;
	}

	if (!Node.RigProfile->TargetSkeleton)
	{
		MessageLog.Error(TEXT("@@ Rig Profile requires a Target Skeleton."), this);
		return;
	}

	if (!Node.RigProfile->HasCompleteFourLegBinding())
	{
		MessageLog.Error(TEXT("@@ requires exactly one complete binding for every quadruped leg slot."), this);
	}

	if (Node.RigProfile->TargetSkeleton && ForSkeleton != Node.RigProfile->TargetSkeleton)
	{
		MessageLog.Error(TEXT("@@ Rig Profile targets a different Skeleton."), this);
		return;
	}

	if (!ForSkeleton)
	{
		return;
	}

	const FReferenceSkeleton& ReferenceSkeleton = ForSkeleton->GetReferenceSkeleton();
	for (const FQuadrupedLimbRigBinding& Limb : Node.RigProfile->Limbs)
	{
		const int32 UpperIndex = ReferenceSkeleton.FindBoneIndex(Limb.UpperBone);
		const int32 LowerIndex = ReferenceSkeleton.FindBoneIndex(Limb.LowerBone);
		const int32 FootIndex = ReferenceSkeleton.FindBoneIndex(Limb.FootBone);
		if (UpperIndex == INDEX_NONE || LowerIndex == INDEX_NONE || FootIndex == INDEX_NONE)
		{
			MessageLog.Error(
				*FString::Printf(
					TEXT("@@ Rig Profile contains a missing bone in leg slot %d."),
					static_cast<int32>(Limb.Slot)),
				this);
			continue;
		}

		if (ReferenceSkeleton.GetParentIndex(FootIndex) != LowerIndex ||
			ReferenceSkeleton.GetParentIndex(LowerIndex) != UpperIndex)
		{
			MessageLog.Error(
				*FString::Printf(
					TEXT("@@ leg slot %d must use a direct Upper -> Lower -> Foot hierarchy."),
					static_cast<int32>(Limb.Slot)),
				this);
		}
	}
}

FText UAnimGraphNode_QuadrupedRobotIK::GetControllerDescription() const
{
	return LOCTEXT("ControllerDescription", "Quadruped Robot IK");
}

#undef LOCTEXT_NAMESPACE
