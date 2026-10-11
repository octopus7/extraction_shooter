#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Package.h"

namespace TunaSweeperQuestData
{
	bool ReadObjectiveType(const FString& RawType, ETunaSweeperObjectiveType& OutType);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperExtractionReturnTypeTest,
	"TunaSweeper.Quest.ExtractionReturn.Type",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperExtractionReturnTypeTest::RunTest(const FString& Parameters)
{
	ETunaSweeperObjectiveType Type = ETunaSweeperObjectiveType::LevelTravel;
	TestTrue(TEXT("JSON supports extraction-specific return objective"),
		TunaSweeperQuestData::ReadObjectiveType(TEXT("extraction_return"), Type));
	TestTrue(TEXT("Extraction return is distinct from ordinary travel"), Type != ETunaSweeperObjectiveType::LevelTravel);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperExtractionReturnTest,
	"TunaSweeper.Quest.ExtractionReturn.ArrivalAndCancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperExtractionReturnTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
	TStrongObjectPtr<UTunaSweeperQuestSubsystem> Quests(NewObject<UTunaSweeperQuestSubsystem>(Game.Get()));
	Quests->bQuestDataLoaded = true;
	FTunaSweeperQuestDefinition Definition;
	Definition.QuestId = 901;
	FTunaSweeperObjectiveDefinition Objective;
	Objective.ObjectiveId = TEXT("extract");
	Objective.Type = ETunaSweeperObjectiveType::ExtractionReturn;
	Objective.SourceLevelName = TEXT("RaidMap");
	Objective.TargetLevelName = TEXT("BunkerMap");
	Objective.RequiredCount = 2;
	Definition.Objectives.Add(Objective);
	Quests->QuestDefinitions.Add(901, Definition);
	Quests->SetQuestState(901, ETunaSweeperQuestState::Accepted);
	auto Count = [&] { return Quests->GetObjectiveProgressCount(901, TEXT("extract")); };
	auto Begin = [&] { Quests->BeginExtractionReturn(TEXT("UEDPIE_0_RaidMap"), TEXT("BunkerMap")); };
	auto Arrive = [&] { Quests->CompleteExtractionReturn(TEXT("UEDPIE_0_BunkerMap")); };
	Quests->NotifyLevelTravelRequested(TEXT("RaidMap"), TEXT("BunkerMap")); Arrive();
	TestEqual(TEXT("Ordinary travel never satisfies extraction"), Count(), 0);
	Begin(); TestEqual(TEXT("Extraction hold alone does not complete"), Count(), 0);
	Quests->CompleteExtractionReturn(TEXT("OtherMap")); Arrive();
	TestEqual(TEXT("Wrong destination consumes pending record without credit"), Count(), 0);
	Begin(); Quests->CancelExtractionReturn(); Arrive();
	TestEqual(TEXT("Failed travel leaves objective incomplete"), Count(), 0);
	Begin(); Quests->NotifyBunkerRescueReturn(TEXT("RaidMap"), TEXT("BunkerMap")); Arrive();
	TestEqual(TEXT("Death rescue cancels extraction credit"), Count(), 0);
	Begin(); Quests->NotifyLevelTravelRequested(TEXT("RaidMap"), TEXT("BunkerMap")); Arrive();
	TestEqual(TEXT("Alternate travel overrides pending extraction"), Count(), 0);
	Quests->BeginExtractionReturn(TEXT("BunkerMap"), TEXT("BunkerMap")); Arrive();
	TestEqual(TEXT("Non-raid source is rejected"), Count(), 0);
	Quests->BeginExtractionReturn(TEXT("RaidMap"), TEXT("OtherMap")); Arrive();
	TestEqual(TEXT("Non-base destination is rejected"), Count(), 0);
	Quests->BeginExtractionReturn(TEXT("RaidMap"), TEXT("Other_BunkerMap")); Arrive();
	TestEqual(TEXT("Similarly named destination is not the base"), Count(), 0);
	Begin(); Quests->CompleteExtractionReturn(TEXT("Other_BunkerMap")); Arrive();
	TestEqual(TEXT("Similarly named arrival cannot complete extraction"), Count(), 0);
	Quests->SetQuestState(901, ETunaSweeperQuestState::Available); Begin();
	Quests->SetQuestState(901, ETunaSweeperQuestState::Accepted); Arrive();
	TestEqual(TEXT("Quest accepted after extraction gets no retroactive credit"), Count(), 0);
	Begin(); Arrive(); Arrive();
	TestEqual(TEXT("Arrival counts once despite duplicate notification"), Count(), 1);
	TestEqual(TEXT("Multiple-return objective remains accepted"), Quests->GetQuestState(901), ETunaSweeperQuestState::Accepted);
	Begin();
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("BunkerMap"), CreatePackage(TEXT("/Temp/ExtractionReturn/BunkerMap")));
	Quests->HandleExtractionArrival(World);
	TestEqual(TEXT("Foreign world cannot grant credit"), Count(), 1);
	World->SetGameInstance(Game.Get());
	Quests->HandleExtractionArrival(World);
	TestEqual(TEXT("Own base world arrival completes goal"), Quests->GetQuestState(901), ETunaSweeperQuestState::RewardAvailable);
	World->SetGameInstance(nullptr); World->DestroyWorld(false); World->RemoveFromRoot();
	TArray<FTunaSweeperNumericQuestProgressSaveData> Saved;
	int32 Tracked = 0, Coins = 0;
	Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
	Quests->LoadQuestProgressFromSave(Saved, Tracked, Coins);
	TestEqual(TEXT("Completed goal survives save restore"), Quests->GetQuestState(901), ETunaSweeperQuestState::RewardAvailable);
	Quests->SetQuestState(901, ETunaSweeperQuestState::Accepted); Begin();
	Quests->LoadQuestProgressFromSave({}, 0, 0);
	Quests->SetQuestState(901, ETunaSweeperQuestState::Accepted); Arrive();
	TestEqual(TEXT("Loading a save discards pending extraction"), Count(), 0);
	Begin(); Quests->ResetQuestProgressForNewGame();
	Quests->SetQuestState(901, ETunaSweeperQuestState::Accepted); Arrive();
	TestEqual(TEXT("New game discards pending extraction"), Count(), 0);
	return true;
}
#endif
