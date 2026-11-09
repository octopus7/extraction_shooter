#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "UObject/UnrealType.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Interaction/TunaSweeperRaidStorageActor.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace RaidStorageTests
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance, FWorldContext* Context)
        {
            auto Member = &FContextAccess::WorldContext;
            Instance->*Member = Context;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRaidStorageContractTest,
    "TunaSweeper.RaidStorage.Contract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRaidStorageContractTest::RunTest(const FString& Parameters)
{
    TestNotNull(TEXT("Secondary warehouse is serialized in the gameplay save"),
        FindFProperty<FArrayProperty>(UTunaSweeperSaveGame::StaticClass(), TEXT("RaidStorageSlots")));
    TestNotNull(TEXT("Placeable persistent chest actor class exists"),
        FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperRaidStorageActor")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRaidStoragePersistenceTest,
    "TunaSweeper.RaidStorage.Persistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRaidStoragePersistenceTest::RunTest(const FString& Parameters)
{
    // Real save/load tests must run under an isolated engine user directory.
    if (!FPaths::ProjectSavedDir().Contains(TEXT("RaidStorageAutomation")))
    {
        AddWarning(TEXT("Disk round-trip requires -UserDir=<workspace>/Saved/RaidStorageAutomation/; no user saves touched."));
        return true;
    }
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    RaidStorageTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT
    {
        Game->UGameInstance::Shutdown();
        World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        RaidStorageTests::FContextAccess::Attach(Game.Get(), nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    Game->StorageSlots.SetNum(100);
    Game->RaidStorageSlots.SetNum(10);
    Game->ActiveSaveSlotIndex = 1;
    const FGuid Tuna = Game->CreateItemInstance(3004, 10);
    Game->PlayerInventorySlots[0].ItemUid = Tuna;
    UObject* First = NewObject<UTunaSweeperSaveGame>(Game.Get());
    UObject* Second = NewObject<UTunaSweeperSaveGame>(Game.Get());
    Game->BeginRaidExperienceSession();
    FTunaSweeperLootContainerInstance OrdinaryLoot;
    OrdinaryLoot.Capacity = 1;
    FTunaSweeperItemStack OrdinaryTuna;
    OrdinaryTuna.ItemId = 3004; OrdinaryTuna.Quantity = 1;
    OrdinaryLoot.Items.Add(OrdinaryTuna);
    Game->SetActiveLootContainerInstance(OrdinaryLoot, First);
    FTunaSweeperItemSlotReference LootSource, LootTarget;
    LootSource.Source = ETunaSweeperItemSlotSource::LootContainer; LootSource.SlotIndex = 0;
    LootTarget.Source = ETunaSweeperItemSlotSource::Inventory; LootTarget.SlotIndex = 1;
    TestTrue(TEXT("Positive control: ordinary loot withdrawal"), Game->MoveItemBetweenSlots(LootSource, LootTarget));
    TestTrue(TEXT("Positive control: ordinary loot awards experience"), Game->PendingRaidExperiencePoints > 0);
    Game->OpenRaidStorage(First, FText::GetEmpty());
    TestEqual(TEXT("Exactly ten slots"), Game->GetActiveLootContainerCapacity(), 10);
    FTunaSweeperItemSlotReference Inventory, Chest;
    Inventory.Source = ETunaSweeperItemSlotSource::Inventory; Inventory.SlotIndex = 0;
    Chest.Source = ETunaSweeperItemSlotSource::LootContainer; Chest.SlotIndex = 0;
    TestTrue(TEXT("Deposit"), Game->MoveItemBetweenSlots(Inventory, Chest));
    const FString SavePath = Game->GetSaveGameFilePath(Game->GetSaveGameSlotName(1));
    auto Read = [&]() { return Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadSaveFileWithRecovery(
        SavePath, {}, [](const USaveGame& Save) { return Save.IsA<UTunaSweeperSaveGame>(); })); };
    auto* Saved = Read();
    if (!TestNotNull(TEXT("Deposit saved immediately without closing UI"), Saved)) return false;
    TestEqual(TEXT("Disk contains deposited item"), Saved->RaidStorageSlots[0].ItemUid, Tuna);
    TestFalse(TEXT("Disk inventory no longer owns deposited item"), Saved->InventorySlots[0].ItemUid.IsValid());
    Game->CloseRaidStorage(First);
    Game->OpenRaidStorage(Second, FText::GetEmpty());
    TestEqual(TEXT("Another instance shares same contents"), Game->GetActiveLootContainerSlots()[0].ItemUid, Tuna);
    const int64 Experience = Game->PendingRaidExperiencePoints;
    TestTrue(TEXT("Withdraw"), Game->MoveItemBetweenSlots(Chest, Inventory));
    TestEqual(TEXT("Warehouse withdrawal grants no loot experience"), Game->PendingRaidExperiencePoints, Experience);
    Saved = Read();
    TestTrue(TEXT("Withdrawal persisted"), Saved && !Saved->RaidStorageSlots[0].ItemUid.IsValid());
    TestTrue(TEXT("Deposit again"), Game->MoveItemBetweenSlots(Inventory, Chest));
    TestTrue(TEXT("Partial withdrawal"), Game->SplitItemStackBetweenSlots(Chest, Inventory, 3));
    TestEqual(TEXT("Split warehouse withdrawal grants no loot experience"), Game->PendingRaidExperiencePoints, Experience);
    Saved = Read();
    if (TestNotNull(TEXT("Split saved immediately"), Saved))
    {
        const auto* Remaining = Saved->ItemInstances.FindByPredicate([&](const auto& Item) { return Item.Uid == Saved->RaidStorageSlots[0].ItemUid; });
        TestTrue(TEXT("Seven tuna remain on disk"), Remaining && Remaining->Quantity == 7);
    }
    TestTrue(TEXT("Merge partial stack back"), Game->MoveItemBetweenSlots(Inventory, Chest));
    Game->CloseRaidStorage(Second);
    const FGuid Carried = Game->CreateItemInstance(3004, 2);
    Game->PlayerInventorySlots[0].ItemUid = Carried;
    TestTrue(TEXT("Death save succeeds"), Game->ClearInventoryAndSaveInternal(false));
    TestTrue(TEXT("Stored item survives death"), Game->ItemInstancesByUid.Contains(Tuna));
    TestFalse(TEXT("Carried item does not survive death"), Game->ItemInstancesByUid.Contains(Carried));
    Game->RaidStorageSlots.Reset(); Game->ItemInstancesByUid.Reset();
    TestTrue(TEXT("Reload after death"), Game->LoadGameState());
    TestEqual(TEXT("Storage survives disk reload"), Game->GetRaidStorageSlots()[0].ItemUid, Tuna);
    Game->OpenRaidStorage(First, FText::GetEmpty());
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // deterministic refused write
    TestTrue(TEXT("Runtime withdrawal remains available when disk refuses write"), Game->MoveItemBetweenSlots(Chest, Inventory));
    TestTrue(TEXT("Failed write stays pending"), Game->bPendingRaidStorageSave);
    Game->RetiredDemoSaveSlotIndex = INDEX_NONE;
    TestTrue(TEXT("Failed save can retry"), Game->FlushRaidStorageSave());
    TestFalse(TEXT("Successful retry clears dirty state"), Game->bPendingRaidStorageSave);
    Game->ResetRuntimeStateForSaveSlotSelection();
    TestFalse(TEXT("Switching slots drops active chest"), Game->IsRaidStorageOpen());
    TestTrue(TEXT("Switching slots clears secondary storage"), Game->RaidStorageSlots[0].IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRaidStoragePresentationTest,
    "TunaSweeper.RaidStorage.Presentation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRaidStoragePresentationTest::RunTest(const FString& Parameters)
{
    const auto* Defaults = GetDefault<ATunaSweeperRaidStorageActor>();
    TestEqual(TEXT("Empty is off"), Defaults->GetSlotIndicatorColor(INDEX_NONE, 0), FLinearColor::Black);
    TestEqual(TEXT("Ten tuna is full green"), Defaults->GetSlotIndicatorColor(3004, 10), Defaults->FullTunaColor);
    for (int32 Qty = 1; Qty < 10; ++Qty)
        TestEqual(TEXT("Partial tuna is half green"), Defaults->GetSlotIndicatorColor(3004, Qty), Defaults->FullTunaColor * 0.5f);
    TestEqual(TEXT("Gourmet tuna is a different item"), Defaults->GetSlotIndicatorColor(6035, 10), Defaults->OtherItemColor);
    UClass* Class = LoadClass<ATunaSweeperRaidStorageActor>(nullptr,
        TEXT("/Game/Interactables/RaidStorage/BP_RaidStorage.BP_RaidStorage_C"));
    if (!TestNotNull(TEXT("Real Blueprint asset loads"), Class)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); World->RemoveFromRoot(); };
    auto* Actor = World->SpawnActor<ATunaSweeperRaidStorageActor>(Class);
    if (!TestNotNull(TEXT("Blueprint spawns"), Actor)) return false;
    TInlineComponentArray<UStaticMeshComponent*> Meshes(Actor);
    TInlineComponentArray<ULightComponent*> Lights(Actor);
    TestEqual(TEXT("Indicators never use lights"), Lights.Num(), 0);
    for (int32 Index = 0; Index < 10; ++Index)
    {
        const FName Tag(*FString::Printf(TEXT("RaidStorageLED_%d"), Index));
        int32 Count = 0;
        for (auto* Mesh : Meshes) if (Mesh->ComponentHasTag(Tag))
        {
            ++Count;
            TestNotNull(TEXT("LED has a mesh"), Mesh->GetStaticMesh().Get());
            TestNotNull(TEXT("LED has a material"), Mesh->GetMaterial(0));
            TestFalse(TEXT("LED is not a Lumen light source"), bool(Mesh->bEmissiveLightSource));
            TestEqual(TEXT("Five-column alignment"), Mesh->GetRelativeLocation().Y, (2 - Index % 5) * 9.0);
        }
        TestEqual(TEXT("Exactly one LED per storage slot"), Count, 1);
    }
    World->SpawnActor<ATunaSweeperRaidStorageActor>(Class);
    TestEqual(TEXT("Duplicate placements detected"), Actor->CountLoadedStorageActors(), 2);
    return true;
}
#endif
