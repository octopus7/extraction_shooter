#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Settings/TunaSweeperGameUserSettings.h"

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

#endif
