#if WITH_DEV_AUTOMATION_TESTS
#include "Game/TunaSweeperGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperVolatileArrivalTest,
    "TunaSweeper.Inventory.VolatileArrival",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperVolatileArrivalTest::RunTest(const FString&)
{
    struct FContextAccess : UGameInstance {
        static void Attach(UGameInstance* Game, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("DeliveryRaidFixture"),
        CreatePackage(TEXT("/Temp/VolatileArrival/DeliveryRaidFixture")));
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context); Context.OwningGameInstance = Game; World->SetGameInstance(Game);
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Never touch player saves.
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game, nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    TestTrue(TEXT("Carry jar"), Game->AddItemToFirstAvailableInventorySlot(6011, 2));
    TestTrue(TEXT("Carry boat"), Game->AddItemToFirstAvailableInventorySlot(6017, 1));
    TestTrue(TEXT("Carry ordinary food"), Game->AddItemToFirstAvailableInventorySlot(3004, 1));
    const FGuid BagParcel = Game->CreateItemInstance(6017, 1);
    Game->AuxiliaryBagSlots.SetNum(1); Game->AuxiliaryBagSlots[0].ItemUid = BagParcel;
    Game->UsableQuickSlots.SetNum(1); Game->UsableQuickSlots[0].ItemUid = BagParcel;
    const FGuid StoredFood = Game->CreateItemInstance(3004, 3);
    Game->StorageSlots.SetNum(1); Game->StorageSlots[0].ItemUid = StoredFood;
    Game->HandleLevelTravelPersistence(TEXT("RaidVillage"), TEXT("RaidForest"));
    FCoreUObjectDelegates::PostLoadMapWithWorld.Broadcast(World);
    TestEqual(TEXT("Raid travel retains jar"), Game->CountInventoryItemById(6011), 2);
    TestTrue(TEXT("Raid travel retains bag parcel"), Game->ItemInstancesByUid.Contains(BagParcel));
    Game->HandleLevelTravelPersistence(TEXT("RaidVillage"), TEXT("BunkerMap"));
    GEngine->OnTravelFailure().Broadcast(World, ETravelFailure::InvalidURL, TEXT("Synthetic failure"));
    TestEqual(TEXT("A failed return must not destroy cargo"), Game->CountInventoryItemById(6011), 2);
    World->GetOutermost()->Rename(TEXT("/Temp/VolatileArrival/Other_BunkerMap"));
    FCoreUObjectDelegates::PostLoadMapWithWorld.Broadcast(World);
    TestEqual(TEXT("Similar map names are not the base"), Game->CountInventoryItemById(6011), 2);
    World->GetOutermost()->Rename(TEXT("/Temp/VolatileArrival/UEDPIE_0_BunkerMap"));
    FCoreUObjectDelegates::PostLoadMapWithWorld.Broadcast(World);
    TestEqual(TEXT("Actual base arrival removes jars"), Game->CountInventoryItemById(6011), 0);
    TestEqual(TEXT("Actual base arrival removes boats"), Game->CountInventoryItemById(6017), 0);
    TestFalse(TEXT("Bag parcel instance removed"), Game->ItemInstancesByUid.Contains(BagParcel));
    TestTrue(TEXT("Bag reference cleared"), Game->AuxiliaryBagSlots[0].IsEmpty());
    TestTrue(TEXT("Quick slot reference cleared"), Game->UsableQuickSlots[0].IsEmpty());
    TestEqual(TEXT("Ordinary carried item retained"), Game->CountInventoryItemById(3004), 1);
    TestTrue(TEXT("Storage retained"), Game->ItemInstancesByUid.Contains(StoredFood));
    FCoreUObjectDelegates::PostLoadMapWithWorld.Broadcast(World);
    TestEqual(TEXT("Repeated arrival is harmless"), Game->CountInventoryItemById(3004), 1);
    return true;
}
#endif
