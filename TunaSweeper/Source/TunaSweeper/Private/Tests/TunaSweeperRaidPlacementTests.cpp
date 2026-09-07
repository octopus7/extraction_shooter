#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystem/TunaSweeperRaidPlacementSubsystem.h"
#include "Raid/TunaSweeperLootAnchorPreviewDataAsset.h"
#include "Game/TunaSweeperDataValueTypes.h"

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace TunaSweeperRaidPlacementTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperAnchorPlacementSchemaTest,
	"TunaSweeper.RaidPlacement.AnchorPlacementSchema",
	TunaSweeperRaidPlacementTests::TestFlags)

bool FTunaSweeperAnchorPlacementSchemaTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const auto ValidateAnchorRows = [this](const TCHAR* FileName, bool bRequireMemoId)
	{
		const FString JsonPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), FileName);
		FString JsonContent;
		TArray<TSharedPtr<FJsonValue>> Rows;
		if (!TestTrue(FString::Printf(TEXT("%s can be read"), FileName), FFileHelper::LoadFileToString(JsonContent, *JsonPath)) ||
			!TestTrue(FString::Printf(TEXT("%s parses as an array"), FileName), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), Rows)))
		{
			return;
		}

		TSet<FString> SeenPlacementKeys;
		for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
		{
			const TSharedPtr<FJsonObject>* ObjectPtr = nullptr;
			const bool bIsObject = Rows[RowIndex].IsValid() && Rows[RowIndex]->TryGetObject(ObjectPtr) && ObjectPtr && ObjectPtr->IsValid();
			if (!TestTrue(FString::Printf(TEXT("%s row %d is an object"), FileName, RowIndex), bIsObject))
			{
				continue;
			}

			FString LevelName;
			double PlacementId = INDEX_NONE;
			const TSharedPtr<FJsonObject>& Object = *ObjectPtr;
			TestTrue(FString::Printf(TEXT("%s row %d has level_name"), FileName, RowIndex), Object->TryGetStringField(TEXT("level_name"), LevelName) && !LevelName.TrimStartAndEnd().IsEmpty());
			TestTrue(FString::Printf(TEXT("%s row %d has positive placement_id"), FileName, RowIndex), Object->TryGetNumberField(TEXT("placement_id"), PlacementId) && PlacementId > 0.0);
			TestFalse(FString::Printf(TEXT("%s row %d has no location"), FileName, RowIndex), Object->HasField(TEXT("location")));
			TestFalse(FString::Printf(TEXT("%s row %d has no rotation"), FileName, RowIndex), Object->HasField(TEXT("rotation")));
			TestFalse(FString::Printf(TEXT("%s row %d has no scale"), FileName, RowIndex), Object->HasField(TEXT("scale")));
			if (bRequireMemoId)
			{
				double MemoId = INDEX_NONE;
				TestTrue(FString::Printf(TEXT("%s row %d has positive memo_id"), FileName, RowIndex), Object->TryGetNumberField(TEXT("memo_id"), MemoId) && MemoId > 0.0);
			}

			const FString PlacementKey = FString::Printf(TEXT("%s:%d"), *LevelName, FMath::RoundToInt(PlacementId));
			TestFalse(FString::Printf(TEXT("%s row %d has a unique level/placement key"), FileName, RowIndex), SeenPlacementKeys.Contains(PlacementKey));
			SeenPlacementKeys.Add(PlacementKey);
		}
	};

	ValidateAnchorRows(TEXT("EnemySpawns.json"), false);
	ValidateAnchorRows(TEXT("MemoSpawns.json"), true);
	TestTrue(TEXT("The shared placement-anchor Blueprint exists"), FPackageName::DoesPackageExist(TEXT("/Game/Raid/Placement/BP_RaidPlacementAnchor")));
	TestTrue(TEXT("The directly placeable extraction Blueprint exists"), FPackageName::DoesPackageExist(TEXT("/Game/Interaction/BP_ExtractionPoint")));
	return true;
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
	FTunaSweeperProbabilityBasisTest,
	"TunaSweeper.RaidPlacement.IntegerProbabilityBasis",
	TunaSweeperRaidPlacementTests::TestFlags)

bool FTunaSweeperProbabilityBasisTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Raw 0 is 0 percent"), TunaSweeperDataValues::NormalizeProbabilityValue(0), 0.0f);
	TestEqual(TEXT("Raw 1 is 0.01 percent"), TunaSweeperDataValues::NormalizeProbabilityValue(1), 0.0001f);
	TestEqual(TEXT("Raw 10000 is 100 percent"), TunaSweeperDataValues::NormalizeProbabilityValue(10000), 1.0f);
	TestEqual(TEXT("Probabilities clamp above 100 percent"), TunaSweeperDataValues::NormalizeProbabilityValue(20000), 1.0f);
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
