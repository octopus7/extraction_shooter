#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperInitialQuestAutoAcceptTest,
	"TunaSweeper.Quest.InitialScenarioAutoAccept",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperInitialQuestAutoAcceptTest::RunTest(const FString& Parameters)
{
	// A base game instance bypasses gameplay save I/O in this isolated fixture.
	auto* Instance = NewObject<UGameInstance>();
	auto* Quests = NewObject<UTunaSweeperQuestSubsystem>(Instance);
	Quests->LoadQuestProgressFromSave({}, NAME_None, 0);
	const FName FirstQuest(TEXT("demo_q1_water_intake_check"));
	const FName IntroFlag(TEXT("dialogue.demo.toilet_intro"));
	int32 Changes = 0;
	Quests->OnQuestProgressChanged.AddLambda([&Changes]() { ++Changes; });
	TestFalse(TEXT("Other dialogue does not accept a quest"), Quests->AutoAcceptQuestForScenario(TEXT("dialogue.other")));
	TestFalse(TEXT("Quest presentations without a flag do not accept a quest"), Quests->AutoAcceptQuestForScenario(NAME_None));
	TestTrue(TEXT("Finishing the intro accepts the first quest"), Quests->AutoAcceptQuestForScenario(IntroFlag));
	TestEqual(TEXT("First quest is accepted"), Quests->GetQuestState(FirstQuest), ETunaSweeperQuestState::Accepted);
	TestFalse(TEXT("Repeated completion cannot accept or notify again"), Quests->AutoAcceptQuestForScenario(IntroFlag));
	TestEqual(TEXT("Exactly one progress notification"), Changes, 1);

	TArray<FTunaSweeperQuestProgressSaveData> Saved;
	FName Tracked;
	int32 Coins = 0;
	Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
	TestEqual(TEXT("Auto-accepted quest is tracked"), Tracked, FirstQuest);
	auto* Restored = NewObject<UTunaSweeperQuestSubsystem>(Instance);
	Restored->LoadQuestProgressFromSave(Saved, Tracked, Coins);
	TestEqual(TEXT("Accepted state survives save data round-trip"), Restored->GetQuestState(FirstQuest), ETunaSweeperQuestState::Accepted);
	TestFalse(TEXT("Loaded acceptance does not notify again"), Restored->AutoAcceptQuestForScenario(IntroFlag));

	for (auto& Progress : Saved) if (Progress.QuestId == FirstQuest) Progress.State = ETunaSweeperQuestState::RewardCompleted;
	Restored->LoadQuestProgressFromSave(Saved, NAME_None, Coins);
	TestFalse(TEXT("Completed first quest is not accepted again"), Restored->AutoAcceptQuestForScenario(IntroFlag));
	TestEqual(TEXT("Replayed intro does not accept the next quest"), Restored->GetQuestState(TEXT("demo_q2_clear_water_screen")), ETunaSweeperQuestState::Available);
	return true;
}
#endif
