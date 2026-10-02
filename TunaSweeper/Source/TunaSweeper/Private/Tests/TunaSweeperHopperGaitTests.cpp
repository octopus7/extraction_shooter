#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/Hopper/HopperGait.h"

using namespace TunaSweeperHopper;

namespace
{
void UpdateOnPlane(FBipedGait& Gait, float Dt, const FVector& Location,
	const FVector& Velocity = FVector::ZeroVector, float Yaw = 0, bool bGrounded = true)
{
	const FTransform Body(FRotator(0, Yaw, 0), Location);
	FVector L = Body.TransformPosition(FVector(0, -34, -120));
	FVector R = Body.TransformPosition(FVector(0, 34, -120));
	Gait.Update(Dt, Body, Velocity, L, FVector::UpVector, R, FVector::UpVector, bGrounded);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperTwoBoneTest, "TunaSweeper.Hopper.Gait.TwoBoneReach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperTwoBoneTest::RunTest(const FString& Parameters)
{
	const auto Reachable = SolveTwoBone(FVector::ZeroVector, FVector(6, 0, 0), FVector::UpVector, 5, 5);
	TestTrue(TEXT("Reachable foot exactly matches requested target"), Reachable.Foot.Equals(FVector(6, 0, 0), .001));
	TestTrue(TEXT("Pole chooses the positive knee bend"), Reachable.Knee.Equals(FVector(3, 0, 4), .001));
	TestFalse(TEXT("Reachable target is not clamped"), Reachable.bClamped);
	const auto Far = SolveTwoBone(FVector::ZeroVector, FVector(100, 0, 0), FVector::UpVector, 5, 3);
	TestTrue(TEXT("Unreachable target clamps"), Far.bClamped);
	TestTrue(TEXT("Upper bone never stretches"), FMath::IsNearlyEqual(Far.Knee.Size(), 5., .001));
	TestTrue(TEXT("Lower bone never stretches"), FMath::IsNearlyEqual((Far.Foot - Far.Knee).Size(), 3., .001));
	const auto Near = SolveTwoBone(FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, 5, 3);
	TestTrue(TEXT("Folded unequal bones stay finite"), !Near.Knee.ContainsNaN() && !Near.Foot.ContainsNaN());
	TestTrue(TEXT("Minimum reach preserves lower bone"), FMath::IsNearlyEqual((Near.Foot - Near.Knee).Size(), 3., .001));
	const auto Folded = SolveTwoBone(FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, 5, 5);
	TestTrue(TEXT("Coincident target folds equal bones without NaN"), Folded.Foot.IsNearlyZero() && FMath::IsNearlyEqual(Folded.Knee.Size(), 5., .001));
	const auto Empty = SolveTwoBone(FVector::ZeroVector, FVector(3, 4, 5), FVector::ZeroVector, 0, 0);
	TestTrue(TEXT("Zero length bones collapse safely"), Empty.Knee.IsNearlyZero() && Empty.Foot.IsNearlyZero());
	const auto Collinear = SolveTwoBone(FVector::ZeroVector, FVector(6, 0, 0), FVector::ForwardVector, 5, 5);
	TestTrue(TEXT("Collinear pole has stable finite fallback"), !Collinear.Knee.ContainsNaN() && FMath::IsNearlyEqual(Collinear.Knee.Size(), 5., .001));
	const auto Negative = SolveTwoBone(FVector(10, 20, 30), FVector(10, 20, 26), FVector::UpVector, -5, 4);
	TestTrue(TEXT("Negative bone length is sanitized without stretching the other bone"), Negative.bClamped
		&& Negative.Knee.Equals(FVector(10, 20, 30), .001) && Negative.Foot.Equals(FVector(10, 20, 26), .001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperSlopeTest, "TunaSweeper.Hopper.Gait.SlopeNormalsAndReach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperSlopeTest::RunTest(const FString& Parameters)
{
	FBipedGait Gait;
	const FVector Normal = FVector(-.25, 0, 1).GetSafeNormal();
	const FTransform Initial(FVector(0, 0, 120));
	Gait.Update(0, Initial, FVector::ZeroVector, FVector(0, -34, 0), Normal,
		FVector(0, 34, 0), FVector::ZeroVector, true);
	TestTrue(TEXT("Initial foot sole aligns with terrain"), Gait.Feet[0].Rotation.GetUpVector().Equals(Normal, .001));
	TestTrue(TEXT("Missing normal falls back to up"), Gait.Feet[1].Normal.Equals(FVector::UpVector, .001));
	bool bLanded = false;
	for (int32 Frame = 1; Frame <= 120; ++Frame)
	{
		const double X = Frame * 70. / 60;
		const FTransform Body(FVector(X, 0, 120 + X * .25));
		Gait.Update(1.f / 60, Body, FVector(70, 0, 17.5), FVector(X, -34, X * .25), Normal,
			FVector(X, 34, X * .25), Normal, true);
		for (const FGaitFoot& Foot : Gait.Feet)
		{
			if (!Foot.bTouchdown) continue;
			bLanded = true;
			TestTrue(TEXT("Predicted touchdown remains on the sampled slope"), FMath::IsNearlyEqual(Foot.Position.Z, Foot.Position.X * .25, .01));
			TestTrue(TEXT("Foot normal and sole rotation agree at touchdown"), Foot.Rotation.GetUpVector().Equals(Normal, .001));
		}
	}
	TestTrue(TEXT("Slope motion produces a touchdown"), bLanded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperStationaryTest, "TunaSweeper.Hopper.Gait.StationaryAndDeltaTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperStationaryTest::RunTest(const FString& Parameters)
{
	FBipedGait Gait;
	UpdateOnPlane(Gait, 0, FVector(0, 0, 120));
	const FVector InitialLeft = Gait.Feet[0].Position;
	for (int32 Frame = 0; Frame < 240; ++Frame)
	{
		UpdateOnPlane(Gait, 1.f / 60, FVector(0, 0, 120));
		TestTrue(TEXT("Idle feet remain fixed"), Gait.Feet[0].Position.Equals(InitialLeft, .0001));
		TestTrue(TEXT("Idle has two planted feet"), Gait.Feet[0].bPlanted && Gait.Feet[1].bPlanted);
		TestTrue(TEXT("Idle body has no procedural bob"), Gait.BodyOffset.IsNearlyZero(.0001));
		TestFalse(TEXT("Idle has no touchdown events"), Gait.Feet[0].bTouchdown || Gait.Feet[1].bTouchdown);
	}
	UpdateOnPlane(Gait, 0, FVector(70, 0, 120), FVector(80, 0, 0));
	TestTrue(TEXT("Zero time cannot advance feet"), Gait.Feet[0].Position.Equals(InitialLeft, .0001));
	UpdateOnPlane(Gait, -1, FVector(70, 0, 120));
	TestTrue(TEXT("Negative time cannot advance feet"), Gait.Feet[0].Position.Equals(InitialLeft, .0001));
	UpdateOnPlane(Gait, 10, FVector(80, 0, 120), FVector(80, 0, 0));
	TestTrue(TEXT("Hitches leave finite feet and support"), !Gait.Feet[0].Position.ContainsNaN() && (Gait.Feet[0].bPlanted || Gait.Feet[1].bPlanted));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperMovingTest, "TunaSweeper.Hopper.Gait.WorldLockAndAlternatingStance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperMovingTest::RunTest(const FString& Parameters)
{
	FBipedGait Gait;
	UpdateOnPlane(Gait, 0, FVector(0, 0, 120));
	int32 Touchdowns = 0;
	int32 LastTouchdown = INDEX_NONE;
	for (int32 Frame = 1; Frame <= 600; ++Frame)
	{
		const FGaitFoot Previous[2] = {Gait.Feet[0], Gait.Feet[1]};
		UpdateOnPlane(Gait, 1.f / 60, FVector(Frame * 80.f / 60, 0, 120), FVector(80, 0, 0));
		TestTrue(TEXT("Heavy gait always retains one stance foot"), Gait.Feet[0].bPlanted || Gait.Feet[1].bPlanted);
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			if (Previous[Foot].bPlanted && Gait.Feet[Foot].bPlanted && !Gait.Feet[Foot].bTouchdown)
				TestTrue(TEXT("Body movement cannot slide a stance foot"), Previous[Foot].Position.Equals(Gait.Feet[Foot].Position, .001));
			if (Gait.Feet[Foot].bTouchdown)
			{
				TestTrue(TEXT("Successive footsteps alternate"), LastTouchdown != Foot);
				TestTrue(TEXT("Touchdown returns to the sampled ground plane"), FMath::IsNearlyZero(Gait.Feet[Foot].Position.Z, .01));
				LastTouchdown = Foot;
				++Touchdowns;
			}
		}
	}
	TestTrue(TEXT("Locomotion produces repeated steps"), Touchdowns >= 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperFrameRateTest, "TunaSweeper.Hopper.Gait.FrameRateIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperFrameRateTest::RunTest(const FString& Parameters)
{
	FBipedGait Slow, Fast;
	int32 Counts[2] = {0, 0};
	FBipedGait* Gaits[2] = {&Slow, &Fast};
	for (int32 Case = 0; Case < 2; ++Case)
	{
		const int32 Fps = Case == 0 ? 30 : 120;
		UpdateOnPlane(*Gaits[Case], 0, FVector(0, 0, 120));
		for (int32 Frame = 1; Frame <= Fps * 8; ++Frame)
		{
			UpdateOnPlane(*Gaits[Case], 1.f / Fps, FVector(Frame * 70. / Fps, 0, 120), FVector(70, 0, 0));
			Counts[Case] += Gaits[Case]->Feet[0].bTouchdown + Gaits[Case]->Feet[1].bTouchdown;
		}
	}
	TestEqual(TEXT("30 and 120 fps produce the same number of steps"), Counts[0], Counts[1]);
	TestTrue(TEXT("30 and 120 fps stay within one centimeter"), Slow.Feet[0].Position.Equals(Fast.Feet[0].Position, 1.) && Slow.Feet[1].Position.Equals(Fast.Feet[1].Position, 1.));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperRecenterTest, "TunaSweeper.Hopper.Gait.TurnTeleportAndAirborne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperRecenterTest::RunTest(const FString& Parameters)
{
	FBipedGait Gait;
	UpdateOnPlane(Gait, 0, FVector(0, 0, 120));
	for (int32 Frame = 0; Frame < 180; ++Frame)
		UpdateOnPlane(Gait, 1.f / 60, FVector(0, 0, 120), FVector::ZeroVector, 90);
	TestTrue(TEXT("Turn in place recenters left foot"), Gait.Feet[0].Position.Equals(FVector(34, 0, 0), .01));
	TestTrue(TEXT("Turn in place recenters right foot"), Gait.Feet[1].Position.Equals(FVector(-34, 0, 0), .01));
	UpdateOnPlane(Gait, 1.f / 60, FVector(1000, 0, 120));
	TestTrue(TEXT("Teleport resets ground anchors"), Gait.Feet[0].Position.Equals(FVector(1000, -34, 0), .01));
	TestFalse(TEXT("Teleport is not a foot impact"), Gait.Feet[0].bTouchdown || Gait.Feet[1].bTouchdown);
	UpdateOnPlane(Gait, 1.f / 60, FVector(1000, 0, 170), FVector::ZeroVector, 0, false);
	TestFalse(TEXT("Airborne feet are not planted"), Gait.Feet[0].bPlanted || Gait.Feet[1].bPlanted);
	TestTrue(TEXT("Airborne feet follow the body"), Gait.Feet[0].Position.Z >= 49);
	UpdateOnPlane(Gait, 1.f / 60, FVector(1000, 0, 120));
	TestTrue(TEXT("Landing reacquires finite stance"), Gait.Feet[0].bPlanted && Gait.Feet[1].bPlanted && !Gait.Feet[0].Position.ContainsNaN());
	Gait.Reset();
	UpdateOnPlane(Gait, 0, FVector(0, 0, 120));
	TestTrue(TEXT("Explicit reset discards old anchors"), Gait.Feet[0].Position.Equals(FVector(0, -34, 0), .01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperActualScaleTest, "TunaSweeper.Hopper.Gait.ActualScalePlantedReach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperActualScaleTest::RunTest(const FString& Parameters)
{
	// Measured model dimensions in centimeters, including the presentation's 10 cm crouch.
	const FVector RestHip(-4.5166, 39.0687, 85.8);
	const FVector RestKnee(-5.307, 44.658, 53.7965);
	const FVector RestAnkle(-.2032, 44.2628, 18.089);
	const float UpperLength = FVector::Dist(RestHip, RestKnee);
	const float LowerLength = FVector::Dist(RestKnee, RestAnkle);
	for (int32 Case = 0; Case < 4; ++Case)
	{
		const int32 Fps = Case % 2 == 0 ? 30 : 120;
		const float Speed = Case < 2 ? 35.f : 40.f;
		for (int32 Scenario = 0; Scenario < 3; ++Scenario)
		{
			FBipedGait Gait;
			Gait.Settings.HipOffsets[0] = FVector(0, 39.0687, 0);
			Gait.Settings.HipOffsets[1] = FVector(0, -39.0687, 0);
			Gait.Settings.MaxLegReach = UpperLength + LowerLength - .2f;
			Gait.Settings.StepDuration = .42f;
			Gait.Settings.StepHeight = 16.f;
			Gait.Settings.StrideTrigger = 25.f;
			Gait.Settings.PredictionTime = 0; // Caller traces the predicted landing point.
			double MaximumPlantError = 0;
			int32 Touchdowns = 0;
			for (int32 Frame = 0; Frame <= 1000; ++Frame)
			{
				const double Time = static_cast<double>(Frame) / Fps;
				const float Slope = Scenario == 2 ? .15f : 0.f;
				const float Yaw = Scenario == 1 ? static_cast<float>(Time * 30.) : 0.f;
				const FVector Origin(Time * Speed, 0, Time * Speed * Slope);
				const FTransform Ground(FRotator(0, Yaw, 0), Origin);
				const FTransform Body(Ground.GetRotation(), Ground.TransformPosition(FVector(-4.5166, 0, 75.8)));
				const FVector Velocity = Frame == 0 ? FVector::ZeroVector : FVector(Speed, 0, Speed * Slope);
				const FVector Normal = FVector(-Slope, 0, 1).GetSafeNormal();
				FVector Targets[2];
				for (int32 Foot = 0; Foot < 2; ++Foot)
				{
					FVector Sample = Ground.TransformPosition(FVector(-.2032, Foot == 0 ? 44.2628 : -44.2628, 0));
					Sample += FVector(Velocity.X, Velocity.Y, 0) * .22;
					Sample.Z = Sample.X * Slope;
					Targets[Foot] = Sample + Normal * 18.089;
				}
				Gait.Update(Frame == 0 ? 0 : 1.f / Fps, Body, Velocity, Targets[0], Normal, Targets[1], Normal, true);
				const FTransform VisualBody(FRotator(Gait.PitchLeanDegrees, Yaw, Gait.RollLeanDegrees),
					Origin + Ground.TransformVectorNoScale(Gait.BodyOffset) - FVector(0, 0, 10));
				for (int32 Foot = 0; Foot < 2; ++Foot)
				{
					const FVector Hip = VisualBody.TransformPosition(FVector(-4.5166, Foot == 0 ? 39.0687 : -39.0687, 85.8));
					const auto Solved = SolveTwoBone(Hip, Gait.Feet[Foot].Position,
						VisualBody.TransformVectorNoScale(FVector(-1, 0, 0)), UpperLength, LowerLength);
					TestTrue(TEXT("Measured model stays finite while walking, turning and climbing"),
						!Solved.Foot.ContainsNaN() && !Solved.Knee.ContainsNaN() && !Gait.Feet[Foot].Rotation.ContainsNaN());
					if (Gait.Feet[Foot].bPlanted)
						MaximumPlantError = FMath::Max(MaximumPlantError, FVector::Dist(Solved.Foot, Gait.Feet[Foot].Position));
					if (Gait.Feet[Foot].bTouchdown)
					{
						++Touchdowns;
						TestTrue(TEXT("Actual-scale ankle touchdown remains on the sampled offset plane"),
							FMath::IsNearlyEqual(FVector::DotProduct(Gait.Feet[Foot].Position, Normal), 18.089, .01));
					}
				}
			}
			TestTrue(TEXT("Measured model produces footsteps"), Touchdowns > 8);
			if (Scenario == 0)
				TestTrue(FString::Printf(TEXT("Planted rendered feet remain within 1 cm of anchor at %d fps / %.0f cm/s (max %.3f cm)"), Fps, Speed, MaximumPlantError), MaximumPlantError <= 1.);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperGroundedClampTest, "TunaSweeper.Hopper.Gait.ReachClampPreservesGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperGroundedClampTest::RunTest(const FString& Parameters)
{
	FBipedGait Gait;
	Gait.Settings.MaxLegReach = 68.4f;
	Gait.Settings.HipOffsets[0] = FVector(0, 39.0687, 0);
	Gait.Settings.HipOffsets[1] = FVector(0, -39.0687, 0);
	Gait.Settings.PredictionTime = 0;
	const FTransform Body(FVector(-4.5166, 0, 75.8));
	Gait.Update(0, Body, FVector::ZeroVector, FVector(-.2032, 44.2628, 18.089), FVector::UpVector,
		FVector(-.2032, -44.2628, 18.089), FVector::UpVector, true);
	bool bTouchedDown = false;
	for (int32 Frame = 0; Frame < 100; ++Frame)
	{
		// A distant sample must shorten the stride along the plane, never lift its endpoint.
		Gait.Update(1.f / 60, Body, FVector::ZeroVector, FVector(160, 44.2628, 18.089), FVector::UpVector,
			FVector(160, -44.2628, 18.089), FVector::UpVector, true);
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			if (!Gait.Feet[Foot].bTouchdown) continue;
			bTouchedDown = true;
			TestTrue(TEXT("Reach-clamped touchdown is still grounded"), FMath::IsNearlyEqual(Gait.Feet[Foot].Position.Z, 18.089, .001));
			TestTrue(TEXT("Ground-constrained endpoint remains reachable"), FVector::Dist(
				Body.TransformPosition(Gait.Settings.HipOffsets[Foot]), Gait.Feet[Foot].Position) <= 68.401);
		}
	}
	TestTrue(TEXT("Distant sample exercises touchdown clamp"), bTouchedDown);
	return true;
}
#endif
