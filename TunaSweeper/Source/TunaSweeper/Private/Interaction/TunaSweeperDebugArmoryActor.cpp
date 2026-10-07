#include "Interaction/TunaSweeperDebugArmoryActor.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"

ATunaSweeperDebugArmoryActor::ATunaSweeperDebugArmoryActor()
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::DebugArmoryOpen,
		FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		FName(TEXT("ui.interaction.debug_armory_open")));
	VisualMesh->SetRelativeScale3D(FVector::OneVector);
	VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));
}

void ATunaSweeperDebugArmoryActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
#if UE_BUILD_SHIPPING
	// Blueprint defaults cannot re-enable a cooked debug interaction. Destroying
	// the component also removes its marker before it can register at BeginPlay.
	if (InteractableComponent)
	{
		InteractableComponent->DestroyComponent();
		InteractableComponent = nullptr;
	}
#endif
}

bool ATunaSweeperDebugArmoryActor::IsArmoryEnabled()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

bool ATunaSweeperDebugArmoryActor::IsAllowedItem(const FTunaSweeperItemDefinition& Item)
{
	static const TSet<FName> AllowedCategories = {
		TEXT("item.category.weapon.gun"), TEXT("item.category.weapon.melee"),
		TEXT("item.category.attachment"), TEXT("item.category.ammo"),
		TEXT("item.category.head"), TEXT("item.category.body"),
		TEXT("item.category.face"), TEXT("item.category.ear")
	};
	return Item.Id > 0 && AllowedCategories.Contains(Item.CategoryTag);
}

bool ATunaSweeperDebugArmoryActor::GetSupplyCatalog(TArray<FTunaSweeperItemDefinition>& OutItems) const
{
	OutItems.Reset();
#if UE_BUILD_SHIPPING
	return false;
#else
	const auto* Game = GetWorld() ? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	auto* Data = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
	if (!Data || !Data->GetAllItemDefinitions(OutItems)) return false;
	OutItems.RemoveAll([](const FTunaSweeperItemDefinition& Item) { return !IsAllowedItem(Item); });
	OutItems.Sort([](const FTunaSweeperItemDefinition& A, const FTunaSweeperItemDefinition& B) { return A.Id < B.Id; });
	return !OutItems.IsEmpty();
#endif
}

bool ATunaSweeperDebugArmoryActor::CanUseArmory(APawn* InstigatorPawn) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const auto* Character = Cast<ATunaSweeperTopDownCharacter>(InstigatorPawn);
	const auto* Controller = Character ? Cast<ATunaSweeperPlayerController>(Character->GetController()) : nullptr;
	return IsValid(this) && !IsHidden() && GetWorld() && GetWorld()->IsGameWorld() &&
		IsValid(Character) && Character->GetWorld() == GetWorld() &&
		!Character->IsDead() && !Character->IsMountedInVehicle() &&
		IsValid(Controller) && Controller->IsLocalController() && Controller->GetPawn() == Character &&
		IsValid(InteractableComponent) && GetInteractionType() == ETunaSweeperInteractionType::DebugArmoryOpen &&
		IsWithinInteractionDistance(Character);
#endif
}

bool ATunaSweeperDebugArmoryActor::TrySupplyItem(APawn* InstigatorPawn, int32 ItemId, int32 Quantity)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (!CanUseArmory(InstigatorPawn) || Quantity < 1 || Quantity > 999) return false;
	auto* Game = GetWorld()->GetGameInstance<UTunaSweeperGameInstance>();
	auto* Data = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
	FTunaSweeperItemDefinition Item;
	if (!Data || !Data->TryGetItemDefinition(ItemId, Item) || !IsAllowedItem(Item)) return false;
	// This existing transaction restores all stacks/slots on insufficient space
	// and uses the normal inventory/save path. No price or shop stock is involved.
	return Game->AddItemToFirstAvailableInventorySlot(ItemId, Quantity);
#endif
}
