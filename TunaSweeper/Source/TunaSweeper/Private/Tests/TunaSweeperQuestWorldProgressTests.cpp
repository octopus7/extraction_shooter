#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperRegionLinkActor.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperQuestData
{
    bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

namespace QuestWorldProgressTest
{
    bool Parse(const FString& Fields, FTunaSweeperObjectiveDefinition& Out)
    {
        TSharedPtr<FJsonObject> Json;
        const FString Text = TEXT("{\"objective_id\":\"open\",\"type\":\"world_progress_completed\",") + Fields + TEXT("}");
        return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) &&
            TunaSweeperQuestData::ParseObjective(Json, Out);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestWorldProgressDataTest,
    "TunaSweeper.Quest.WorldProgress.DataValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestWorldProgressDataTest::RunTest(const FString& Parameters)
{
    FTunaSweeperObjectiveDefinition Objective;
    TestTrue(TEXT("Saved world completion is a supported objective"),
        QuestWorldProgressTest::Parse(TEXT("\"world_progress_id\":\"test.passage\""), Objective));
    TestEqual(TEXT("World completion requires one completed object"), Objective.RequiredCount, 1);
    TestEqual(TEXT("Exact persistent ID is retained"), Objective.WorldProgressId, FName(TEXT("test.passage")));
    TestEqual(TEXT("World completion is distinct from an interaction"), Objective.Type, ETunaSweeperObjectiveType::WorldProgressCompleted);
    TestTrue(TEXT("Explicit one-object count loads"),
        QuestWorldProgressTest::Parse(TEXT("\"world_progress_id\":\"test.passage\",\"required_count\":1"), Objective));
    for (const FString Id : {TEXT("\"\""), TEXT("\"   \""), TEXT("17"), TEXT("true"), TEXT("null")})
        TestFalse(TEXT("Missing or malformed world ID cannot match all objects"),
            QuestWorldProgressTest::Parse(TEXT("\"world_progress_id\":") + Id, Objective));
    TestFalse(TEXT("Missing ID is rejected"), QuestWorldProgressTest::Parse(TEXT("\"required_count\":1"), Objective));
    for (const FString Count : {TEXT("0"), TEXT("2"), TEXT("1.5"), TEXT("-1"), TEXT("\"1\""), TEXT("null"), TEXT("true")})
        TestFalse(TEXT("World completion rejects repeated or malformed counts"),
            QuestWorldProgressTest::Parse(TEXT("\"world_progress_id\":\"test.passage\",\"required_count\":") + Count, Objective));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestWorldProgressTest,
    "TunaSweeper.Quest.WorldProgress.PassageAcceptanceAndPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestWorldProgressTest::RunTest(const FString& Parameters)
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Never read/write player save slots.
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        FContextAccess::Attach(Game.Get(), nullptr); World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Real initialized quest subsystem"), Quests)) return false;
    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset(); Quests->bQuestDataLoaded = true;
    Quests->CoinBalance = 0; Game->WorldProgressStatesById.Reset(); Game->ResetPlayerSlotArrays();

    auto AddQuest = [&](int32 Id, FName ProgressId)
    {
        FTunaSweeperObjectiveDefinition Objective;
        Objective.ObjectiveId = TEXT("open"); Objective.Type = ETunaSweeperObjectiveType::WorldProgressCompleted;
        Objective.WorldProgressId = ProgressId;
        FTunaSweeperQuestDefinition Definition;
        Definition.QuestId = Id; Definition.Objectives = {Objective};
        Quests->QuestDefinitions.Add(Id, Definition);
    };
    constexpr int32 GateId = 900201, LiftId = 900202, MixedId = 900203, LateId = 900204, LockedId = 900205, PrerequisiteId = 900206;
    AddQuest(GateId, TEXT("test.passage.gate"));
    AddQuest(LiftId, TEXT("test.passage.lift"));
    AddQuest(LateId, TEXT("test.passage.gate"));
    AddQuest(LockedId, TEXT("test.passage.gate"));
    Quests->QuestDefinitions[GateId].Rewards.Coins = 5;
    Quests->QuestDefinitions[LockedId].RequiredCompletedQuestIds = {PrerequisiteId};
    FTunaSweeperQuestDefinition Prerequisite;
    Prerequisite.QuestId = PrerequisiteId; Prerequisite.bCompleteOnAccept = true;
    Quests->QuestDefinitions.Add(PrerequisiteId, Prerequisite);
    TestTrue(TEXT("Accept unopened gate objective"), Quests->AcceptQuest(GateId));
    TestTrue(TEXT("Accept broken lift objective"), Quests->AcceptQuest(LiftId));
    TArray<FTunaSweeperNumericQuestProgressSaveData> AcceptedSave;
    int32 Tracked = 0, Coins = 0;
    Quests->ExportQuestProgressForSave(AcceptedSave, Tracked, Coins);
    Game->UpdateWorldProgressState(TEXT("test.other"), NAME_None, ETunaSweeperWorldProgressState::Completed, 1, 1, false);
    Game->UpdateWorldProgressState(TEXT("test.passage.gate"), NAME_None, ETunaSweeperWorldProgressState::InProgress, 1, 1, false);
    Quests->NotifyInteractionCompleted(TEXT("test.passage.gate"), NAME_None);
    TestEqual(TEXT("Wrong object, incomplete state and generic interaction give no credit"), Quests->GetQuestState(GateId), ETunaSweeperQuestState::Accepted);

    Game->UpdateWorldProgressState(TEXT("test.passage.gate"), NAME_None, ETunaSweeperWorldProgressState::Completed, 1, 1, false);
    TestEqual(TEXT("World completion completes accepted gate objective"), Quests->GetQuestState(GateId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("World completion does not auto-accept another quest"), Quests->GetQuestState(LateId), ETunaSweeperQuestState::Available);
    TestEqual(TEXT("Completion does not grant reward"), Quests->GetCoinBalance(), 0);
    TestTrue(TEXT("Late acceptance recognizes already completed world object"), Quests->AcceptQuest(LateId));
    TestEqual(TEXT("Late quest is ready for reward"), Quests->GetQuestState(LateId), ETunaSweeperQuestState::RewardAvailable);
    TestFalse(TEXT("Completed world object cannot bypass quest prerequisites"), Quests->AcceptQuest(LockedId));
    Quests->AcceptQuest(PrerequisiteId);
    TestFalse(TEXT("Pending prerequisite reward still blocks acceptance"), Quests->AcceptQuest(LockedId));
    Quests->ClaimQuestReward(PrerequisiteId); Quests->AcceptQuest(LockedId);
    TestEqual(TEXT("After prerequisite claim, existing opening counts"), Quests->GetQuestState(LockedId), ETunaSweeperQuestState::RewardAvailable);
    TestTrue(TEXT("Reward can be claimed once"), Quests->ClaimQuestReward(GateId));
    TestFalse(TEXT("Reward cannot be duplicated"), Quests->ClaimQuestReward(GateId));
    Game->UpdateWorldProgressState(TEXT("test.passage.gate"), NAME_None, ETunaSweeperWorldProgressState::Completed, 1, 1, false);
    TestEqual(TEXT("Repeated state event preserves claimed quest"), Quests->GetQuestState(GateId), ETunaSweeperQuestState::RewardCompleted);
    TestEqual(TEXT("Repeated event does not grant coins"), Quests->GetCoinBalance(), 5);

    auto* Lift = World->SpawnActor<ATunaSweeperRegionLinkActor>();
    Lift->bUseSlidingDoors = true;
    Lift->RequiredProgressId = TEXT("test.passage.lift");
    FTunaSweeperItemStack Cable; Cable.ItemId = 6010; Cable.Quantity = 1;
    FTunaSweeperItemStack Other; Other.ItemId = 6006; Other.Quantity = 1;
    Lift->RepairParts = {Cable, Other};
    Game->AddItemToFirstAvailableInventorySlot(Cable.ItemId, 1);
    TestFalse(TEXT("Incomplete materials do not repair"), Lift->RepairPassage(false));
    TestEqual(TEXT("Failed repair does not advance quest"), Quests->GetQuestState(LiftId), ETunaSweeperQuestState::Accepted);
    Game->AddItemToFirstAvailableInventorySlot(Other.ItemId, 1);
    bool bObservedConsistentCompletion = false;
    const auto Observer = Quests->OnQuestProgressChanged.AddLambda([&] {
        if (Quests->GetQuestState(LiftId) == ETunaSweeperQuestState::RewardAvailable)
            bObservedConsistentCompletion = Lift->IsPassageUnlocked() &&
                Game->CountInventoryItemById(Cable.ItemId) == 0 && Game->CountInventoryItemById(Other.ItemId) == 0;
    });
    TestTrue(TEXT("Actual two-part repair succeeds"), Lift->RepairPassage(false));
    TestEqual(TEXT("Repair begins with closed sliding doors"), Lift->GetDoorOpenAlpha(), 0.f);
    Lift->Tick(Lift->DoorOpenSeconds);
    TestEqual(TEXT("Actual repair automatically opens the doors"), Lift->GetDoorOpenAlpha(), 1.f);
    Quests->OnQuestProgressChanged.Remove(Observer);
    TestTrue(TEXT("Observers see materials consumed, passage open and objective complete together"), bObservedConsistentCompletion);
    TestEqual(TEXT("Repair completes objective once"), Quests->GetObjectiveProgressCount(LiftId, TEXT("open")), 1);
    TestFalse(TEXT("Repeated repair is rejected"), Lift->RepairPassage(false));

    // Same order as LoadGameState: world objects first, quest progress second.
    const auto SavedWorld = Game->WorldProgressStatesById;
    TArray<FTunaSweeperNumericQuestProgressSaveData> CompletedSave;
    Quests->ExportQuestProgressForSave(CompletedSave, Tracked, Coins);
    Game->WorldProgressStatesById.Reset(); Quests->ResetQuestProgressForNewGame();
    Game->WorldProgressStatesById = SavedWorld;
    Quests->LoadQuestProgressFromSave(CompletedSave, Tracked, Coins);
    TestEqual(TEXT("Reward claim survives load"), Quests->GetQuestState(GateId), ETunaSweeperQuestState::RewardCompleted);
    TestEqual(TEXT("Repaired lift objective survives load"), Quests->GetQuestState(LiftId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("Load does not duplicate coins"), Quests->GetCoinBalance(), 5);
    for (auto& Progress : AcceptedSave) Progress.ObjectiveProgress.Reset();
    Quests->LoadQuestProgressFromSave(AcceptedSave, GateId, 0);
    TestEqual(TEXT("Old accepted save with missing objective count recovers from world state"), Quests->GetQuestState(GateId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("Recovery preserves tracking"), Quests->GetTrackedQuestId(), GateId);
    TestEqual(TEXT("Load recovery does not claim reward"), Quests->GetCoinBalance(), 0);
    TestEqual(TEXT("Load recovery also handles lift without an active actor"), Quests->GetQuestState(LiftId), ETunaSweeperQuestState::RewardAvailable);

    AddQuest(MixedId, TEXT("test.passage.gate"));
    FTunaSweeperObjectiveDefinition Extra;
    Extra.ObjectiveId = TEXT("talk"); Extra.Type = ETunaSweeperObjectiveType::InteractionCompleted; Extra.InteractionEventId = TEXT("test.talk");
    Quests->QuestDefinitions[MixedId].Objectives.Add(Extra);
    Quests->AcceptQuest(MixedId);
    TestEqual(TEXT("Existing opening does not skip other objectives"), Quests->GetQuestState(MixedId), ETunaSweeperQuestState::Accepted);
    Quests->NotifyInteractionCompleted(TEXT("test.talk"), NAME_None);
    TestEqual(TEXT("Other objective still completes normally"), Quests->GetQuestState(MixedId), ETunaSweeperQuestState::RewardAvailable);

    Game->WorldProgressStatesById.Reset(); Quests->ResetQuestProgressForNewGame();
    Quests->AcceptQuest(GateId);
    TestEqual(TEXT("New game cannot inherit prior opening"), Quests->GetQuestState(GateId), ETunaSweeperQuestState::Accepted);
    Game->BeginCombatTestSession();
    Game->UpdateWorldProgressState(TEXT("test.passage.gate"), NAME_None, ETunaSweeperWorldProgressState::Completed, 1, 1, false);
    TestEqual(TEXT("Combat laboratory cannot advance objectives"), Quests->GetQuestState(GateId), ETunaSweeperQuestState::Accepted);
    Game->EndCombatTestSession();
    return true;
}
#endif
