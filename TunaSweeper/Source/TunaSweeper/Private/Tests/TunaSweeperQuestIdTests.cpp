#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Quest/TunaSweeperQuestId.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestIdBoundaryTest,
	"TunaSweeper.Quest.Identity.BoundariesAndDisplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestIdBoundaryTest::RunTest(const FString& Parameters)
{
	const auto ReadJson = [](const FString& Literal)
	{
		TSharedPtr<FJsonObject> Object;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(TEXT("{\"id\":") + Literal + TEXT("}")), Object);
		return TunaSweeperQuestId::Read(Object, TEXT("id"));
	};
	TestEqual(TEXT("Smallest positive numeric ID"), ReadJson(TEXT("1")), 1);
	TestEqual(TEXT("Largest int32 numeric ID"), ReadJson(TEXT("2147483647")), MAX_int32);
	for (const FString& Invalid : { TEXT("0"), TEXT("-1"), TEXT("3.5"), TEXT("2147483648"),
		TEXT("1e100"), TEXT("null"), TEXT("true"), TEXT("[]"), TEXT("{}"), TEXT("\"quest.old\""),
		TEXT("\"+3\""), TEXT("\" 3\""), TEXT("\"3.0\""), TEXT("\"2147483648\"") })
	{
		TestEqual(FString::Printf(TEXT("Invalid identity %s is unset"), *Invalid), ReadJson(Invalid), 0);
	}
	TestEqual(TEXT("Legacy numeric JSON strings retain their integer identity"), ReadJson(TEXT("\"003\"")), 3);
	TestEqual(TEXT("Missing field is unset"), TunaSweeperQuestId::Read(MakeShared<FJsonObject>(), TEXT("id")), 0);
	TestEqual(TEXT("Null object is unset"), TunaSweeperQuestId::Read(nullptr, TEXT("id")), 0);
	TestEqual(TEXT("Small ID has minimum three display digits"), TunaSweeperQuestId::Format(3), FString(TEXT("003")));
	TestEqual(TEXT("Four digit ID is not truncated"), TunaSweeperQuestId::Format(1000), FString(TEXT("1000")));
	TestEqual(TEXT("Unset ID has no display number"), TunaSweeperQuestId::Format(0), FString());
	TestEqual(TEXT("Invalid negative ID has no display number"), TunaSweeperQuestId::Format(-1), FString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestIdSaveMigrationTest,
	"TunaSweeper.Quest.Identity.SaveMigrationRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestIdSaveMigrationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UTunaSweeperSaveGame> Save(NewObject<UTunaSweeperSaveGame>());
	Save->SaveVersion = 21;
	Save->QuestCoinBalance = 125;
	Save->TotalExperiencePoints = 987654;
	Save->SelectedOutfitId = TEXT("Maid");
	Save->AcquiredMemoIds = { 4, 8 };
	Save->TrackedQuestId = TEXT("003");
	auto& Legacy = Save->QuestProgressStates.AddDefaulted_GetRef();
	Legacy.QuestId = TEXT("003");
	Legacy.State = ETunaSweeperQuestState::Accepted;
	auto& Objective = Legacy.ObjectiveProgress.AddDefaulted_GetRef();
	Objective.ObjectiveId = TEXT("collect");
	Objective.CurrentCount = 2;
	auto& Retired = Save->QuestProgressStates.AddDefaulted_GetRef();
	Retired.QuestId = TEXT("demo_q4_todays_reward");
	Retired.State = ETunaSweeperQuestState::RewardCompleted;
	auto& Conflict = Save->QuestProgressStates.AddDefaulted_GetRef();
	Conflict.QuestId = TEXT("1000");
	Conflict.State = ETunaSweeperQuestState::Accepted;
	auto& Current = Save->NumericQuestProgressStates.AddDefaulted_GetRef();
	Current.QuestId = 1000;
	Current.State = ETunaSweeperQuestState::RewardCompleted;

	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Legacy FName schema serializes"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bytes))) return false;
	TStrongObjectPtr<UTunaSweeperSaveGame> Loaded(Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
	if (!TestNotNull(TEXT("Legacy save deserializes"), Loaded.Get())) return false;
	if (!TestEqual(TEXT("Legacy progress array deserializes"), Loaded->QuestProgressStates.Num(), 3)) return false;
	TestEqual(TEXT("Legacy identity retains its serialized FName"), Loaded->QuestProgressStates[0].QuestId, FName(TEXT("003")));
	Loaded->MigrateLegacyQuestIds();
	if (!TestEqual(TEXT("Only numeric identities survive migration"), Loaded->NumericQuestProgressStates.Num(), 2)) return false;
	TestEqual(TEXT("Numeric tracking migrates"), Loaded->NumericTrackedQuestId, 3);
	const auto* Migrated = Loaded->NumericQuestProgressStates.FindByPredicate([](const auto& Entry) { return Entry.QuestId == 3; });
	if (!TestNotNull(TEXT("Numeric legacy quest is retained"), Migrated)) return false;
	TestEqual(TEXT("Legacy state survives"), Migrated->State, ETunaSweeperQuestState::Accepted);
	if (!TestEqual(TEXT("Legacy objective array survives"), Migrated->ObjectiveProgress.Num(), 1)) return false;
	TestEqual(TEXT("Legacy objective identity survives"), Migrated->ObjectiveProgress[0].ObjectiveId, FName(TEXT("collect")));
	TestEqual(TEXT("Legacy objective count survives"), Migrated->ObjectiveProgress[0].CurrentCount, 2);
	TestEqual(TEXT("Existing numeric progress wins duplicate identity"), Loaded->NumericQuestProgressStates[0].State, ETunaSweeperQuestState::RewardCompleted);
	TestTrue(TEXT("Legacy fields are cleared"), Loaded->QuestProgressStates.IsEmpty() && Loaded->TrackedQuestId.IsNone());
	Loaded->MigrateLegacyQuestIds();
	TestEqual(TEXT("Migration is idempotent"), Loaded->NumericQuestProgressStates.Num(), 2);
	if (!TestTrue(TEXT("Migrated save serializes"), UGameplayStatics::SaveGameToMemory(Loaded.Get(), Bytes))) return false;
	TStrongObjectPtr<UTunaSweeperSaveGame> Reloaded(Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
	if (!TestNotNull(TEXT("Migrated save reloads"), Reloaded.Get())) return false;
	TestEqual(TEXT("Numeric progress survives a second serialization"), Reloaded->NumericQuestProgressStates.Num(), 2);
	TestEqual(TEXT("Tracked integer survives a second serialization"), Reloaded->NumericTrackedQuestId, 3);
	TestEqual(TEXT("Currency preserved"), Reloaded->QuestCoinBalance, 125);
	TestEqual(TEXT("Unrelated experience preserved"), Reloaded->TotalExperiencePoints, static_cast<int64>(987654));
	TestEqual(TEXT("Unrelated outfit preserved"), Reloaded->SelectedOutfitId, FName(TEXT("Maid")));
	TestTrue(TEXT("Unrelated memo progress preserved"), Reloaded->AcquiredMemoIds == TArray<int32>({ 4, 8 }));
	Reloaded->NumericTrackedQuestId = 0;
	Reloaded->TrackedQuestId = TEXT("demo_q4_todays_reward");
	Reloaded->MigrateLegacyQuestIds();
	TestEqual(TEXT("Retired text tracking is discarded"), Reloaded->NumericTrackedQuestId, 0);
	return true;
}

#endif
