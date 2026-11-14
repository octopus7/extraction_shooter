#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperFoodWarehouseActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaFoodWarehouseAssetsTest, "TunaSweeper.FoodWarehouse.AssetsAndInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaFoodWarehouseAssetsTest::RunTest(const FString&)
{
    auto* WarehouseClass=LoadClass<ATunaSweeperFoodWarehouseActor>(nullptr,TEXT("/Game/Interaction/DemoEnding/BP_FoodWarehouse.BP_FoodWarehouse_C"));
    if (!TestNotNull(TEXT("Warehouse BP"),WarehouseClass)) return false;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* GI=NewObject<UTunaSweeperGameInstance>(World); World->SetGameInstance(GI);
    // No item-data subsystem is initialized in this isolated fixture; lab weapon setup is irrelevant.
    AddExpectedError(TEXT("Boss lab could not initialize rifle and ammunition"),EAutomationExpectedErrorFlags::Contains,1);
    GI->BeginCombatTestSession(); // Disable disk saves in this isolated gameplay test.
    auto* Warehouse=World->SpawnActor<ATunaSweeperFoodWarehouseActor>(WarehouseClass);
    TestNotNull(TEXT("Replaceable static mesh"),Warehouse->WarehouseMesh->GetStaticMesh().Get());
    TestEqual(TEXT("Retired final quest has no numeric replacement"),Warehouse->RequiredQuestId,0);
    TestTrue(TEXT("Unconfigured quest gate remains required"),Warehouse->bRequireQuest);
    TestEqual(TEXT("Interaction hidden before final quest"),Warehouse->Interactable->GetInteractionType(),ETunaSweeperInteractionType::None);
    int32 Before=GI->CountInventoryItemById(3004);
    TestFalse(TEXT("Cannot take a can before final quest"),Warehouse->TakeFood());
    TestEqual(TEXT("No can granted before final quest"),GI->CountInventoryItemById(3004),Before);

    // This isolated world has no game-instance subsystems. Clearing the configurable requirement
    // exercises the active presentation and collection path without touching the user's save data.
    Warehouse->RequiredQuestId=0;
    Warehouse->bRequireQuest=false;
    Warehouse->DispatchBeginPlay();
    TestEqual(TEXT("Interaction connected"),Warehouse->Interactable->GetInteractionType(),ETunaSweeperInteractionType::WorldProgress);
    TestEqual(TEXT("Interaction says take tuna can"),Warehouse->Interactable->GetInteractionDisplayName().ToString(),FString(TEXT("참치캔 획득")));
    TestTrue(TEXT("Take a can"),Warehouse->TakeFood());
    TestEqual(TEXT("Exactly one can"),GI->CountInventoryItemById(3004),Before+1);
    TestFalse(TEXT("Repeated interaction cannot duplicate"),Warehouse->TakeFood());
    TestEqual(TEXT("Collected disables interaction"),Warehouse->Interactable->GetInteractionType(),ETunaSweeperInteractionType::None);
    Warehouse->WarehouseMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Warehouse->RerunConstructionScripts();
    TestEqual(TEXT("Replacement mesh survives construction"),Warehouse->WarehouseMesh->GetStaticMesh()->GetName(),FString(TEXT("Cube")));
    World->DestroyWorld(false);

    return true;
}
#endif
