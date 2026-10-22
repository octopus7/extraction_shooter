#if WITH_DEV_AUTOMATION_TESTS
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperRepairableATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"

namespace RepairableATVTests
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRepairableATVTest,
    "TunaSweeper.Vehicle.RepairableATV.RepairAndRewardNextEntry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperRepairableATVTest::RunTest(const FString& Parameters)
{
    // Catches consuming broken/stored parts, partial save commits, immediate replacement and missing reward gates.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    RepairableATVTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get();
    World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Never read/write player save slots.
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        Game->UGameInstance::Shutdown();
        World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr;
        RepairableATVTests::FContextAccess::Attach(Game.Get(), nullptr);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    Game->StorageSlots.SetNum(1);
    Game->WorldProgressStatesById.Reset();
    World->InitializeActorsForPlay(FURL());
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Real initialized quest subsystem"), Quests)) return false;
    Quests->QuestDefinitions.Reset();
    Quests->QuestProgressById.Reset();
    Quests->bQuestDataLoaded = true;
    constexpr int32 RepairQuestId = 900320;
    const FName ProgressId(TEXT("test.atv.repair"));
    FTunaSweeperQuestDefinition Definition;
    Definition.QuestId = RepairQuestId;
    FTunaSweeperObjectiveDefinition Objective;
    Objective.ObjectiveId = TEXT("repair");
    Objective.Type = ETunaSweeperObjectiveType::WorldProgressCompleted;
    Objective.WorldProgressId = ProgressId;
    Definition.Objectives = {Objective};
    Quests->QuestDefinitions.Add(RepairQuestId, Definition);

    auto Configure = [&](ATunaSweeperRepairableATVActor* Actor)
    {
        auto* Class = Actor->GetClass();
        FindFProperty<FNameProperty>(Class, TEXT("ProgressObjectId"))->SetPropertyValue_InContainer(Actor, ProgressId);
        FindFProperty<FIntProperty>(Class, TEXT("RepairQuestId"))->SetPropertyValue_InContainer(Actor, RepairQuestId);
        FindFProperty<FNameProperty>(Class, TEXT("RepairObjectiveId"))->SetPropertyValue_InContainer(Actor, TEXT("repair"));
        FindFProperty<FNameProperty>(Class, TEXT("RepairObjectiveEventId"))->SetPropertyValue_InContainer(Actor, TEXT("test.atv.repair"));
        Actor->RequiredRewardQuestId = RepairQuestId;
        Actor->RepairedVehicleClass = ATunaSweeperATVActor::StaticClass();
    };
    auto CountVehicles = [&]
    {
        int32 Count = 0;
        for (TActorIterator<ATunaSweeperATVActor> It(World); It; ++It) ++Count;
        return Count;
    };
    auto* Broken = World->SpawnActor<ATunaSweeperRepairableATVActor>();
    if (!TestNotNull(TEXT("Separate repair actor spawns"), Broken)) return false;
    Configure(Broken);
    Broken->DispatchBeginPlay();
    auto* Interaction = Broken->FindComponentByClass<UTunaSweeperInteractableComponent>();
    if (!TestNotNull(TEXT("Repair interaction exists"), Interaction)) return false;
    TestNull(TEXT("Repair scenery has no mount interaction"), Broken->FindComponentByClass<UTunaSweeperVehicleMountComponent>());
    TestFalse(TEXT("Repair scenery never simulates vehicle physics"), Broken->VehicleMesh->IsSimulatingPhysics());
    TestFalse(TEXT("Unaccepted quest blocks repair"), Broken->Repair(false));

    FTunaSweeperNumericQuestProgressSaveData Accepted;
    Accepted.QuestId = RepairQuestId;
    Accepted.State = ETunaSweeperQuestState::Accepted;
    Quests->LoadQuestProgressFromSave({Accepted}, RepairQuestId, 0);
    TestTrue(TEXT("Carry a broken spark plug"), Game->AddItemToFirstAvailableInventorySlot(6015, 1));
    TestFalse(TEXT("Broken spark plug cannot repair the vehicle"), Broken->Repair(false));
    const FGuid StoredPlug = Game->CreateItemInstance(6016, 1);
    Game->StorageSlots[0].ItemUid = StoredPlug;
    TestFalse(TEXT("Storage spark plug cannot pay for repair"), Broken->Repair(false));
    TestTrue(TEXT("Carry a working spark plug"), Game->AddItemToFirstAvailableInventorySlot(6016, 1));
    TestFalse(TEXT("Rejected save rolls back repair"), Broken->Repair(true));
    TestEqual(TEXT("Failed save retains the working spark plug"), Game->CountInventoryItemById(6016), 1);
    TestFalse(TEXT("Failed save leaves the ATV broken"), Broken->IsRepaired());
    TestTrue(TEXT("Repair consumes the working spark plug"), Broken->Repair(false));
    TestEqual(TEXT("Exactly one working spark plug was consumed"), Game->CountInventoryItemById(6016), 0);
    TestEqual(TEXT("Broken spark plug remains"), Game->CountInventoryItemById(6015), 1);
    TestEqual(TEXT("Storage plug retains its identity"), Game->StorageSlots[0].ItemUid, StoredPlug);
    TestTrue(TEXT("Repair is persistent world progress"), Broken->IsRepaired());
    TestEqual(TEXT("Repair readies the quest reward"), Quests->GetQuestState(RepairQuestId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("Repaired ATV becomes noninteractive scenery"), Interaction->GetInteractionType(), ETunaSweeperInteractionType::None);
    TestFalse(TEXT("Repair cannot consume another part"), Broken->Repair(false));
    TestEqual(TEXT("Repair does not spawn a drivable vehicle this visit"), CountVehicles(), 0);

    auto* AwaitingReward = World->SpawnActor<ATunaSweeperRepairableATVActor>();
    Configure(AwaitingReward);
    AwaitingReward->DispatchBeginPlay();
    TestTrue(TEXT("Next entry restores repaired scenery"), AwaitingReward->IsRepaired());
    TestEqual(TEXT("Unclaimed reward still blocks a vehicle on next entry"), CountVehicles(), 0);

    // A claimed-save fixture avoids reporting synthetic achievements into account storage.
    Quests->QuestProgressById[RepairQuestId].State = ETunaSweeperQuestState::RewardCompleted;
    Quests->OnQuestProgressChanged.Broadcast();
    Game->OnWorldProgressChanged.Broadcast();
    TestEqual(TEXT("Claim/world notifications never replace actors already in this visit"), CountVehicles(), 0);
    TestFalse(TEXT("Existing repaired scenery remains after reward notification"), AwaitingReward->IsActorBeingDestroyed());

    const auto SavedWorld = Game->WorldProgressStatesById;
    TArray<FTunaSweeperNumericQuestProgressSaveData> SavedQuests;
    int32 Tracked = 0, Coins = 0;
    Quests->ExportQuestProgressForSave(SavedQuests, Tracked, Coins);
    Game->WorldProgressStatesById.Reset();
    Quests->ResetQuestProgressForNewGame();
    Game->WorldProgressStatesById = SavedWorld;
    Quests->LoadQuestProgressFromSave(SavedQuests, Tracked, Coins);
    TestEqual(TEXT("Loading state does not replace existing scenery"), CountVehicles(), 0);

    auto* Unconfigured = World->SpawnActor<ATunaSweeperRepairableATVActor>();
    Configure(Unconfigured);
    Unconfigured->RequiredRewardQuestId = 0;
    Unconfigured->DispatchBeginPlay();
    TestEqual(TEXT("Missing reward quest configuration fails closed"), CountVehicles(), 0);
    auto* MissingRepair = World->SpawnActor<ATunaSweeperRepairableATVActor>();
    Configure(MissingRepair);
    FindFProperty<FNameProperty>(MissingRepair->GetClass(), TEXT("ProgressObjectId"))->SetPropertyValue_InContainer(MissingRepair, TEXT("test.atv.not_repaired"));
    MissingRepair->DispatchBeginPlay();
    TestEqual(TEXT("Reward alone cannot replace an unrepaired vehicle"), CountVehicles(), 0);

    const FVector ExpectedLocation(900.0, 1200.0, 80.0);
    const FRotator ExpectedRotation(0.0, 37.0, 0.0);
    auto* NextEntry = World->SpawnActor<ATunaSweeperRepairableATVActor>(ExpectedLocation, ExpectedRotation);
    Configure(NextEntry);
    NextEntry->DispatchBeginPlay();
    TestEqual(TEXT("Next entry after claimed reward spawns one drivable ATV"), CountVehicles(), 1);
    TestTrue(TEXT("Successful replacement removes repair scenery"), NextEntry->IsActorBeingDestroyed());
    for (TActorIterator<ATunaSweeperATVActor> It(World); It; ++It)
    {
        TestTrue(TEXT("Replacement preserves placement location"), It->GetActorLocation().Equals(ExpectedLocation));
        TestTrue(TEXT("Replacement preserves placement rotation"), It->GetActorRotation().Equals(ExpectedRotation));
    }
    Quests->OnQuestProgressChanged.Broadcast();
    Game->OnWorldProgressChanged.Broadcast();
    TestEqual(TEXT("Further state notifications cannot duplicate the vehicle"), CountVehicles(), 1);
    return true;
}
#endif
