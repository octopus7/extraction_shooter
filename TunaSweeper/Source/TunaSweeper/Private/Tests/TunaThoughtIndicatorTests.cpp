#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/WidgetComponent.h"
#include "UI/TunaThoughtIndicatorActor.h"
#include "UI/TunaThoughtIndicatorWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaThoughtIndicatorDefaultsTest,
	"TunaSweeper.UI.TunaThoughtIndicator.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaThoughtIndicatorDefaultsTest::RunTest(const FString& Parameters)
{
	const ATunaThoughtIndicatorActor* Defaults = GetDefault<ATunaThoughtIndicatorActor>();
	if (!TestNotNull(TEXT("Actor defaults exist"), Defaults))
	{
		return false;
	}

	const UWidgetComponent* WidgetComponent = Defaults->GetIndicatorWidgetComponent();
	if (!TestNotNull(TEXT("Indicator widget component exists"), WidgetComponent))
	{
		return false;
	}

	TestEqual(TEXT("Widget uses screen space"), WidgetComponent->GetWidgetSpace(), EWidgetSpace::Screen);
	TestTrue(
		TEXT("Widget uses the native tuna thought class"),
		WidgetComponent->GetWidgetClass() == UTunaThoughtIndicatorWidget::StaticClass());
	TestEqual(TEXT("Widget pivot anchors at the tail"), WidgetComponent->GetPivot(), FVector2D(0.5f, 1.0f));
	TestEqual(TEXT("Indicator image size"), Defaults->GetIndicatorImageSizePixels(), FVector2D(176.0f, 176.0f));
	TestEqual(TEXT("Default bob amplitude"), Defaults->GetBobAmplitudePixels(), 8.0f);
	TestEqual(TEXT("Default bob frequency"), Defaults->GetBobCyclesPerSecond(), 0.55f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaThoughtIndicatorMotionTest,
	"TunaSweeper.UI.TunaThoughtIndicator.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaThoughtIndicatorMotionTest::RunTest(const FString& Parameters)
{
	const ATunaThoughtIndicatorActor* Defaults = GetDefault<ATunaThoughtIndicatorActor>();
	if (!TestNotNull(TEXT("Actor defaults exist"), Defaults))
	{
		return false;
	}

	const float Frequency = Defaults->GetBobCyclesPerSecond();
	const float QuarterPeriod = 1.0f / (Frequency * 4.0f);
	TestTrue(TEXT("Bob starts at center"), FMath::IsNearlyZero(Defaults->CalculateBobOffsetPixels(0.0f), KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("Bob reaches positive amplitude at quarter period"),
		FMath::IsNearlyEqual(
			Defaults->CalculateBobOffsetPixels(QuarterPeriod),
			Defaults->GetBobAmplitudePixels(),
			KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("Bob returns to center at half period"),
		FMath::IsNearlyZero(Defaults->CalculateBobOffsetPixels(QuarterPeriod * 2.0f), KINDA_SMALL_NUMBER));

	return true;
}

#endif
