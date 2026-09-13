#include "Interaction/TunaSweeperFoodWarehouseActor.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"

ATunaSweeperFoodWarehouseActor::ATunaSweeperFoodWarehouseActor()
{
    WarehouseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarehouseMesh"));
    RootComponent = WarehouseMesh;
    WarehouseMesh->SetCollisionProfileName(TEXT("BlockAll"));
    Interactable = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
    Interactable->SetupAttachment(RootComponent);
    Interactable->SetRelativeLocation(FVector(0, 0, 130));
    Interactable->SetInteractionTypeDisplayNameAndStringKey(
        ETunaSweeperInteractionType::WorldProgress, NSLOCTEXT("DemoEnding", "TakeFood", "참치캔 가져가기"), NAME_None);
}
bool ATunaSweeperFoodWarehouseActor::TakeFood()
{
    auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
    if (bCollected || !GI || FoodQuantity < 1 || !GI->AddItemToFirstAvailableInventorySlot(FoodItemId, FoodQuantity))
        return false;
    bCollected = true;
    Interactable->SetInteractionTypeDisplayNameAndStringKey(ETunaSweeperInteractionType::None, FText::GetEmpty(), NAME_None);
    // Restock on a new raid so dying or eating the quest food cannot deadlock the demo.
    GI->SaveGameState();
    return true;
}
