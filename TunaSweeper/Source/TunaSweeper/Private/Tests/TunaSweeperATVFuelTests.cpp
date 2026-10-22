#if WITH_DEV_AUTOMATION_TESTS
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include <limits>

namespace ATVFuelTests
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVFuelTest,
    "TunaSweeper.Vehicle.Fuel.ConsumptionRefuelAndQuestCredit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperATVFuelTest::RunTest(const FString& Parameters)
{
    // Catches incorrect consumption rates, parked fuel loss, free/duplicate refills, and failed-attempt quest credit.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    ATVFuelTests::FContextAccess::Attach(Game.Get(), &Context);
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
        ATVFuelTests::FContextAccess::Attach(Game.Get(), nullptr);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    Game->StorageSlots.SetNum(1);
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Real initialized quest subsystem"), Quests)) return false;
    Quests->QuestDefinitions.Reset();
    Quests->QuestProgressById.Reset();
    Quests->bQuestDataLoaded = true;

    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* ATV = World->SpawnActor<ATunaSweeperATVActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector(0, 140, 90), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("ATV spawns"), ATV) || !TestNotNull(TEXT("Player spawns"), Player)) return false;
    ATV->VehicleMesh->SetSimulatePhysics(false);
    auto* Mount = ATV->MountComponent.Get();
    Mount->EngineStartSound = Mount->EngineIdleSound = Mount->EngineStopSound = nullptr;
    Mount->EngineDriveSound = Mount->EngineBoostSound = nullptr;
    World->InitializeActorsForPlay(FURL());
    ATV->DispatchBeginPlay();
    TestFalse(TEXT("ATV begins with an empty tank"), ATV->HasFuel());
    TestFalse(TEXT("Empty ATV cannot be mounted"), Mount->TryMount(Player));
    TestNull(TEXT("Failed empty mount leaves player on foot"), Mount->GetRider());
    TestEqual(TEXT("Empty ATV offers refueling"), Mount->GetInteractionType(), ETunaSweeperInteractionType::WorldProgress);

    ATV->FuelSeconds = 60.f;
    ATV->RefreshFuelInteraction();
    ATV->ConsumeFuel(600.f, false, true);
    TestEqual(TEXT("Engine-off movement consumes no fuel"), ATV->FuelSeconds, 60.f);
    ATV->ConsumeFuel(600.f, false, false);
    TestEqual(TEXT("Engine-off idle consumes no fuel"), ATV->FuelSeconds, 60.f);
    ATV->Tick(600.f);
    TestEqual(TEXT("Real unmounted vehicle tick consumes no fuel"), ATV->FuelSeconds, 60.f);
    for (const float InvalidDelta : {0.f, -1.f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
    {
        ATV->ConsumeFuel(InvalidDelta, true, true);
        TestEqual(TEXT("Invalid elapsed time leaves fuel unchanged"), ATV->FuelSeconds, 60.f);
    }
    ATV->ConsumeFuel(30.f, true, true);
    TestEqual(TEXT("Thirty seconds of driving consumes half a tank"), ATV->FuelSeconds, 30.f);
    TestEqual(TEXT("HUD ratio reflects half a tank"), ATV->GetFuelRatio(), 0.5f);
    ATV->ConsumeFuel(29.f, true, true);
    TestTrue(TEXT("Driving tank still has fuel before one minute"), ATV->HasFuel());
    TestEqual(TEXT("Fifty-nine seconds leaves one driving second"), ATV->FuelSeconds, 1.f);
    ATV->ConsumeFuel(1.f, true, true);
    TestFalse(TEXT("One minute of driving empties a full tank"), ATV->HasFuel());
    TestEqual(TEXT("Empty tank displays zero"), ATV->GetFuelRatio(), 0.f);
    ATV->ConsumeFuel(10.f, true, true);
    TestEqual(TEXT("Empty tank cannot become negative"), ATV->FuelSeconds, 0.f);
    ATV->FuelSeconds = 60.f;
    ATV->ConsumeFuel(300.f, true, false);
    TestEqual(TEXT("Five minutes of idle consumes half a tank"), ATV->FuelSeconds, 30.f);
    ATV->ConsumeFuel(300.f, true, false);
    TestFalse(TEXT("Ten minutes of idle empties a full tank"), ATV->HasFuel());
    ATV->FuelSeconds = 60.f;
    ATV->ConsumeFuel(100.f, true, true);
    TestEqual(TEXT("Consumption crossing empty clamps at zero"), ATV->FuelSeconds, 0.f);

    constexpr int32 RefuelQuestId = 900321;
    FTunaSweeperQuestDefinition Definition;
    Definition.QuestId = RefuelQuestId;
    FTunaSweeperObjectiveDefinition Objective;
    Objective.ObjectiveId = TEXT("refuel");
    Objective.Type = ETunaSweeperObjectiveType::InteractionCompleted;
    Objective.InteractionEventId = TEXT("test.atv.refuel");
    Objective.InteractionTypeName = TEXT("world_progress");
    Objective.RequiredCount = 2;
    Definition.Objectives = {Objective};
    Quests->QuestDefinitions.Add(RefuelQuestId, Definition);
    FTunaSweeperNumericQuestProgressSaveData Accepted;
    Accepted.QuestId = RefuelQuestId;
    Accepted.State = ETunaSweeperQuestState::Accepted;
    Quests->LoadQuestProgressFromSave({Accepted}, RefuelQuestId, 0);
    ATV->RefuelObjectiveEventId = TEXT("test.atv.refuel");
    ATV->RefreshFuelInteraction();
    TestFalse(TEXT("Missing ethanol rejects refueling"), ATV->Refuel(Player));
    TestTrue(TEXT("Carry unrelated material"), Game->AddItemToFirstAvailableInventorySlot(6016, 1));
    TestFalse(TEXT("Unrelated material cannot fuel the ATV"), ATV->Refuel(Player));
    const FGuid StoredEthanol = Game->CreateItemInstance(6009, 1);
    Game->StorageSlots[0].ItemUid = StoredEthanol;
    TestFalse(TEXT("Storage ethanol cannot pay for refueling"), ATV->Refuel(Player));
    TestEqual(TEXT("Failed refueling never advances the objective"), Quests->GetObjectiveProgressCount(RefuelQuestId, TEXT("refuel")), 0);
    TestTrue(TEXT("Carry ethanol for two refills"), Game->AddItemToFirstAvailableInventorySlot(6009, 2));
    TestFalse(TEXT("Null instigator cannot refuel"), ATV->Refuel(nullptr));
    Player->SetActorLocation(FVector(10000, 0, 90));
    TestFalse(TEXT("Remote player cannot refuel"), ATV->Refuel(Player));
    Player->SetActorLocation(FVector(0, 140, 10000));
    TestFalse(TEXT("Player on another floor cannot refuel"), ATV->Refuel(Player));
    Player->SetActorLocation(FVector(0, 140, 90));

    bool bObservedCommittedRefuel = false;
    const auto Observer = Quests->OnQuestProgressChanged.AddLambda([&]
    {
        if (Quests->GetObjectiveProgressCount(RefuelQuestId, TEXT("refuel")) == 1)
            bObservedCommittedRefuel = ATV->FuelSeconds == 60.f && Game->CountInventoryItemById(6009) == 1;
    });
    TestTrue(TEXT("One carried ethanol refuels an empty ATV"), ATV->Refuel(Player));
    Quests->OnQuestProgressChanged.Remove(Observer);
    TestTrue(TEXT("Quest observers see fuel and consumed ethanol together"), bObservedCommittedRefuel);
    TestEqual(TEXT("One refill consumes exactly one ethanol"), Game->CountInventoryItemById(6009), 1);
    TestEqual(TEXT("Refill leaves unrelated material untouched"), Game->CountInventoryItemById(6016), 1);
    TestEqual(TEXT("Refill leaves stored ethanol untouched"), Game->StorageSlots[0].ItemUid, StoredEthanol);
    TestEqual(TEXT("Refill grants one minute of driving"), ATV->FuelSeconds, 60.f);
    TestEqual(TEXT("Refill displays a full tank"), ATV->GetFuelRatio(), 1.f);
    TestEqual(TEXT("Fueled ATV switches to mounting"), Mount->GetInteractionType(), ETunaSweeperInteractionType::VehicleMount);
    TestEqual(TEXT("Successful refill advances objective exactly once"), Quests->GetObjectiveProgressCount(RefuelQuestId, TEXT("refuel")), 1);
    TestFalse(TEXT("A fueled tank cannot be refilled"), ATV->Refuel(Player));
    TestEqual(TEXT("Rejected duplicate refill preserves ethanol"), Game->CountInventoryItemById(6009), 1);
    TestEqual(TEXT("Rejected duplicate refill gives no extra credit"), Quests->GetObjectiveProgressCount(RefuelQuestId, TEXT("refuel")), 1);
    ATV->ConsumeFuel(5.f, true, true);
    TestFalse(TEXT("A partially used tank also cannot be refilled"), ATV->Refuel(Player));
    TestEqual(TEXT("Rejected partial refill preserves remaining fuel"), ATV->FuelSeconds, 55.f);
    TestEqual(TEXT("Rejected partial refill preserves ethanol"), Game->CountInventoryItemById(6009), 1);

    TestTrue(TEXT("Fuel enables mounting"), Mount->TryMount(Player));
    ATV->RefreshFuelInteraction();
    TestEqual(TEXT("Occupied ATV offers no outside interaction"), Mount->GetInteractionType(), ETunaSweeperInteractionType::None);
    ATV->Tick(10.f);
    TestEqual(TEXT("Mounted idle ticks consume one tenth of driving fuel"), ATV->FuelSeconds, 54.f);
    ATV->SetDriveInput(FVector2D(0, 1));
    ATV->Tick(10.f);
    TestEqual(TEXT("Throttle ticks use the driving consumption rate"), ATV->FuelSeconds, 44.f);
    ATV->SetBoostInput(true);
    ATV->Tick(5.f);
    TestEqual(TEXT("Boost keeps the requested driving fuel rate"), ATV->FuelSeconds, 39.f);
    ATV->ConsumeFuel(39.f, true, true);
    TestFalse(TEXT("Fuel depletion prevents further drive input"), ATV->CanAcceptDriveInput());
    TestTrue(TEXT("Fuel depletion clears held throttle"), ATV->GetDriveInput().IsNearlyZero());
    TestFalse(TEXT("Fuel depletion clears boost"), ATV->IsBoosting());
    TestEqual(TEXT("Fuel depletion keeps the rider seated"), Mount->GetRider(), Player);
    TestFalse(TEXT("Occupied empty vehicle cannot be refueled"), ATV->Refuel(Player));
    Mount->ReleaseRiderForEndPlay();
    ATV->RefreshFuelInteraction();
    TestFalse(TEXT("Empty ATV still blocks mounting after dismount"), Mount->CanMount(Player));
    TestTrue(TEXT("Empty ATV can be refueled again after dismount"), ATV->Refuel(Player));
    TestEqual(TEXT("Second successful refill consumes the second ethanol"), Game->CountInventoryItemById(6009), 0);
    TestEqual(TEXT("Only two successful refills finish the objective"), Quests->GetObjectiveProgressCount(RefuelQuestId, TEXT("refuel")), 2);
    TestEqual(TEXT("Refill completion still requires claiming the reward"), Quests->GetQuestState(RefuelQuestId), ETunaSweeperQuestState::RewardAvailable);
    ATV->FuelSeconds = 0.f;
    ATV->bVehicleDestroyed = true;
    ATV->RefreshFuelInteraction();
    TestTrue(TEXT("Carry ethanol beside a destroyed ATV"), Game->AddItemToFirstAvailableInventorySlot(6009, 1));
    TestFalse(TEXT("Destroyed ATV cannot be refueled"), ATV->Refuel(Player));
    TestEqual(TEXT("Destroyed ATV offers no refill interaction"), Mount->GetInteractionType(), ETunaSweeperInteractionType::None);
    TestEqual(TEXT("Destroyed ATV leaves ethanol untouched"), Game->CountInventoryItemById(6009), 1);

    // Recreated actors must recover fuel by stable placement, while a completed raid rejects stale writers.
    auto SpawnSessionVehicle = [&](FName PlacementId)
    {
        auto* Vehicle = World->SpawnActor<ATunaSweeperATVActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        if (TestNotNull(TEXT("Session vehicle spawns"), Vehicle))
        {
            Vehicle->VehicleMesh->SetSimulatePhysics(false);
            Vehicle->FuelSessionId = PlacementId;
            Vehicle->InitializeSessionFuel();
            Vehicle->RefreshFuelInteraction();
        }
        return Vehicle;
    };
    const FName SessionPlacement(TEXT("test.atv.fuel.session"));
    auto* SessionVehicle = SpawnSessionVehicle(SessionPlacement);
    if (!SessionVehicle) return false;
    TestFalse(TEXT("New placement has no cached fuel"), SessionVehicle->HasFuel());
    Player->SetActorLocation(FVector(0, 140, 90));
    TestTrue(TEXT("Refuel session vehicle with the remaining carried ethanol"), SessionVehicle->Refuel(Player));
    auto* RefueledCopy = SpawnSessionVehicle(SessionPlacement);
    if (!RefueledCopy) return false;
    TestTrue(TEXT("Recreated vehicle has a different runtime actor name"), RefueledCopy->GetFName() != SessionVehicle->GetFName());
    TestEqual(TEXT("Refueling stores fuel under the stable placement ID"), RefueledCopy->FuelSeconds, 60.f);
    SessionVehicle->ConsumeFuel(13.f, true, true);
    auto* ConsumedCopy = SpawnSessionVehicle(SessionPlacement);
    if (!ConsumedCopy) return false;
    TestEqual(TEXT("Recreated placement restores remaining fuel after consumption"), ConsumedCopy->FuelSeconds, 47.f);
    auto* OtherPlacement = SpawnSessionVehicle(TEXT("test.atv.fuel.other"));
    if (!OtherPlacement) return false;
    TestFalse(TEXT("A different placement cannot borrow another vehicle's fuel"), OtherPlacement->HasFuel());

    Game->HandleLevelTravelPersistence(TEXT("RaidForest"), TEXT("RaidVillage"));
    Game->HandleLevelTravelPersistence(TEXT("RaidVillage"), TEXT("RaidForest"));
    auto* RaidReturnVehicle = SpawnSessionVehicle(SessionPlacement);
    if (!RaidReturnVehicle) return false;
    TestEqual(TEXT("Travel between raid maps preserves fuel when revisiting the placement"), RaidReturnVehicle->FuelSeconds, 47.f);
    Game->HandleLevelTravelPersistence(TEXT("RaidVillage"), TEXT("BunkerMap"));
    auto* NextRaidVehicle = SpawnSessionVehicle(SessionPlacement);
    if (!NextRaidVehicle) return false;
    TestFalse(TEXT("Returning to base clears fuel for the next raid"), NextRaidVehicle->HasFuel());
    SessionVehicle->ConsumeFuel(1.f, true, true);
    TestEqual(TEXT("Old actor really attempts a final consumption update"), SessionVehicle->FuelSeconds, 46.f);
    auto* AfterStaleWrite = SpawnSessionVehicle(SessionPlacement);
    if (!AfterStaleWrite) return false;
    TestFalse(TEXT("An old raid actor cannot repopulate cleared session fuel"), AfterStaleWrite->HasFuel());
    return true;
}
#endif
