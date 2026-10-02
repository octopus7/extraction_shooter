#include "TunaRocketFlight.h"
#include "Misc/AutomationTest.h"
#include <limits>
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketTurnTest, "TunaGuidedRocket.Flight.TurnLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketTurnTest::RunTest(const FString& Parameters)
{
    const FVector Result = TunaRocketFlight::Turn(FVector::ForwardVector, FVector::RightVector, 30.f, 0.1f);
    const double Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Result, FVector::ForwardVector)));
    TestTrue(TEXT("30 degrees/s turns exactly 3 degrees in 0.1 seconds"), FMath::IsNearlyEqual(Angle, 3.0, 0.0001));
    TestTrue(TEXT("Direction remains unit length"), Result.IsUnit());
    TestTrue(TEXT("Zero rate flies straight"), TunaRocketFlight::Turn(FVector::ForwardVector, FVector::RightVector, 0, 1).Equals(FVector::ForwardVector));
    TestTrue(TEXT("No overshoot near target"), TunaRocketFlight::Turn(FVector::ForwardVector, Result, 90, 1).Equals(Result));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketGuidanceTest, "TunaGuidedRocket.Flight.GuidanceWindow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketGuidanceTest::RunTest(const FString& Parameters)
{
    auto Guide = [](float Age, bool Lost, FVector Target) { return TunaRocketFlight::CanGuide(Age, .15f, 1.1f, Lost, FVector::ForwardVector, Target, 65); };
    TestFalse(TEXT("Launch delay"), Guide(0, false, FVector::ForwardVector));
    TestTrue(TEXT("Guides during its window"), Guide(.5f, false, FVector::ForwardVector));
    TestFalse(TEXT("Guidance expires"), Guide(1.25f, false, FVector::ForwardVector));
    TestFalse(TEXT("Cannot reacquire after loss"), Guide(.5f, true, FVector::ForwardVector));
    TestFalse(TEXT("No U turn"), Guide(.5f, false, -FVector::ForwardVector));
    TestFalse(TEXT("Coincident target"), Guide(.5f, false, FVector::ZeroVector));
    TestEqual(TEXT("Invalid number fallback"), TunaRocketFlight::FiniteClamp(std::numeric_limits<float>::infinity(), 3, .05f, 30), 3.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketDodgeTest, "TunaGuidedRocket.Flight.EarlySidestep", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketDodgeTest::RunTest(const FString& Parameters)
{
    auto Simulate = [](float Dt)
    {
        FVector Position = FVector::ZeroVector, Forward = FVector::ForwardVector;
        float Closest = 1.e9f;
        bool Lost = false;
        for (int32 Step = 0; Step < FMath::RoundToInt(3.f / Dt); ++Step)
        {
            const float Age = Step * Dt;
            const FVector Target(1800, 400 * Age, 0);
            const bool Guide = TunaRocketFlight::CanGuide(Age, .15f, 1.1f, Lost, Forward, Target - Position, 65);
            if (Age >= .15f && !Guide) Lost = true;
            if (Guide) Forward = TunaRocketFlight::Turn(Forward, Target - Position, 30, Dt);
            Position += Forward * 1200 * Dt;
            Closest = FMath::Min(Closest, float(FVector::Distance(Position, Target)));
        }
        return Closest;
    };
    const float Clearance = Simulate(1.f / 120);
    TestTrue(TEXT("Early sidestep clears 1m blast plus pawn radius"), Clearance > 134);
    TestTrue(TEXT("Consistent at 60 and 120 Hz"), FMath::Abs(Clearance - Simulate(1.f / 60)) < 25);
    return true;
}
#endif
