#if WITH_DEV_AUTOMATION_TESTS

#include "Component/TunaSweeperGazePoseSink.h"
#include "Component/TunaSweeperGazeSkeletalMeshComponent.h"
#include "Component/TunaSweeperGazeTrackingComponent.h"
#include "Character/TunaSweeperGazeTestRobotCharacter.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "ReferenceSkeleton.h"
#include "Title/TunaSweeperTitlePresentationActor.h"

namespace TunaSweeperGazeTrackingTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	bool TestAngle(
		FAutomationTestBase& Test,
		const TCHAR* Description,
		float Actual,
		float Expected,
		float Tolerance = 0.01f)
	{
		return Test.TestTrue(
			Description,
			FMath::IsNearlyEqual(Actual, Expected, Tolerance));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGazeForwardAndIndependentTargetsTest,
	"TunaSweeper.Gaze.ForwardAndIndependentTargets",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeForwardAndIndependentTargetsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	float YawDegrees = 0.0f;
	float PitchDegrees = 0.0f;
	TestTrue(
		TEXT("A forward target produces a valid look solution"),
		TunaSweeperGaze::SolveClampedLookAngles(
			FQuat::Identity,
			FVector::ForwardVector,
			FVector::UpVector,
			FVector::ForwardVector,
			30.0f,
			20.0f,
			15.0f,
			YawDegrees,
			PitchDegrees));
	TunaSweeperGazeTrackingTests::TestAngle(*this, TEXT("Forward yaw remains neutral"), YawDegrees, 0.0f);
	TunaSweeperGazeTrackingTests::TestAngle(*this, TEXT("Forward pitch remains neutral"), PitchDegrees, 0.0f);

	float LeftYawDegrees = 0.0f;
	float LeftPitchDegrees = 0.0f;
	float RightYawDegrees = 0.0f;
	float RightPitchDegrees = 0.0f;
	TunaSweeperGaze::SolveClampedLookAngles(
		FQuat::Identity,
		FVector::ForwardVector,
		FVector::UpVector,
		FVector(1000.0f, -100.0f, 0.0f),
		30.0f,
		20.0f,
		15.0f,
		LeftYawDegrees,
		LeftPitchDegrees);
	TunaSweeperGaze::SolveClampedLookAngles(
		FQuat::Identity,
		FVector::ForwardVector,
		FVector::UpVector,
		FVector(1000.0f, 100.0f, 0.0f),
		30.0f,
		20.0f,
		15.0f,
		RightYawDegrees,
		RightPitchDegrees);
	TestTrue(TEXT("Left and right targets produce opposite eye yaw"), LeftYawDegrees < 0.0f && RightYawDegrees > 0.0f);
	TunaSweeperGazeTrackingTests::TestAngle(
		*this,
		TEXT("Separated targets are symmetric"),
		FMath::Abs(LeftYawDegrees),
		FMath::Abs(RightYawDegrees));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGazeAngleLimitsTest,
	"TunaSweeper.Gaze.AngleLimits",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeAngleLimitsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	float YawDegrees = 0.0f;
	float PitchDegrees = 0.0f;
	const FVector HighTargetDirection = FRotator(35.0f, 65.0f, 0.0f).Vector();
	TestTrue(
		TEXT("An extreme target produces a valid clamped solution"),
		TunaSweeperGaze::SolveClampedLookAngles(
			FQuat::Identity,
			FVector::ForwardVector,
			FVector::UpVector,
			HighTargetDirection,
			22.0f,
			14.0f,
			9.0f,
			YawDegrees,
			PitchDegrees));
	TunaSweeperGazeTrackingTests::TestAngle(*this, TEXT("Yaw is clamped"), YawDegrees, 22.0f);
	TunaSweeperGazeTrackingTests::TestAngle(*this, TEXT("Up pitch is clamped"), PitchDegrees, 14.0f);

	const FVector LowTargetDirection = FRotator(-35.0f, 0.0f, 0.0f).Vector();
	TunaSweeperGaze::SolveClampedLookAngles(
		FQuat::Identity,
		FVector::ForwardVector,
		FVector::UpVector,
		LowTargetDirection,
		22.0f,
		14.0f,
		9.0f,
		YawDegrees,
		PitchDegrees);
	TunaSweeperGazeTrackingTests::TestAngle(*this, TEXT("Down pitch uses its independent limit"), PitchDegrees, -9.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGazeDeltaAndInterpolationTest,
	"TunaSweeper.Gaze.DeltaAndInterpolation",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeDeltaAndInterpolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FQuat LookDelta = TunaSweeperGaze::BuildLookDelta(
		FQuat::Identity,
		FVector::ForwardVector,
		FVector::UpVector,
		20.0f,
		10.0f);
	const FVector ActualAim = LookDelta.RotateVector(FVector::ForwardVector).GetSafeNormal();
	const FVector ExpectedAim = FRotator(10.0f, 20.0f, 0.0f).Vector().GetSafeNormal();
	TestTrue(TEXT("Look delta points the eye aim axis at the requested yaw and pitch"), ActualAim.Equals(ExpectedAim, 0.001f));

	const float OneStepAlpha = TunaSweeperGaze::CalculateExponentialInterpolationAlpha(12.0f, 1.0f / 60.0f);
	TestTrue(TEXT("Positive interpolation is partial"), OneStepAlpha > 0.0f && OneStepAlpha < 1.0f);
	TestEqual(TEXT("Zero speed snaps to the target"), TunaSweeperGaze::CalculateExponentialInterpolationAlpha(0.0f, 1.0f / 60.0f), 1.0f);
	TestEqual(TEXT("Zero delta time does not advance"), TunaSweeperGaze::CalculateExponentialInterpolationAlpha(12.0f, 0.0f), 0.0f);

	float YawDegrees = 0.0f;
	float PitchDegrees = 0.0f;
	TestFalse(
		TEXT("Parallel aim and up axes are rejected"),
		TunaSweeperGaze::SolveClampedLookAngles(
			FQuat::Identity,
			FVector::ForwardVector,
			FVector::ForwardVector,
			FVector::ForwardVector,
			30.0f,
			20.0f,
			15.0f,
			YawDegrees,
			PitchDegrees));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGazeLunaEyeAxisTest,
	"TunaSweeper.Gaze.LunaEyeAxis",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeLunaEyeAxisTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const USkeletalMesh* LunaMesh = LoadObject<USkeletalMesh>(
		nullptr,
		TEXT("/Game/Characters/Player/Luna/SKM_Luna.SKM_Luna"));
	TestNotNull(TEXT("Luna skeletal mesh loads"), LunaMesh);
	if (!LunaMesh)
	{
		return false;
	}

	const FReferenceSkeleton& ReferenceSkeleton = LunaMesh->GetRefSkeleton();
	const TArray<FTransform>& LocalPose = ReferenceSkeleton.GetRefBonePose();
	TArray<FTransform> ComponentPose;
	ComponentPose.SetNum(LocalPose.Num());
	for (int32 BoneIndex = 0; BoneIndex < LocalPose.Num(); ++BoneIndex)
	{
		const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
		ComponentPose[BoneIndex] = ParentIndex == INDEX_NONE
			? LocalPose[BoneIndex]
			: LocalPose[BoneIndex] * ComponentPose[ParentIndex];
	}

	const FName EyeBoneNames[] = {TEXT("cc_base_l_eye"), TEXT("cc_base_r_eye")};
	const FName EyeEndBoneNames[] = {TEXT("cc_base_l_eye_end"), TEXT("cc_base_r_eye_end")};
	for (int32 EyeIndex = 0; EyeIndex < 2; ++EyeIndex)
	{
		const int32 EyeBoneIndex = ReferenceSkeleton.FindBoneIndex(EyeBoneNames[EyeIndex]);
		const int32 EyeEndBoneIndex = ReferenceSkeleton.FindBoneIndex(EyeEndBoneNames[EyeIndex]);
		TestTrue(TEXT("Eye bone exists"), ComponentPose.IsValidIndex(EyeBoneIndex));
		TestTrue(TEXT("Eye end bone exists"), ComponentPose.IsValidIndex(EyeEndBoneIndex));
		if (!ComponentPose.IsValidIndex(EyeBoneIndex) || !ComponentPose.IsValidIndex(EyeEndBoneIndex))
		{
			continue;
		}

		const FVector EyeToEndDirection =
			(ComponentPose[EyeEndBoneIndex].GetLocation() - ComponentPose[EyeBoneIndex].GetLocation()).GetSafeNormal();
		const FVector EyeLocalXDirection =
			ComponentPose[EyeBoneIndex].GetRotation().RotateVector(FVector::ForwardVector).GetSafeNormal();
		const FVector EyeLocalYDirection =
			ComponentPose[EyeBoneIndex].GetRotation().RotateVector(FVector::RightVector).GetSafeNormal();
		const FVector EyeLocalZDirection =
			ComponentPose[EyeBoneIndex].GetRotation().RotateVector(FVector::UpVector).GetSafeNormal();
		AddInfo(FString::Printf(
			TEXT("%s eye-end axis dots: X=%.4f Y=%.4f Z=%.4f"),
			*EyeBoneNames[EyeIndex].ToString(),
			FVector::DotProduct(EyeToEndDirection, EyeLocalXDirection),
			FVector::DotProduct(EyeToEndDirection, EyeLocalYDirection),
			FVector::DotProduct(EyeToEndDirection, EyeLocalZDirection)));
		TestTrue(
			TEXT("Luna eye bone local negative Y axis points toward its eye-end bone"),
			FVector::DotProduct(EyeToEndDirection, -EyeLocalYDirection) > 0.98f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGazeLunaMk2EyeRigTest,
	"TunaSweeper.Gaze.LunaMk2EyeRig",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeLunaMk2EyeRigTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const USkeletalMesh* LunaMk2Mesh = LoadObject<USkeletalMesh>(
		nullptr,
		TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2.SKM_LunaMk2"));
	TestNotNull(TEXT("Luna Mk2 skeletal mesh loads"), LunaMk2Mesh);
	if (!LunaMk2Mesh)
	{
		return false;
	}

	const FReferenceSkeleton& ReferenceSkeleton = LunaMk2Mesh->GetRefSkeleton();
	const int32 LeftEyeIndex = ReferenceSkeleton.FindBoneIndex(TEXT("eye_l"));
	const int32 RightEyeIndex = ReferenceSkeleton.FindBoneIndex(TEXT("eye_r"));
	TestTrue(TEXT("Luna Mk2 left eye bone exists"), LeftEyeIndex != INDEX_NONE);
	TestTrue(TEXT("Luna Mk2 right eye bone exists"), RightEyeIndex != INDEX_NONE);
	TestTrue(TEXT("Luna Mk2 head bone exists"), ReferenceSkeleton.FindBoneIndex(TEXT("Head")) != INDEX_NONE);
	if (LeftEyeIndex == INDEX_NONE || RightEyeIndex == INDEX_NONE)
	{
		return false;
	}

	const TArray<FTransform>& LocalPose = ReferenceSkeleton.GetRefBonePose();
	TArray<FTransform> ComponentPose;
	ComponentPose.SetNum(LocalPose.Num());
	for (int32 BoneIndex = 0; BoneIndex < LocalPose.Num(); ++BoneIndex)
	{
		const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
		ComponentPose[BoneIndex] = ParentIndex == INDEX_NONE
			? LocalPose[BoneIndex]
			: LocalPose[BoneIndex] * ComponentPose[ParentIndex];
	}

	const FVector EyeLateralDirection = FVector(
		ComponentPose[LeftEyeIndex].GetLocation() - ComponentPose[RightEyeIndex].GetLocation()).GetSafeNormal2D();
	const FVector FaceForwardDirection = FVector::CrossProduct(FVector::UpVector, EyeLateralDirection).GetSafeNormal();
	for (const int32 EyeIndex : {LeftEyeIndex, RightEyeIndex})
	{
		const FQuat EyeRotation = ComponentPose[EyeIndex].GetRotation();
		const FVector EyeAimDirection = EyeRotation.RotateVector(FVector::RightVector).GetSafeNormal();
		const FVector EyeUpDirection = EyeRotation.RotateVector(FVector::UpVector).GetSafeNormal();
		TestTrue(
			TEXT("Luna Mk2 eye local positive Y axis points toward the face forward direction"),
			FVector::DotProduct(EyeAimDirection, FaceForwardDirection) > 0.9f);
		TestTrue(
			TEXT("Luna Mk2 eye local Z axis remains upright"),
			FVector::DotProduct(EyeUpDirection, FVector::UpVector) > 0.9f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGazeTitleHierarchyTest,
	"TunaSweeper.Gaze.TitleHierarchy",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeTitleHierarchyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	ATunaSweeperTitlePresentationActor* TitleActor = GetMutableDefault<ATunaSweeperTitlePresentationActor>();
	TestNotNull(TEXT("Title presentation actor default object exists"), TitleActor);
	if (!TitleActor)
	{
		return false;
	}

	UTunaSweeperGazeTrackingComponent* GazeTracking =
		TitleActor->FindComponentByClass<UTunaSweeperGazeTrackingComponent>();
	UTunaSweeperTitleSkeletalMeshComponent* BodyMesh =
		TitleActor->FindComponentByClass<UTunaSweeperTitleSkeletalMeshComponent>();
	TestNotNull(TEXT("Title actor owns the gaze tracking component"), GazeTracking);
	TestNotNull(TEXT("Title body mesh exists"), BodyMesh);
	if (!GazeTracking || !BodyMesh)
	{
		return false;
	}

	const USkeletalMesh* LunaMk2Mesh = LoadObject<USkeletalMesh>(
		nullptr,
		TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2.SKM_LunaMk2"));
	TestNotNull(TEXT("Luna Mk2 skeletal mesh loads"), LunaMk2Mesh);
	TestTrue(TEXT("Title body uses Luna Mk2"), BodyMesh->GetSkeletalMeshAsset() == LunaMk2Mesh);
	TestTrue(
		TEXT("Title body uses the Luna Mk2 animation Blueprint"),
		BodyMesh->GetAnimClass() && BodyMesh->GetAnimClass()->GetPathName().Contains(TEXT("ABP_LunaMk2")));
	TestFalse(
		TEXT("Title body preserves the Luna Mk2 animation pose instead of forcing the old relaxed-arm override"),
		BodyMesh->IsTemporaryRelaxedArmPoseEnabled());
	TestEqual(TEXT("Title gaze uses the Luna Mk2 left eye bone"), GazeTracking->GetLeftEyeBoneName(), FName(TEXT("eye_l")));
	TestEqual(TEXT("Title gaze uses the Luna Mk2 right eye bone"), GazeTracking->GetRightEyeBoneName(), FName(TEXT("eye_r")));
	TestTrue(
		TEXT("Title gaze uses the Luna Mk2 positive-Y eye aim axis"),
		GazeTracking->GetEyeAimAxis().Equals(FVector::RightVector, 0.001f));
	TestTrue(
		TEXT("Title gaze preserves the eye local Z up axis"),
		GazeTracking->GetEyeUpAxis().Equals(FVector::UpVector, 0.001f));

	USceneComponent* LeftEyeTarget = nullptr;
	USceneComponent* RightEyeTarget = nullptr;
	USceneComponent* HeadLookTarget = nullptr;
	USkeletalMeshComponent* SkirtMesh = nullptr;
	TArray<USceneComponent*> SceneComponents;
	TitleActor->GetComponents(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!SceneComponent)
		{
			continue;
		}

		if (SceneComponent->GetFName() == TEXT("LeftEyeTarget"))
		{
			LeftEyeTarget = SceneComponent;
		}
		else if (SceneComponent->GetFName() == TEXT("RightEyeTarget"))
		{
			RightEyeTarget = SceneComponent;
		}
		else if (SceneComponent->GetFName() == TEXT("HeadLookTarget"))
		{
			HeadLookTarget = SceneComponent;
		}
		else if (SceneComponent->GetFName() == TEXT("Skirt"))
		{
			SkirtMesh = Cast<USkeletalMeshComponent>(SceneComponent);
		}
	}

	TestNotNull(TEXT("Left eye target exists"), LeftEyeTarget);
	TestNotNull(TEXT("Right eye target exists"), RightEyeTarget);
	TestNotNull(TEXT("Head look target remains separate"), HeadLookTarget);
	TestNotNull(TEXT("Title Luna Mk2 has the player skirt component"), SkirtMesh);
	if (LeftEyeTarget && RightEyeTarget && HeadLookTarget)
	{
		TestEqual(
			TEXT("Left eye target is a gaze child"),
			static_cast<const USceneComponent*>(LeftEyeTarget->GetAttachParent()),
			static_cast<const USceneComponent*>(GazeTracking));
		TestEqual(
			TEXT("Right eye target is a gaze child"),
			static_cast<const USceneComponent*>(RightEyeTarget->GetAttachParent()),
			static_cast<const USceneComponent*>(GazeTracking));
		TestTrue(
			TEXT("Gaze target is not parented to the head target"),
			GazeTracking->GetAttachParent() != HeadLookTarget);
		TestTrue(
			TEXT("Eye targets have mirrored lateral offsets"),
			FMath::IsNearlyEqual(
				LeftEyeTarget->GetRelativeLocation().Y,
				-RightEyeTarget->GetRelativeLocation().Y,
				0.01f));
	}
	TestTrue(
		TEXT("Title body mesh implements the gaze pose sink"),
		BodyMesh->GetClass()->ImplementsInterface(UTunaSweeperGazePoseSink::StaticClass()));
	if (SkirtMesh)
	{
		const USkeletalMesh* PlayerSkirtMesh = LoadObject<USkeletalMesh>(
			nullptr,
			TEXT("/Game/Characters/Player/Luna/Skirt/Luna__Skirt_front.Luna__Skirt_front"));
		TestTrue(TEXT("Title skirt uses the player skirt mesh"), SkirtMesh->GetSkeletalMeshAsset() == PlayerSkirtMesh);
		TestTrue(
			TEXT("Title skirt is attached to the Mk2 body like the player Blueprint"),
			SkirtMesh->GetAttachParent() == BodyMesh);
		TestEqual(
			TEXT("Title skirt uses the player Blueprint pelvis socket"),
			SkirtMesh->GetAttachSocketName(),
			FName(TEXT("pelvis")));
		TestTrue(
			TEXT("Title skirt copies the player Blueprint position correction"),
			SkirtMesh->GetRelativeLocation().Equals(FVector(6.012824f, 5.358606f, -1.723956f), 0.001f));
		TestTrue(
			TEXT("Title skirt copies the player Blueprint rotation correction"),
			SkirtMesh->GetRelativeRotation().Equals(FRotator(-90.0f, 34.0f, 0.0f), 0.01f));
		TestTrue(
			TEXT("Title skirt uses the same animation Blueprint as the player skirt"),
			SkirtMesh->GetAnimClass() && SkirtMesh->GetAnimClass()->GetPathName().Contains(TEXT("ABP_Luna_Skirt")));
	}

	const FVector CursorTarget = ATunaSweeperTitlePresentationActor::CalculateCursorTargetWorldLocation(
		FVector::ZeroVector,
		FVector::ForwardVector,
		FVector(650.0f, 0.0f, 0.0f),
		250.0f,
		50.0f);
	TestTrue(
		TEXT("Title cursor target stays on the cursor ray in front of Luna Mk2"),
		CursorTarget.Equals(FVector(400.0f, 0.0f, 0.0f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGazeTestRobotRigTest,
	"TunaSweeper.Gaze.TestRobotRigAndPlacement",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeTestRobotRigTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const USkeletalMesh* RobotMesh = LoadObject<USkeletalMesh>(
		nullptr,
		TEXT("/Game/Characters/Test/GazeRobot/SKM_GazeTestRobot.SKM_GazeTestRobot"));
	TestNotNull(TEXT("Gaze test robot skeletal mesh loads"), RobotMesh);
	if (!RobotMesh)
	{
		return false;
	}
	const float RawMeshHalfHeight = RobotMesh->GetImportedBounds().BoxExtent.Z;
	TestTrue(
		TEXT("Robot raw mesh preserves the stable metre-authored FBX scale"),
		RawMeshHalfHeight >= 0.8f && RawMeshHalfHeight <= 1.0f);

	const FReferenceSkeleton& ReferenceSkeleton = RobotMesh->GetRefSkeleton();
	TestEqual(TEXT("Robot rig contains exactly the requested six bones"), ReferenceSkeleton.GetNum(), 6);
	const FName ExpectedBoneNames[] = {
		TEXT("root"), TEXT("body"), TEXT("neck"), TEXT("head"), TEXT("left_eye"), TEXT("right_eye")};
	const int32 ExpectedParentIndices[] = {INDEX_NONE, 0, 1, 2, 3, 3};
	for (int32 BoneIndex = 0; BoneIndex < UE_ARRAY_COUNT(ExpectedBoneNames); ++BoneIndex)
	{
		TestEqual(
			*FString::Printf(TEXT("Bone %d uses the requested name"), BoneIndex),
			ReferenceSkeleton.GetBoneName(BoneIndex),
			ExpectedBoneNames[BoneIndex]);
		TestEqual(
			*FString::Printf(TEXT("Bone %s has the requested parent"), *ExpectedBoneNames[BoneIndex].ToString()),
			ReferenceSkeleton.GetParentIndex(BoneIndex),
			ExpectedParentIndices[BoneIndex]);
	}

	const TArray<FTransform>& LocalPose = ReferenceSkeleton.GetRefBonePose();
	TArray<FTransform> ComponentPose;
	ComponentPose.SetNum(LocalPose.Num());
	for (int32 BoneIndex = 0; BoneIndex < LocalPose.Num(); ++BoneIndex)
	{
		const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
		ComponentPose[BoneIndex] = ParentIndex == INDEX_NONE
			? LocalPose[BoneIndex]
			: LocalPose[BoneIndex] * ComponentPose[ParentIndex];
	}

	for (const FName EyeBoneName : {FName(TEXT("left_eye")), FName(TEXT("right_eye"))})
	{
		const int32 EyeBoneIndex = ReferenceSkeleton.FindBoneIndex(EyeBoneName);
		if (!ComponentPose.IsValidIndex(EyeBoneIndex))
		{
			continue;
		}
		const FQuat EyeRotation = ComponentPose[EyeBoneIndex].GetRotation();
		const float XDot = FVector::DotProduct(EyeRotation.RotateVector(FVector::ForwardVector), FVector::ForwardVector);
		const float YRightDot =
			FVector::DotProduct(EyeRotation.RotateVector(FVector::RightVector), FVector::RightVector);
		const float ZUpDot = FVector::DotProduct(EyeRotation.RotateVector(FVector::UpVector), FVector::UpVector);
		AddInfo(FString::Printf(
			TEXT("%s basis dots: local-X/forward=%.4f local-Y/right=%.4f local-Z/up=%.4f"),
			*EyeBoneName.ToString(), XDot, YRightDot, ZUpDot));
		TestTrue(TEXT("Robot eye local negative Y axis points toward the pupils"), YRightDot < -0.98f);
		TestTrue(TEXT("Robot eye local Z axis points toward actor up"), ZUpDot > 0.98f);
	}

	const FVector CursorRayOrigin = FVector::ZeroVector;
	const FVector EyeCenter(600.0f, 0.0f, 0.0f);
	const FVector CenterCursorTarget = ATunaSweeperGazeTestRobotCharacter::CalculateCursorTargetWorldLocation(
		CursorRayOrigin, FVector::ForwardVector, EyeCenter, 250.0f, 50.0f);
	TestTrue(
		TEXT("Center cursor target stays on the cursor ray in front of the eyes"),
		CenterCursorTarget.Equals(FVector(350.0f, 0.0f, 0.0f), 0.01f));
	const FVector LeftCursorTarget = ATunaSweeperGazeTestRobotCharacter::CalculateCursorTargetWorldLocation(
		CursorRayOrigin, FVector(1.0f, -0.1f, 0.0f), EyeCenter, 250.0f, 50.0f);
	const FVector RightCursorTarget = ATunaSweeperGazeTestRobotCharacter::CalculateCursorTargetWorldLocation(
		CursorRayOrigin, FVector(1.0f, 0.1f, 0.0f), EyeCenter, 250.0f, 50.0f);
	TestTrue(TEXT("Left cursor ray produces a left-side target"), LeftCursorTarget.Y < 0.0f);
	TestTrue(TEXT("Right cursor ray produces a right-side target"), RightCursorTarget.Y > 0.0f);

	TestEqual(TEXT("Robot mesh preserves five contrasting material slots"), RobotMesh->GetMaterials().Num(), 5);
	for (const FSkeletalMaterial& MaterialSlot : RobotMesh->GetMaterials())
	{
		TestNotNull(TEXT("Every robot material slot is assigned"), MaterialSlot.MaterialInterface.Get());
	}

	const ATunaSweeperGazeTestRobotCharacter* RobotDefaults = GetDefault<ATunaSweeperGazeTestRobotCharacter>();
	TestNotNull(TEXT("Gaze test robot character defaults exist"), RobotDefaults);
	if (RobotDefaults)
	{
		const UTunaSweeperGazeSkeletalMeshComponent* GazeMesh =
			Cast<UTunaSweeperGazeSkeletalMeshComponent>(RobotDefaults->GetMesh());
		const UTunaSweeperGazeTrackingComponent* GazeTracking =
			RobotDefaults->FindComponentByClass<UTunaSweeperGazeTrackingComponent>();
		TestNotNull(TEXT("Robot Character uses the reusable gaze skeletal mesh component"), GazeMesh);
		TestNotNull(TEXT("Robot Character owns an independent gaze tracking component"), GazeTracking);
		const UCharacterMovementComponent* Movement = RobotDefaults->GetCharacterMovement();
		TestNotNull(TEXT("Robot Character retains a movement component"), Movement);
		if (Movement)
		{
			TestEqual(TEXT("Robot test actor has no gravity"), Movement->GravityScale, 0.0f);
			TestTrue(
				TEXT("Robot test actor defaults to no land movement"),
				Movement->DefaultLandMovementMode == MOVE_None);
			TestTrue(
				TEXT("Robot test actor movement starts disabled"),
				Movement->MovementMode == MOVE_None);
		}
		if (GazeMesh)
		{
			TestTrue(
				TEXT("Robot Character defaults assign the generated skeletal mesh"),
				GazeMesh->GetSkeletalMeshAsset() == RobotMesh);
			TestTrue(
				TEXT("Robot mesh component converts the metre-authored rig to Unreal centimetres"),
				GazeMesh->GetRelativeScale3D().Equals(FVector(100.0f), 0.01f));
			const float EffectiveMeshHeight =
				RawMeshHalfHeight * 2.0f * GazeMesh->GetRelativeScale3D().Z;
			TestTrue(
				TEXT("Robot effective height remains visible inside the Character capsule"),
				EffectiveMeshHeight >= 170.0f && EffectiveMeshHeight <= 190.0f);
		}
		if (GazeTracking)
		{
			const USceneComponent* LeftTarget = nullptr;
			const USceneComponent* RightTarget = nullptr;
			TArray<USceneComponent*> SceneComponents;
			RobotDefaults->GetComponents(SceneComponents);
			for (const USceneComponent* Component : SceneComponents)
			{
				if (Component && Component->GetFName() == TEXT("LeftEyeTarget"))
				{
					LeftTarget = Component;
				}
				else if (Component && Component->GetFName() == TEXT("RightEyeTarget"))
				{
					RightTarget = Component;
				}
			}
			TestNotNull(TEXT("Robot has a left eye target"), LeftTarget);
			TestNotNull(TEXT("Robot has a right eye target"), RightTarget);
			if (LeftTarget && RightTarget)
			{
				TestEqual(
					TEXT("Left eye target is a gaze child"),
					static_cast<const USceneComponent*>(LeftTarget->GetAttachParent()),
					static_cast<const USceneComponent*>(GazeTracking));
				TestEqual(
					TEXT("Right eye target is a gaze child"),
					static_cast<const USceneComponent*>(RightTarget->GetAttachParent()),
					static_cast<const USceneComponent*>(GazeTracking));
				TestTrue(
					TEXT("Robot eye targets preserve the authored 40 cm separation"),
					FMath::IsNearlyEqual(
						RightTarget->GetRelativeLocation().Y - LeftTarget->GetRelativeLocation().Y,
						40.0f,
						0.01f));
			}
		}
	}

	const UBlueprint* RobotBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/Characters/Test/GazeRobot/BP_GazeTestRobot.BP_GazeTestRobot"));
	TestNotNull(TEXT("BP_GazeTestRobot exists"), RobotBlueprint);
	const UWorld* IntroWorld = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/IntroMap.IntroMap"));
	TestNotNull(TEXT("IntroMap loads for placement validation"), IntroWorld);
	int32 PlacedRobotCount = 0;
	if (IntroWorld && IntroWorld->PersistentLevel)
	{
		for (const AActor* Actor : IntroWorld->PersistentLevel->Actors)
		{
			if (Cast<ATunaSweeperGazeTestRobotCharacter>(Actor))
			{
				++PlacedRobotCount;
			}
		}
	}
	TestEqual(TEXT("IntroMap contains one placed gaze test robot Blueprint"), PlacedRobotCount, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
