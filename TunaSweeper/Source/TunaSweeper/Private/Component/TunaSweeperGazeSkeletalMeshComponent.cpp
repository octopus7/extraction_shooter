#include "Component/TunaSweeperGazeSkeletalMeshComponent.h"

#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

void UTunaSweeperGazeSkeletalMeshComponent::SetGazePoseRequest(const FTunaSweeperGazePoseRequest& Request)
{
	CurrentGazePoseRequest = Request;
}

void UTunaSweeperGazeSkeletalMeshComponent::FinalizeBoneTransform()
{
	ApplyEyeGazeToEditablePose();
	Super::FinalizeBoneTransform();
}

bool UTunaSweeperGazeSkeletalMeshComponent::IsBoneDescendantOf(int32 BoneIndex, int32 ParentBoneIndex) const
{
	const USkeletalMesh* MeshAsset = GetSkeletalMeshAsset();
	if (!MeshAsset || BoneIndex < 0 || ParentBoneIndex < 0)
	{
		return false;
	}

	const FReferenceSkeleton& ReferenceSkeleton = MeshAsset->GetRefSkeleton();
	int32 CurrentBoneIndex = BoneIndex;
	while (CurrentBoneIndex != INDEX_NONE)
	{
		if (CurrentBoneIndex == ParentBoneIndex)
		{
			return true;
		}
		CurrentBoneIndex = ReferenceSkeleton.GetParentIndex(CurrentBoneIndex);
	}
	return false;
}

void UTunaSweeperGazeSkeletalMeshComponent::ApplyEyeGazeToEditablePose()
{
	if (!bApplyDirectEyeGaze)
	{
		CurrentLeftEyeYawDegrees = 0.0f;
		CurrentLeftEyePitchDegrees = 0.0f;
		CurrentRightEyeYawDegrees = 0.0f;
		CurrentRightEyePitchDegrees = 0.0f;
		return;
	}

	TArray<FTransform>& ComponentSpaceTransforms = GetEditableComponentSpaceTransforms();
	if (ComponentSpaceTransforms.IsEmpty())
	{
		return;
	}

	ApplyEyeGazeBranch(
		ComponentSpaceTransforms,
		CurrentGazePoseRequest.LeftEyeBoneName,
		CurrentGazePoseRequest.LeftTargetWorldLocation,
		CurrentGazePoseRequest.bEnabled && CurrentGazePoseRequest.bHasLeftTarget,
		CurrentLeftEyeYawDegrees,
		CurrentLeftEyePitchDegrees);
	ApplyEyeGazeBranch(
		ComponentSpaceTransforms,
		CurrentGazePoseRequest.RightEyeBoneName,
		CurrentGazePoseRequest.RightTargetWorldLocation,
		CurrentGazePoseRequest.bEnabled && CurrentGazePoseRequest.bHasRightTarget,
		CurrentRightEyeYawDegrees,
		CurrentRightEyePitchDegrees);
}

void UTunaSweeperGazeSkeletalMeshComponent::ApplyEyeGazeBranch(
	TArray<FTransform>& ComponentSpaceTransforms,
	FName EyeBoneName,
	const FVector& TargetWorldLocation,
	bool bHasTarget,
	float& InOutCurrentYawDegrees,
	float& InOutCurrentPitchDegrees)
{
	const int32 EyeBoneIndex = GetBoneIndex(EyeBoneName);
	if (!ComponentSpaceTransforms.IsValidIndex(EyeBoneIndex))
	{
		InOutCurrentYawDegrees = 0.0f;
		InOutCurrentPitchDegrees = 0.0f;
		return;
	}

	const FTransform BaseEyeTransform = ComponentSpaceTransforms[EyeBoneIndex];
	const FTransform MeshWorldTransform = GetComponentTransform();
	const FVector EyeWorldLocation = MeshWorldTransform.TransformPosition(BaseEyeTransform.GetLocation());
	const FVector TargetOffsetWorld = TargetWorldLocation - EyeWorldLocation;
	const float MinimumDistance = FMath::Max(0.0f, CurrentGazePoseRequest.MinimumTargetDistance);
	bool bHasValidDirection = bHasTarget && TargetOffsetWorld.SizeSquared() >= FMath::Square(MinimumDistance);

	float TargetYawDegrees = 0.0f;
	float TargetPitchDegrees = 0.0f;
	if (bHasValidDirection)
	{
		const FVector DesiredComponentDirection =
			MeshWorldTransform.InverseTransformVectorNoScale(TargetOffsetWorld).GetSafeNormal();
		bHasValidDirection = TunaSweeperGaze::SolveClampedLookAngles(
			BaseEyeTransform.GetRotation(),
			CurrentGazePoseRequest.EyeAimAxis,
			CurrentGazePoseRequest.EyeUpAxis,
			DesiredComponentDirection,
			CurrentGazePoseRequest.MaxYawDegrees,
			CurrentGazePoseRequest.MaxPitchUpDegrees,
			CurrentGazePoseRequest.MaxPitchDownDegrees,
			TargetYawDegrees,
			TargetPitchDegrees);
	}

	if (bHasValidDirection)
	{
		const float Weight = FMath::Clamp(CurrentGazePoseRequest.Weight, 0.0f, 1.0f);
		TargetYawDegrees *= Weight;
		TargetPitchDegrees *= Weight;
	}
	else
	{
		TargetYawDegrees = 0.0f;
		TargetPitchDegrees = 0.0f;
	}

	const float InterpolationSpeed = bHasValidDirection
		? CurrentGazePoseRequest.TrackingInterpolationSpeed
		: CurrentGazePoseRequest.NeutralReturnInterpolationSpeed;
	const float InterpolationAlpha = TunaSweeperGaze::CalculateExponentialInterpolationAlpha(
		InterpolationSpeed,
		CurrentGazePoseRequest.DeltaSeconds);
	InOutCurrentYawDegrees = FMath::Lerp(InOutCurrentYawDegrees, TargetYawDegrees, InterpolationAlpha);
	InOutCurrentPitchDegrees = FMath::Lerp(InOutCurrentPitchDegrees, TargetPitchDegrees, InterpolationAlpha);

	if (FMath::IsNearlyZero(InOutCurrentYawDegrees, 0.01f) &&
		FMath::IsNearlyZero(InOutCurrentPitchDegrees, 0.01f))
	{
		return;
	}

	const FQuat LookDelta = TunaSweeperGaze::BuildLookDelta(
		BaseEyeTransform.GetRotation(),
		CurrentGazePoseRequest.EyeAimAxis,
		CurrentGazePoseRequest.EyeUpAxis,
		InOutCurrentYawDegrees,
		InOutCurrentPitchDegrees);
	const FVector EyeLocation = BaseEyeTransform.GetLocation();
	for (int32 BoneIndex = 0; BoneIndex < ComponentSpaceTransforms.Num(); ++BoneIndex)
	{
		if (!IsBoneDescendantOf(BoneIndex, EyeBoneIndex))
		{
			continue;
		}

		FTransform& BoneTransform = ComponentSpaceTransforms[BoneIndex];
		BoneTransform.SetLocation(
			EyeLocation + LookDelta.RotateVector(BoneTransform.GetLocation() - EyeLocation));
		BoneTransform.SetRotation((LookDelta * BoneTransform.GetRotation()).GetNormalized());
	}
}
