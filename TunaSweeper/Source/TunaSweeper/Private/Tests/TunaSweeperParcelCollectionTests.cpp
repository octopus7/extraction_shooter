#if WITH_DEV_AUTOMATION_TESTS
#include "Interaction/TunaSweeperParcelContainerActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperParcelCollectionTest,
    "TunaSweeper.Loot.ParcelCollection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperParcelCollectionTest::RunTest(const FString& Parameters)
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Game, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context);
    Context.OwningGameInstance = Game;
    World->SetGameInstance(Game);
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game, nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    auto* Parcel = World->SpawnActor<ATunaSweeperParcelContainerActor>();
    if (!TestNotNull(TEXT("Parcel actor"), Parcel)) return false;
    FTunaSweeperLootContainerInstance Instance;
    TestTrue(TEXT("Open toad parcel"), Parcel->OpenRuntimeContainer(Game, Instance));
    TestTrue(TEXT("Collect on opening"), Parcel->CollectContentsOnOpen(Game));
    TestEqual(TEXT("Exactly one jar received"), Game->CountInventoryItemById(6011), 1);
    TestTrue(TEXT("Reopen same parcel"), Parcel->OpenRuntimeContainer(Game, Instance));
    TestTrue(TEXT("Empty parcel handled without loot panel"), Parcel->CollectContentsOnOpen(Game));
    TestEqual(TEXT("No duplicate on reopen"), Game->CountInventoryItemById(6011), 1);

    auto* Boat = World->SpawnActor<ATunaSweeperParcelContainerActor>();
    Boat->SetContainerDataIds(7004, 8015);
    while (Game->AddItemToFirstAvailableInventorySlot(6011, 1)) {}
    TestTrue(TEXT("Open boat with full inventory"), Boat->OpenRuntimeContainer(Game, Instance));
    TestFalse(TEXT("Full inventory leaves parcel for normal loot UI"), Boat->CollectContentsOnOpen(Game));
    TestEqual(TEXT("No boat granted without room"), Game->CountInventoryItemById(6017), 0);
    TestEqual(TEXT("Free one slot"), Game->ConsumeInventoryItemById(6011, 1), 1);
    TestTrue(TEXT("Reopen after freeing space"), Boat->OpenRuntimeContainer(Game, Instance));
    TestTrue(TEXT("Remaining boat can be collected"), Boat->CollectContentsOnOpen(Game));
    TestEqual(TEXT("Boat preserved on failed collection"), Game->CountInventoryItemById(6017), 1);
    auto* Ordinary = World->SpawnActor<ATunaSweeperLootContainerActor>();
    TestFalse(TEXT("Ordinary containers keep manual looting"), Ordinary->CollectContentsOnOpen(Game));
    TestFalse(TEXT("Cannot collect another container's active contents"), Parcel->CollectContentsOnOpen(Game));
    auto* ParcelClass = LoadClass<ATunaSweeperParcelContainerActor>(nullptr,
        TEXT("/Game/Interaction/Parcels/BP_Parcel_ToadJar.BP_Parcel_ToadJar_C"));
    if (!TestNotNull(TEXT("Authored parcel BP loads"), ParcelClass)) return false;
    auto* AuthoredParcel = World->SpawnActor<ATunaSweeperParcelContainerActor>(ParcelClass);
    auto* Replacement = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!TestNotNull(TEXT("Replacement mesh"), Replacement)) return false;
    AuthoredParcel->GetBodyMeshComponent()->SetStaticMesh(Replacement);
    AuthoredParcel->SetContainerDataIds(7004, 8014);
    TestTrue(TEXT("BP component mesh replacement survives data refresh"),
        AuthoredParcel->GetBodyMeshComponent()->GetStaticMesh() == Replacement);
    return true;
}
#endif
