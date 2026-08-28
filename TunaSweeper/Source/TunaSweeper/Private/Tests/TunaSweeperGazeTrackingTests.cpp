#if WITH_DEV_AUTOMATION_TESTS

#include "Component/TunaSweeperGazePoseSink.h"
#include "Component/TunaSweeperGazeSkeletalMeshComponent.h"
#include "Component/TunaSweeperGazeTrackingComponent.h"
#include "Character/TunaSweeperGazeTestRobotCharacter.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
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
	FTunaSweeperGazeTitleHierarchyTest,
	"TunaSweeper.Gaze.TitleHierarchy",
	TunaSweeperGazeTrackingTests::TestFlags)

bool FTunaSweeperGazeTitleHierarchyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ATunaSweeperTitlePresentationActor* TitleActor = GetDefault<ATunaSweeperTitlePresentationActor>();
	TestNotNull(TEXT("Title presentation actor default object exists"), TitleActor);
	if (!TitleActor)
	{
		return false;
	}

	const UTunaSweeperGazeTrackingComponent* GazeTracking =
		TitleActor->FindComponentByClass<UTunaSweeperGazeTrackingComponent>();
	const UTunaSweeperTitleSkeletalMeshComponent* BodyMesh =
		TitleActor->FindComponentByClass<UTunaSweeperTitleSkeletalMeshComponent>();
	TestNotNull(TEXT("Title actor owns the gaze tracking component"), GazeTracking);
	TestNotNull(TEXT("Title body mesh exists"), BodyMesh);
	if (!GazeTracking || !BodyMesh)
	{
		return false;
	}

	const USceneComponent* LeftEyeTarget = nullptr;
	const USceneComponent* RightEyeTarget = nullptr;
	const USceneComponent* HeadLookTarget = nullptr;
	TArray<USceneComponent*> SceneComponents;
	TitleActor->GetComponents(SceneComponents);
	for (const USceneComponent* SceneComponent : SceneComponents)
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
	}

	TestNotNull(TEXT("Left eye target exists"), LeftEyeTarget);
	TestNotNull(TEXT("Right eye target exists"), RightEyeTarget);
	TestNotNull(TEXT("Head look target remains separate"), HeadLookTarget);
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
		TestNotEqual(
			TEXT("Gaze target is not parented to the head target"),
			static_cast<const USceneComponent*>(GazeTracking->GetAttachParent()),
			HeadLookTarget);
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
		const float YDot = FVector::DotProduct(EyeRotation.RotateVector(FVector::RightVector), FVector::ForwardVector);
		const float ZDot = FVector::DotProduct(EyeRotation.RotateVector(FVector::UpVector), FVector::ForwardVector);
		const float ZUpDot = FVector::DotProduct(EyeRotation.RotateVector(FVector::UpVector), FVector::UpVector);
		AddInfo(FString::Printf(
			TEXT("%s actor-forward axis dots: X=%.4f Y=%.4f Z=%.4f; local-Z up dot=%.4f"),
			*EyeBoneName.ToString(), XDot, YDot, ZDot, ZUpDot));
		TestTrue(TEXT("Robot eye local negative X axis points toward actor forward"), XDot < -0.98f);
		TestTrue(TEXT("Robot eye local Z axis points toward actor up"), ZUpDot > 0.98f);
	}

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
