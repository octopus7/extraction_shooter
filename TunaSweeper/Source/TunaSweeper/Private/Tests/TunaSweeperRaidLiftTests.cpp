#if WITH_DEV_AUTOMATION_TESTS
#include "Interaction/TunaSweeperRaidLiftActor.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRaidLiftPlatformTest,
    "TunaSweeper.RaidLift.PlatformAndLockedInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperRaidLiftPlatformTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); World->RemoveFromRoot(); };
    auto* Lift = World->SpawnActor<ATunaSweeperRaidLiftActor>();
    auto* Pawn = World->SpawnActor<APawn>();
    if (!TestNotNull(TEXT("Lift"), Lift) || !TestNotNull(TEXT("Pawn"), Pawn)) return false;
    // APawn has no root by default; supply one so test locations are real.
    auto* Root = NewObject<USceneComponent>(Pawn);
    Pawn->SetRootComponent(Root);
    Root->RegisterComponent();
    Lift->SetActorLocationAndRotation(FVector(500, 300, 40), FRotator(0, 90, 0));
    const auto Place = [Lift, Pawn](const FVector& Local) { Pawn->SetActorLocation(Lift->GetActorTransform().TransformPosition(Local)); };
    Place(FVector(0, 0, 108));
    TestTrue(TEXT("Standing pawn is accepted on rotated lift"), Lift->IsPawnOnPlatform(Pawn));
    TestFalse(TEXT("Unrepaired lift cannot open picker"), Lift->TravelToTargetLevel(Pawn));
    TestFalse(TEXT("Selection without open picker cannot travel"), Lift->SelectArrival(ETunaSweeperRaidArrivalGroup::Forest));
    Place(FVector(160, 0, 108));
    TestFalse(TEXT("Nearby pawn outside platform is rejected"), Lift->IsPawnOnPlatform(Pawn));
    Place(FVector(0, 0, -20));
    TestFalse(TEXT("Pawn underneath lift is rejected"), Lift->IsPawnOnPlatform(Pawn));
    Place(FVector(0, 0, 300));
    TestFalse(TEXT("Pawn above lift is rejected"), Lift->IsPawnOnPlatform(Pawn));
    TestFalse(TEXT("Missing pawn is rejected"), Lift->IsPawnOnPlatform(nullptr));
    return true;
}


namespace RaidLiftTest
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRaidLiftRepairTest,
    "TunaSweeper.RaidLift.CapacitorRepairAndPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRaidLiftRepairTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    RaidLiftTest::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get();
    World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        RaidLiftTest::FContextAccess::Attach(Game.Get(), nullptr); World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    Game->WorldProgressStatesById.Reset();
    auto* Lift = World->SpawnActor<ATunaSweeperRaidLiftActor>();
    if (!TestNotNull(TEXT("Lift spawns"), Lift)) return false;
    TestTrue(TEXT("Unrelated part added"), Game->AddItemToFirstAvailableInventorySlot(6010, 1));
    TestFalse(TEXT("Wrong part cannot repair lift"), Lift->RepairPassage(false));
    TestEqual(TEXT("Rejected repair preserves unrelated part"), Game->CountInventoryItemById(6010), 1);
    TestFalse(TEXT("Rejected repair does not unlock lift"), Lift->IsPassageUnlocked());
    TestTrue(TEXT("Capacitors added"), Game->AddItemToFirstAvailableInventorySlot(6025, 2));
    TestTrue(TEXT("One capacitor repairs lift"), Lift->RepairPassage(false));
    TestEqual(TEXT("Exactly one capacitor is consumed"), Game->CountInventoryItemById(6025), 1);
    TestFalse(TEXT("Duplicate repair is rejected"), Lift->RepairPassage(false));
    TestEqual(TEXT("Duplicate repair preserves remaining capacitor"), Game->CountInventoryItemById(6025), 1);
    FTunaSweeperWorldProgressSaveData Saved;
    TestTrue(TEXT("Repair produces persisted world progress"), Game->TryGetWorldProgressState(TEXT("world.passage.raid_lift"), Saved));
    const auto SavedProgress = Game->WorldProgressStatesById;
    Game->WorldProgressStatesById.Reset();
    TestFalse(TEXT("Fresh progression locks lift"), Lift->IsPassageUnlocked());
    Game->WorldProgressStatesById = SavedProgress;
    auto* Restored = World->SpawnActor<ATunaSweeperRaidLiftActor>();
    if (!TestNotNull(TEXT("Restored lift spawns"), Restored)) return false;
    TestTrue(TEXT("Restored progression unlocks every lift instance"), Restored->IsPassageUnlocked());
    TestFalse(TEXT("Restored lift cannot consume another capacitor"), Restored->RepairPassage(false));
    TestEqual(TEXT("Restored lift preserves remaining capacitor"), Game->CountInventoryItemById(6025), 1);
    return true;
}
#endif
