#if WITH_DEV_AUTOMATION_TESTS
#include "UI/TunaSweeperDeliveryIndicator.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperDeliveryIndicatorLayoutTest,
    "TunaSweeper.UI.DeliveryIndicator.Layout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperDeliveryIndicatorLayoutTest::RunTest(const FString& Parameters)
{
    using namespace TunaSweeperDeliveryIndicator;
    FPlacement Rest, Peak, OnScreen;
    const FVector2D Size(1920, 1080);
    TestTrue(TEXT("Offscreen destination produces a marker"), ComputePlacement(Size, FVector2D(3000, 540), true, 0, Rest));
    TestTrue(TEXT("Offscreen marker is on the inset edge"), Rest.bAtEdge);
    TestEqual(TEXT("Marker center has an 80-unit border gap"), Rest.Position.X, 1840.0);
    TestTrue(TEXT("Arrow points toward destination"), Rest.Direction.Equals(FVector2D(1, 0)));
    TestTrue(TEXT("Bounce frame produces a marker"), ComputePlacement(Size, FVector2D(3000, 540), true, 0.24f, Peak));
    TestTrue(TEXT("Bounce moves inward instead of touching border"), Peak.Position.X < Rest.Position.X - 10);
    TestTrue(TEXT("Bounce never rotates arrow"), Peak.Direction.Equals(Rest.Direction));
    TestTrue(TEXT("Onscreen destination produces a marker"), ComputePlacement(Size, FVector2D(700, 400), true, 0, OnScreen));
    TestFalse(TEXT("Onscreen marker is not an edge arrow"), OnScreen.bAtEdge);
    TestTrue(TEXT("Onscreen marker follows destination"), OnScreen.Position.Equals(FVector2D(700, 400)));
    ComputePlacement(Size, FVector2D(700, 400), true, 0.24f, Peak);
    TestTrue(TEXT("Onscreen marker stays stable"), Peak.Position.Equals(OnScreen.Position));
    ComputePlacement(Size, FVector2D(1000, 540), false, 0, Peak);
    TestTrue(TEXT("Behind-camera destination stays an edge arrow"), Peak.bAtEdge);
    TestTrue(TEXT("Behind-camera direction is not mirrored"), Peak.Direction.Equals(FVector2D(1, 0)));
    TestFalse(TEXT("Zero viewport cannot draw"), ComputePlacement(FVector2D::ZeroVector, FVector2D::ZeroVector, true, 0, Peak));

    for (const FVector2D View : {FVector2D(1280,720), FVector2D(3440,1440), FVector2D(600,1000)})
    {
        for (int32 Angle = 0; Angle < 360; Angle += 15)
        {
            const double Radians = FMath::DegreesToRadians(static_cast<double>(Angle));
            const FVector2D Direction(FMath::Cos(Radians), FMath::Sin(Radians));
            for (int32 Frame = 0; Frame < 60; ++Frame)
            {
                FPlacement P;
                TestTrue(TEXT("Every direction and bounce phase resolves"), ComputePlacement(View, View * .5 + Direction * 10000, true, Frame / 50.0f, P));
                TestTrue(TEXT("Whole marker stays clear of screen border"),
                    P.Position.X >= 79.9 && P.Position.Y >= 79.9 &&
                    P.Position.X <= View.X - 79.9 && P.Position.Y <= View.Y - 79.9);
                TestTrue(TEXT("Aspect ratio preserves screen-space bearing"), P.Direction.Equals(Direction, 0.001));
            }
        }
    }
    return true;
}
#endif
