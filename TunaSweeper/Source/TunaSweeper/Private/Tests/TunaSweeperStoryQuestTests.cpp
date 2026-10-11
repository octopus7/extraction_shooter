#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperStoryQuestTest,
    "TunaSweeper.Quest.StoryAcceptanceRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperStoryQuestTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
    TStrongObjectPtr<UTunaSweeperQuestSubsystem> Quests(NewObject<UTunaSweeperQuestSubsystem>(Game.Get()));
    Quests->bQuestDataLoaded = true;
    FTunaSweeperQuestDefinition Definition;
    Definition.QuestId = 1;
    Definition.AutoAcceptAfterScenarioFlag = TEXT("test.intro.complete");
    FTunaSweeperObjectiveDefinition Objective;
    Objective.ObjectiveId = TEXT("leave_bunker");
    Objective.Type = ETunaSweeperObjectiveType::LevelTravel;
    Objective.SourceLevelName = TEXT("BunkerMap");
    Objective.TargetLevelName = TEXT("RaidForest");
    Definition.Objectives.Add(Objective);
    Quests->QuestDefinitions.Add(1, Definition);
    const TSet<FName> CompletedFlags { FName(TEXT("test.intro.complete")) };
    Quests->ReconcileStoryQuestAcceptance({});
    TestEqual(TEXT("No acceptance before dialogue"), Quests->GetQuestState(1), ETunaSweeperQuestState::Available);
    Quests->ReconcileStoryQuestAcceptance(CompletedFlags);
    TestEqual(TEXT("Dialogue completion accepts quest"), Quests->GetQuestState(1), ETunaSweeperQuestState::Accepted);
    TestEqual(TEXT("Accepted quest is tracked"), Quests->GetTrackedQuestId(), 1);
    Quests->NotifyLevelTravelRequested(TEXT("RaidForest"), TEXT("BunkerMap"));
    TestEqual(TEXT("Return does not satisfy outward travel"), Quests->GetQuestState(1), ETunaSweeperQuestState::Accepted);
    Quests->NotifyLevelTravelRequested(TEXT("UEDPIE_0_BunkerMap"), TEXT("RaidForest"));
    TestEqual(TEXT("Outward travel satisfies objective"), Quests->GetQuestState(1), ETunaSweeperQuestState::RewardAvailable);
    Quests->ReconcileStoryQuestAcceptance(CompletedFlags);
    TestEqual(TEXT("Recovery preserves achieved state"), Quests->GetQuestState(1), ETunaSweeperQuestState::RewardAvailable);
    Quests->LoadQuestProgressFromSave({}, 0, 0);
    Quests->ReconcileStoryQuestAcceptance(CompletedFlags);
    TestEqual(TEXT("Loaded story flag repairs absent quest progress"), Quests->GetQuestState(1), ETunaSweeperQuestState::Accepted);
    Quests->ReconcileStoryQuestAcceptance(CompletedFlags);
    TestEqual(TEXT("Repeated reconciliation is idempotent"), Quests->QuestProgressById.Num(), 1);
    Quests->SetQuestState(1, ETunaSweeperQuestState::RewardCompleted);
    Quests->ReconcileStoryQuestAcceptance(CompletedFlags);
    TestEqual(TEXT("Recovery preserves completed state"), Quests->GetQuestState(1), ETunaSweeperQuestState::RewardCompleted);
    FTunaSweeperQuestDefinition Immediate;
    Immediate.QuestId = 902;
    Immediate.ProviderId = TEXT("provider.test_source");
    Immediate.CompletionProviderId = TEXT("provider.test_reward");
    const FBoolProperty* CompleteOnAccept = FindFProperty<FBoolProperty>(
        FTunaSweeperQuestDefinition::StaticStruct(), TEXT("bCompleteOnAccept"));
    if (!TestNotNull(TEXT("Explicit completion-on-accept option exists"), CompleteOnAccept)) return false;
    CompleteOnAccept->SetPropertyValue_InContainer(&Immediate, true);
    Quests->QuestDefinitions.Add(902, Immediate);
    FTunaSweeperQuestDefinition Followup;
    Followup.QuestId = 903;
    Followup.RequiredCompletedQuestIds.Add(902);
    Followup.Objectives.Add(Objective);
    Quests->QuestDefinitions.Add(903, Followup);
    TestTrue(TEXT("Immediate quest accepts"), Quests->AcceptQuest(902));
    TestEqual(TEXT("Acceptance immediately enables reward"), Quests->GetQuestState(902), ETunaSweeperQuestState::RewardAvailable);
    TestFalse(TEXT("Acceptance does not unlock reward-dependent followup"), Quests->CanAcceptQuest(903));
    TestFalse(TEXT("Cannot accept again"), Quests->AcceptQuest(902));
    int32 Resolved = 0;
    TestFalse(TEXT("Source NPC cannot offer completed quest reward"), Quests->TryResolveQuestForProvider(Immediate.ProviderId, 0, Resolved));
    TestTrue(TEXT("Reward NPC offers reward"), Quests->TryResolveQuestForProvider(Immediate.CompletionProviderId, 0, Resolved));
    TestEqual(TEXT("Reward NPC resolves immediate quest"), Resolved, 902);
    TArray<FTunaSweeperNumericQuestProgressSaveData> Saved;
    int32 Tracked = 0, Coins = 0;
    Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
    Quests->LoadQuestProgressFromSave(Saved, Tracked, Coins);
    TestEqual(TEXT("Immediate reward state survives reload"), Quests->GetQuestState(902), ETunaSweeperQuestState::RewardAvailable);
    TestTrue(TEXT("Explicit reward claim succeeds"), Quests->ClaimQuestReward(902));
    TestTrue(TEXT("Reward claim unlocks followup"), Quests->CanAcceptQuest(903));
    TestFalse(TEXT("Reward cannot be claimed twice"), Quests->ClaimQuestReward(902));
    return true;
}
#endif
