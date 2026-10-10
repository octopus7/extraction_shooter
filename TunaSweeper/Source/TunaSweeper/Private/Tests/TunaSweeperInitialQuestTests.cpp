#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRetiredDemoQuestTest,
	"TunaSweeper.Quest.RetiredDemoCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRetiredDemoQuestTest::RunTest(const FString& Parameters)
{
	auto* Settings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	const ETunaSweeperBuildTarget PreviousTarget = Settings->BuildTarget;
	ON_SCOPE_EXIT { Settings->BuildTarget = PreviousTarget; };
	Settings->BuildTarget = ETunaSweeperBuildTarget::NoStoreDemo;
	// A base game instance keeps the fixture independent of user save slots.
	auto* Instance = NewObject<UGameInstance>();
	auto* Quests = NewObject<UTunaSweeperQuestSubsystem>(Instance);
	TestTrue(TEXT("An intentionally empty catalog loads without fallback quests"), Quests->LoadQuestData(true));
	TArray<FTunaSweeperQuestDefinition> Definitions;
	Quests->GetAllQuestDefinitions(Definitions);
	TestEqual(TEXT("No retired demo quests remain"), Definitions.Num(), 0);

	FTunaSweeperNumericQuestProgressSaveData OldProgress;
	OldProgress.QuestId = 999999;
	OldProgress.State = ETunaSweeperQuestState::RewardCompleted;
	Quests->LoadQuestProgressFromSave({OldProgress}, OldProgress.QuestId, 125);
	TestFalse(TEXT("Removed quests cannot be accepted"), Quests->AcceptQuest(OldProgress.QuestId));
	TestFalse(TEXT("Removed quests cannot grant rewards"), Quests->ClaimQuestReward(OldProgress.QuestId));
	TestTrue(TEXT("Removed tracked quest is cleared"), Quests->GetTrackedQuestId() == 0);
	int32 ResolvedQuest = 0;
	TestFalse(TEXT("Provider cannot resolve a retired quest"),
		Quests->TryResolveQuestForProvider(TEXT("provider.mole"), OldProgress.QuestId, ResolvedQuest));

	TArray<FTunaSweeperNumericQuestProgressSaveData> Saved;
	int32 Tracked = 0;
	int32 Coins = 0;
	Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
	TestEqual(TEXT("Removed progress is not written back"), Saved.Num(), 0);
	TestTrue(TEXT("No retired tracking is written back"), Tracked == 0);
	TestEqual(TEXT("Existing currency remains intact"), Coins, 125);
	return true;
}
#endif
