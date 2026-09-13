#include "Interaction/TunaSweeperFoodWarehouseActor.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperFoodWarehouseActor::ATunaSweeperFoodWarehouseActor()
{
    WarehouseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarehouseMesh"));
    RootComponent = WarehouseMesh;
    WarehouseMesh->SetCollisionProfileName(TEXT("BlockAll"));
    Interactable = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
    Interactable->SetupAttachment(RootComponent);
    Interactable->SetRelativeLocation(FVector(0, 0, 130));
    RefreshInteractionState();
}

void ATunaSweeperFoodWarehouseActor::BeginPlay()
{
    Super::BeginPlay();

    if (UTunaSweeperGameInstance* GameInstance = GetGameInstance<UTunaSweeperGameInstance>())
    {
        if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
        {
            QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
            QuestSubsystem->OnQuestProgressChanged.AddUObject(this, &ATunaSweeperFoodWarehouseActor::RefreshInteractionState);
        }
    }

    RefreshInteractionState();
}

void ATunaSweeperFoodWarehouseActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UTunaSweeperGameInstance* GameInstance = GetGameInstance<UTunaSweeperGameInstance>())
    {
        if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
        {
            QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperFoodWarehouseActor::CanTakeFood() const
{
    UTunaSweeperGameInstance* GameInstance = GetGameInstance<UTunaSweeperGameInstance>();
    const UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance
        ? GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
        : nullptr;
    const bool bRequiredQuestActive = RequiredQuestId.IsNone() ||
        (QuestSubsystem && QuestSubsystem->GetQuestState(RequiredQuestId) == ETunaSweeperQuestState::Accepted);
    return !bCollected && FoodQuantity > 0 && bRequiredQuestActive;
}

bool ATunaSweeperFoodWarehouseActor::TakeFood()
{
    auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
    if (!CanTakeFood() || !GI || !GI->AddItemToFirstAvailableInventorySlot(FoodItemId, FoodQuantity))
        return false;
    bCollected = true;
    RefreshInteractionState();
    // Restock on a new raid so dying or eating the quest food cannot deadlock the demo.
    GI->SaveGameState();
    return true;
}

void ATunaSweeperFoodWarehouseActor::RefreshInteractionState()
{
    if (!Interactable)
    {
        return;
    }

    const bool bCanTakeFood = CanTakeFood();
    Interactable->SetInteractionTypeDisplayNameAndStringKey(
        bCanTakeFood ? ETunaSweeperInteractionType::WorldProgress : ETunaSweeperInteractionType::None,
        bCanTakeFood ? InteractionDisplayName : FText::GetEmpty(),
        bCanTakeFood ? InteractionDisplayNameStringKey : NAME_None);
}
