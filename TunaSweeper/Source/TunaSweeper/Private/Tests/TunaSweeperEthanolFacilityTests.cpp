#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperEthanolFacilityActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace EthanolFacilityTests
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperEthanolFacilityTest,
    "TunaSweeper.Interaction.EthanolFacility.ProductionTransactionAndRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperEthanolFacilityTest::RunTest(const FString& Parameters)
{
    // Catches quest-report gating, storage consumption, wrong exchange ratios and partial commits.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    EthanolFacilityTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get();
    World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        Game->UGameInstance::Shutdown();
        World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr;
        EthanolFacilityTests::FContextAccess::Attach(Game.Get(), nullptr);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    // ResetPlayerSlotArrays prepares carried slots only; this fixture also exercises storage exclusion.
    Game->StorageSlots.SetNum(1);
    Game->WorldProgressStatesById.Reset();
    World->InitializeActorsForPlay(FURL());
    auto Configure = [](ATunaSweeperEthanolFacilityActor* Facility)
    {
        auto* Class = Facility->GetClass();
        FindFProperty<FNameProperty>(Class, TEXT("ProgressObjectId"))->SetPropertyValue_InContainer(Facility, TEXT("test.facility.production"));
        FindFProperty<FBoolProperty>(Class, TEXT("bRequireActiveQuest"))->SetPropertyValue_InContainer(Facility, false);
        FindFProperty<FBoolProperty>(Class, TEXT("bRequiresItem"))->SetPropertyValue_InContainer(Facility, false);
        FindFProperty<FIntProperty>(Class, TEXT("ProductionInputItemId"))->SetPropertyValue_InContainer(Facility, 6020);
        FindFProperty<FIntProperty>(Class, TEXT("ProductionOutputItemId"))->SetPropertyValue_InContainer(Facility, 6002);
    };
    auto* Facility = World->SpawnActor<ATunaSweeperEthanolFacilityActor>();
    if (!TestNotNull(TEXT("Production facility spawns"), Facility)) return false;
    Configure(Facility);
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Production uses the real quest subsystem"), Quests)) return false;
    constexpr int32 RepairQuestId = 900300;
    FTunaSweeperQuestDefinition RepairQuest;
    RepairQuest.QuestId = RepairQuestId;
    FTunaSweeperObjectiveDefinition CurrentRepairObjective;
    CurrentRepairObjective.ObjectiveId = TEXT("repair");
    CurrentRepairObjective.Type = ETunaSweeperObjectiveType::WorldProgressCompleted;
    CurrentRepairObjective.WorldProgressId = TEXT("test.facility.production");
    RepairQuest.Objectives = {CurrentRepairObjective};
    Quests->QuestDefinitions.Reset();
    Quests->QuestDefinitions.Add(RepairQuestId, RepairQuest);
    Quests->bQuestDataLoaded = true;
    FTunaSweeperNumericQuestProgressSaveData AcceptedRepair;
    AcceptedRepair.QuestId = RepairQuestId;
    AcceptedRepair.State = ETunaSweeperQuestState::Accepted;
    Quests->LoadQuestProgressFromSave({AcceptedRepair}, RepairQuestId, 0);
    FindFProperty<FBoolProperty>(Facility->GetClass(), TEXT("bRequireActiveQuest"))->SetPropertyValue_InContainer(Facility, true);
    FindFProperty<FIntProperty>(Facility->GetClass(), TEXT("RepairQuestId"))->SetPropertyValue_InContainer(Facility, RepairQuestId);
    FindFProperty<FNameProperty>(Facility->GetClass(), TEXT("RepairObjectiveId"))->SetPropertyValue_InContainer(Facility, TEXT("repair"));
    FindFProperty<FNameProperty>(Facility->GetClass(), TEXT("RepairObjectiveEventId"))->SetPropertyValue_InContainer(Facility, TEXT("test.production.repair"));
    Facility->DispatchBeginPlay();
    TestFalse(TEXT("Broken facility cannot produce"), Facility->Produce(nullptr, false));
    TestTrue(TEXT("Repair completes"), Facility->Repair(false));
    TestEqual(TEXT("Repair makes the real quest ready for reporting"), Quests->GetQuestState(RepairQuestId), ETunaSweeperQuestState::RewardAvailable);
    auto* Interaction = Facility->FindComponentByClass<UTunaSweeperInteractableComponent>();
    TestEqual(TEXT("Repair immediately changes to reusable production prompt"), Interaction->GetInteractionType(), ETunaSweeperInteractionType::WorldProgress);
    TestTrue(TEXT("Production cannot repeat the repair quest event"), Interaction->GetObjectiveEventId().IsNone());
    FindFProperty<FBoolProperty>(Facility->GetClass(), TEXT("bRequireActiveQuest"))->SetPropertyValue_InContainer(Facility, true);
    Facility->RefreshFacilityState();
    TestFalse(TEXT("Missing material rejects production"), Facility->Produce(nullptr, false));
    const FGuid StoredInputUid = Game->CreateItemInstance(6020, 10);
    Game->StorageSlots[0].ItemUid = StoredInputUid;
    TestFalse(TEXT("Storage ingredients cannot pay for production"), Facility->Produce(nullptr, false));
    TestEqual(TEXT("Storage retains the original item identity"), Game->StorageSlots[0].ItemUid, StoredInputUid);
    const auto* StoredInput = Game->ItemInstancesByUid.Find(StoredInputUid);
    if (TestNotNull(TEXT("Storage ingredients remain present"), StoredInput))
        TestEqual(TEXT("Storage ingredients remain untouched"), StoredInput->Quantity, 10);
    TestTrue(TEXT("Supply one carried ingredient"), Game->AddItemToFirstAvailableInventorySlot(6020, 1));
    TestFalse(TEXT("One ingredient cannot pay for a batch"), Facility->Produce(nullptr, false));
    TestEqual(TEXT("Insufficient ingredients remain"), Game->CountInventoryItemById(6020), 1);
    TestTrue(TEXT("Supply remaining ingredients for two batches"), Game->AddItemToFirstAvailableInventorySlot(6020, 3));
    int32 InventoryNotifications = 0;
    const auto Listener = Game->OnInventoryStateChanged.AddLambda([&] { ++InventoryNotifications; });
    TestFalse(TEXT("Refused save rolls back production"), Facility->Produce(nullptr, true));
    TestEqual(TEXT("Failed save retains all inputs"), Game->CountInventoryItemById(6020), 4);
    TestEqual(TEXT("Failed save retains no outputs"), Game->CountInventoryItemById(6002), 0);
    TestEqual(TEXT("Failed save publishes no inventory changes"), InventoryNotifications, 0);
    TestTrue(TEXT("Repaired facility produces before the repair reward is claimed"), Facility->Produce(nullptr, false));
    TestEqual(TEXT("Production never claims the repair quest reward"), Quests->GetQuestState(RepairQuestId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("One press consumes two ingredients"), Game->CountInventoryItemById(6020), 2);
    TestEqual(TEXT("One press produces one result"), Game->CountInventoryItemById(6002), 1);
    TestTrue(TEXT("Next press produces another batch"), Facility->Produce(nullptr, false));
    TestEqual(TEXT("Two presses consume four ingredients"), Game->CountInventoryItemById(6020), 0);
    TestEqual(TEXT("Two presses produce two results"), Game->CountInventoryItemById(6002), 2);
    TestFalse(TEXT("Exhausted inputs reject another batch"), Facility->Produce(nullptr, false));
    TestEqual(TEXT("Only successful batches publish inventory changes"), InventoryNotifications, 2);
    auto* Restored = World->SpawnActor<ATunaSweeperEthanolFacilityActor>();
    Configure(Restored);
    Restored->DispatchBeginPlay();
    TestTrue(TEXT("Recreated facility restores repaired state"), Restored->IsRepaired());
    TestTrue(TEXT("Supply one batch after recreation"), Game->AddItemToFirstAvailableInventorySlot(6020, 2));
    TestTrue(TEXT("Restored facility remains usable"), Restored->Produce(nullptr, false));
    TestEqual(TEXT("Restored production preserves accumulated output"), Game->CountInventoryItemById(6002), 3);
    Game->ResetPlayerSlotArrays();
    Game->StorageSlots.Reset();
    Game->ItemInstancesByUid.Reset();
    for (auto& Slot : Game->PlayerInventorySlots) Slot.ItemUid = Game->CreateItemInstance(3004, 1);
    const FGuid InputUid = Game->PlayerInventorySlots[0].ItemUid;
    Game->ItemInstancesByUid[InputUid].ItemId = 6020;
    Game->ItemInstancesByUid[InputUid].Quantity = 3;
    TestFalse(TEXT("Full inventory rejects the whole batch"), Restored->Produce(nullptr, false));
    TestEqual(TEXT("Full inventory restores input quantity"), Game->CountInventoryItemById(6020), 3);
    TestEqual(TEXT("Full inventory creates no partial output"), Game->CountInventoryItemById(6002), 0);
    Game->OnInventoryStateChanged.Remove(Listener);

    constexpr int32 LegacyQuestId = 900301;
    FTunaSweeperQuestDefinition LegacyQuest;
    LegacyQuest.QuestId = LegacyQuestId;
    FTunaSweeperObjectiveDefinition RepairObjective;
    RepairObjective.ObjectiveId = TEXT("repair");
    RepairObjective.Type = ETunaSweeperObjectiveType::WorldProgressCompleted;
    RepairObjective.WorldProgressId = TEXT("test.facility.legacy.production");
    LegacyQuest.Objectives = {RepairObjective};
    Quests->QuestDefinitions.Add(LegacyQuestId, LegacyQuest);
    Quests->bQuestDataLoaded = true;
    FTunaSweeperNumericQuestProgressSaveData OldProgress;
    OldProgress.QuestId = LegacyQuestId;
    OldProgress.State = ETunaSweeperQuestState::RewardAvailable;
    FTunaSweeperObjectiveProgressSaveData OldObjective;
    OldObjective.ObjectiveId = TEXT("retired.delivery");
    OldObjective.CurrentCount = 1;
    OldProgress.ObjectiveProgress = {OldObjective};
    Quests->LoadQuestProgressFromSave({OldProgress}, LegacyQuestId, 0);
    TestEqual(TEXT("Old ready-to-report save requires the new repair objective"),
        Quests->GetQuestState(LegacyQuestId), ETunaSweeperQuestState::Accepted);
    TestFalse(TEXT("Old objective cannot claim the replacement repair reward"), Quests->CanClaimQuestReward(LegacyQuestId));
    // Construct a claimed-save fixture without reporting synthetic achievements to account storage.
    Quests->QuestProgressById[LegacyQuestId].State = ETunaSweeperQuestState::RewardCompleted;
    auto ConfigureLegacy = [&](ATunaSweeperEthanolFacilityActor* Actor)
    {
        Configure(Actor);
        FindFProperty<FNameProperty>(Actor->GetClass(), TEXT("ProgressObjectId"))->SetPropertyValue_InContainer(Actor, TEXT("test.facility.legacy.production"));
        FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("RepairQuestId"))->SetPropertyValue_InContainer(Actor, LegacyQuestId);
    };
    auto* Legacy = World->SpawnActor<ATunaSweeperEthanolFacilityActor>();
    ConfigureLegacy(Legacy);
    Legacy->DispatchBeginPlay();
    TestFalse(TEXT("Failed migration save leaves facility broken"), Legacy->IsRepaired());
    TestFalse(TEXT("Failed migration retains missing record for next-load retry"),
        Game->WorldProgressStatesById.Contains(TEXT("test.facility.legacy.production")));
    TestEqual(TEXT("Failed migration charges no ingredients"), Game->CountInventoryItemById(6020), 3);
    TestEqual(TEXT("Failed migration preserves claimed quest"), Quests->GetQuestState(LegacyQuestId), ETunaSweeperQuestState::RewardCompleted);
    Game->UpdateWorldProgressState(TEXT("test.facility.legacy.production"), TEXT("world_progress.power_facility"),
        ETunaSweeperWorldProgressState::InProgress, 0, 1, false);
    auto* ExplicitIncomplete = World->SpawnActor<ATunaSweeperEthanolFacilityActor>();
    ConfigureLegacy(ExplicitIncomplete);
    ExplicitIncomplete->DispatchBeginPlay();
    TestFalse(TEXT("Legacy claimed quest never overwrites explicit incomplete state"), ExplicitIncomplete->IsRepaired());
    return true;
}
#endif
