#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystem/TunaSweeperRaidPlacementSubsystem.h"
#include "Raid/TunaSweeperLootAnchorPreviewDataAsset.h"

#include "Misc/AutomationTest.h"

namespace TunaSweeperRaidPlacementTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperRaidPlacementDeterministicRollTest,
	"TunaSweeper.RaidPlacement.DeterministicRoll",
	TunaSweeperRaidPlacementTests::TestFlags)

bool FTunaSweeperRaidPlacementDeterministicRollTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const float First = UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48271, 101);
	const float Interleaved = UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48271, 202);
	const float Second = UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48271, 101);

	TestTrue(TEXT("Roll is in [0, 1)"), First >= 0.0f && First < 1.0f);
	TestEqual(TEXT("Same RaidSeed and PlacementId reproduce exactly"), First, Second);
	TestNotEqual(TEXT("PlacementId contributes to the roll"), First, Interleaved);
	TestNotEqual(TEXT("RaidSeed contributes to the roll"), First, UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48272, 101));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperLootAnchorPreviewCatalogTest,
	"TunaSweeper.RaidPlacement.LootPreviewCatalog",
	TunaSweeperRaidPlacementTests::TestFlags)

bool FTunaSweeperLootAnchorPreviewCatalogTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UTunaSweeperLootAnchorPreviewDataAsset* Catalog = NewObject<UTunaSweeperLootAnchorPreviewDataAsset>();
	Catalog->PreviewDefinitions.Add({TEXT("Small")});
	Catalog->PreviewDefinitions.Add({TEXT("Medium")});
	Catalog->PreviewDefinitions.Add({TEXT("Large")});

	const FTunaSweeperLootAnchorPreviewDefinition* DefaultPreview = Catalog->FindPreview(NAME_None);
	const FTunaSweeperLootAnchorPreviewDefinition* MediumPreview = Catalog->FindPreview(TEXT("Medium"));
	TestNotNull(TEXT("An empty selection resolves to the first catalog entry"), DefaultPreview);
	TestEqual(TEXT("The first catalog entry is the neutral default"), DefaultPreview ? DefaultPreview->PreviewId : NAME_None, FName(TEXT("Small")));
	TestNotNull(TEXT("A combo option resolves by its catalog ID"), MediumPreview);
	TestEqual(TEXT("The selected catalog entry is returned"), MediumPreview ? MediumPreview->PreviewId : NAME_None, FName(TEXT("Medium")));
	TestNull(TEXT("Unknown combo option is rejected"), Catalog->FindPreview(TEXT("Unknown")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
