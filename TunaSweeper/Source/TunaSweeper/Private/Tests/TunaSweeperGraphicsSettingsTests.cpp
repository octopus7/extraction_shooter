#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Settings/TunaSweeperGameUserSettings.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"

namespace TunaSweeperGraphicsSettingsTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGraphicsAutoVramBoundaryTest,
	"TunaSweeper.Graphics.AutoVramBoundary",
	TunaSweeperGraphicsSettingsTests::TestFlags)

bool FTunaSweeperGraphicsAutoVramBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	constexpr int64 FourGiB = 4ll * 1024ll * 1024ll * 1024ll;
	TestEqual(TEXT("Unknown VRAM uses Low"), UTunaSweeperGameUserSettings::ResolveAutoPresetForDedicatedVideoMemory(-1), ETunaSweeperGraphicsPreset::Low);
	TestEqual(TEXT("Zero VRAM uses Low"), UTunaSweeperGameUserSettings::ResolveAutoPresetForDedicatedVideoMemory(0), ETunaSweeperGraphicsPreset::Low);
	TestEqual(TEXT("Just below 4 GiB uses Low"), UTunaSweeperGameUserSettings::ResolveAutoPresetForDedicatedVideoMemory(FourGiB - 1), ETunaSweeperGraphicsPreset::Low);
	TestEqual(TEXT("Exactly 4 GiB uses Epic"), UTunaSweeperGameUserSettings::ResolveAutoPresetForDedicatedVideoMemory(FourGiB), ETunaSweeperGraphicsPreset::Epic);
	TestEqual(TEXT("Above 4 GiB uses Epic"), UTunaSweeperGameUserSettings::ResolveAutoPresetForDedicatedVideoMemory(FourGiB + 1), ETunaSweeperGraphicsPreset::Epic);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGraphicsPresetMappingTest,
	"TunaSweeper.Graphics.PresetMapping",
	TunaSweeperGraphicsSettingsTests::TestFlags)

bool FTunaSweeperGraphicsPresetMappingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const Scalability::FQualityLevels Low = UTunaSweeperGameUserSettings::BuildQualityLevelsForPreset(ETunaSweeperGraphicsPreset::Low, 73.0f);
	TestEqual(TEXT("Low texture quality saves VRAM"), Low.TextureQuality, 0);
	TestEqual(TEXT("Low reflection quality"), Low.ReflectionQuality, 0);
	TestEqual(TEXT("Low post process quality"), Low.PostProcessQuality, 0);
	TestEqual(TEXT("Low foliage quality"), Low.FoliageQuality, 0);
	TestEqual(TEXT("Low shadow quality"), Low.ShadowQuality, 1);
	TestEqual(TEXT("Resolution quality is preserved"), Low.ResolutionQuality, 73.0f);
	TestEqual(TEXT("Low profile is recognized"), UTunaSweeperGameUserSettings::MatchNamedPreset(Low), ETunaSweeperGraphicsPreset::Low);

	const Scalability::FQualityLevels Medium = UTunaSweeperGameUserSettings::BuildQualityLevelsForPreset(ETunaSweeperGraphicsPreset::Medium, 100.0f);
	TestEqual(TEXT("Medium anti-aliasing quality"), Medium.AntiAliasingQuality, 2);
	TestEqual(TEXT("Medium texture quality"), Medium.TextureQuality, 1);
	TestEqual(TEXT("Medium profile is recognized"), UTunaSweeperGameUserSettings::MatchNamedPreset(Medium), ETunaSweeperGraphicsPreset::Medium);

	Scalability::FQualityLevels Custom = UTunaSweeperGameUserSettings::BuildQualityLevelsForPreset(ETunaSweeperGraphicsPreset::High, 100.0f);
	Custom.TextureQuality = 1;
	TestEqual(TEXT("Mixed profile is custom"), UTunaSweeperGameUserSettings::MatchNamedPreset(Custom), ETunaSweeperGraphicsPreset::Custom);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperGraphicsCustomSelectorRoundTripTest,
	"TunaSweeper.Graphics.CustomSelectorRoundTrip",
	TunaSweeperGraphicsSettingsTests::TestFlags)

bool FTunaSweeperGraphicsCustomSelectorRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TArray<float> InitialFrameRates = TunaSweeperGraphicsSettingsOptions::BuildFrameRateCandidates(75.0f, 75.0f);
	const int32 InitialFrameRateIndex = InitialFrameRates.IndexOfByKey(75.0f);
	TestTrue(TEXT("Custom frame rate is a selectable candidate"), InitialFrameRateIndex != INDEX_NONE);
	const bool bHasNextFrameRate = InitialFrameRateIndex != INDEX_NONE && InitialFrameRateIndex + 1 < InitialFrameRates.Num();
	TestTrue(TEXT("Custom frame rate has a next candidate"), bHasNextFrameRate);
	if (bHasNextFrameRate)
	{
		const float NextFrameRate = InitialFrameRates[InitialFrameRateIndex + 1];
		TestEqual(TEXT("Next candidate after custom 75 FPS is 120 FPS"), NextFrameRate, 120.0f);
		const TArray<float> SteppedFrameRates = TunaSweeperGraphicsSettingsOptions::BuildFrameRateCandidates(NextFrameRate, 75.0f);
		const int32 SteppedIndex = SteppedFrameRates.IndexOfByKey(NextFrameRate);
		TestTrue(TEXT("Previous step returns to applied custom frame rate"), SteppedIndex > 0 && FMath::IsNearlyEqual(SteppedFrameRates[SteppedIndex - 1], 75.0f));
	}

	const FIntPoint CustomResolution(1920, 1200);
	const TArray<FIntPoint> InitialResolutions = TunaSweeperGraphicsSettingsOptions::BuildResolutionCandidates(CustomResolution, CustomResolution);
	const int32 InitialResolutionIndex = InitialResolutions.IndexOfByKey(CustomResolution);
	TestTrue(TEXT("Custom resolution is a selectable candidate"), InitialResolutionIndex != INDEX_NONE);
	const bool bHasNextResolution = InitialResolutionIndex != INDEX_NONE && InitialResolutionIndex + 1 < InitialResolutions.Num();
	TestTrue(TEXT("Custom resolution has a next candidate"), bHasNextResolution);
	if (bHasNextResolution)
	{
		const FIntPoint NextResolution = InitialResolutions[InitialResolutionIndex + 1];
		TestEqual(TEXT("Next resolution width after 1920x1200"), NextResolution.X, 2560);
		TestEqual(TEXT("Next resolution height after 1920x1200"), NextResolution.Y, 1440);
		const TArray<FIntPoint> SteppedResolutions = TunaSweeperGraphicsSettingsOptions::BuildResolutionCandidates(NextResolution, CustomResolution);
		const int32 SteppedIndex = SteppedResolutions.IndexOfByKey(NextResolution);
		TestTrue(TEXT("Previous step returns to applied custom resolution"), SteppedIndex > 0 && SteppedResolutions[SteppedIndex - 1] == CustomResolution);
	}
	return true;
}

#endif
