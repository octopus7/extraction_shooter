#include "Subsystem/TunaSweeperInteractionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Character/TunaSweeperQuestNpcActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperDoorActor.h"
#include "Interaction/TunaSweeperHousingManagementActor.h"
#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"
#include "Interaction/TunaSweeperMemoActor.h"
#include "Interaction/TunaSweeperPersistentDoorActor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "Interaction/TunaSweeperSelfDestructInteractableActor.h"
#include "Interaction/TunaSweeperShopActor.h"
#include "Interaction/TunaSweeperStorageActor.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Stats/Stats.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperMemoSubsystem.h"

namespace TunaSweeperInteractionQuestEvents
{
	bool IsBunkerMap(const UWorld* World)
	{
		return World && World->GetMapName().EndsWith(TEXT("BunkerMap"));
	}

	FName GetInteractionTypeName(ETunaSweeperInteractionType InteractionType)
	{
		switch (InteractionType)
		{
		case ETunaSweeperInteractionType::ItemPickup:
			return FName(TEXT("item_pickup"));
		case ETunaSweeperInteractionType::ItemSpawn:
			return FName(TEXT("item_spawn"));
		case ETunaSweeperInteractionType::LootContainerOpen:
			return FName(TEXT("loot_container_open"));
		case ETunaSweeperInteractionType::LootContainerSpawn:
			return FName(TEXT("loot_container_spawn"));
		case ETunaSweeperInteractionType::LevelTravel:
			return FName(TEXT("level_travel"));
		case ETunaSweeperInteractionType::Quest:
			return FName(TEXT("quest"));
		case ETunaSweeperInteractionType::SelfDestruct:
			return FName(TEXT("self_destruct"));
		case ETunaSweeperInteractionType::WorldProgress:
			return FName(TEXT("world_progress"));
		case ETunaSweeperInteractionType::PersistentDoor:
			return FName(TEXT("persistent_door"));
		case ETunaSweeperInteractionType::WarpPoint:
			return FName(TEXT("warp_point"));
		case ETunaSweeperInteractionType::Memo:
			return FName(TEXT("memo"));
		case ETunaSweeperInteractionType::DoorOpen:
			return FName(TEXT("door_open"));
		case ETunaSweeperInteractionType::HousingManagement:
			return FName(TEXT("housing_management"));
		case ETunaSweeperInteractionType::StorageOpen:
			return FName(TEXT("storage_open"));
		case ETunaSweeperInteractionType::ShopOpen:
			return FName(TEXT("shop_open"));
		default:
			return NAME_None;
		}
	}
}

void UTunaSweeperInteractionSubsystem::Tick(float DeltaTime)
{
	RefreshFocusedInteractable();
}

TStatId UTunaSweeperInteractionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperInteractionSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperInteractionSubsystem::IsTickable() const
{
	return GetWorld() && GetWorld()->IsGameWorld();
}

void UTunaSweeperInteractionSubsystem::RegisterInteractable(UTunaSweeperInteractableComponent* Interactable)
{
	if (IsValid(Interactable))
	{
		RegisteredInteractables.Add(Interactable);
	}
}

void UTunaSweeperInteractionSubsystem::UnregisterInteractable(UTunaSweeperInteractableComponent* Interactable)
{
	RegisteredInteractables.Remove(Interactable);

	if (FocusedInteractable.Get() == Interactable)
	{
		FocusedInteractable.Reset();
	}
}

UTunaSweeperInteractableComponent* UTunaSweeperInteractionSubsystem::GetFocusedInteractable() const
{
	return FocusedInteractable.Get();
}

bool UTunaSweeperInteractionSubsystem::TryInteract(APawn* InstigatorPawn)
{
	RefreshFocusedInteractable();
	return RequestInteraction(FocusedInteractable.Get(), InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::RequestInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn)
{
	if (!IsValid(Interactable) ||
		!IsValid(InstigatorPawn) ||
		!CanOfferInteraction(Interactable) ||
		!Interactable->IsWithinInteractionDistance(InstigatorPawn))
	{
		return false;
	}

	bool bHandled = false;
	switch (Interactable->GetInteractionType())
	{
	case ETunaSweeperInteractionType::ItemPickup:
		bHandled = HandlePickupItemInteraction(Interactable);
		break;
	case ETunaSweeperInteractionType::ItemSpawn:
		bHandled = HandleItemSpawnInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::LootContainerOpen:
		bHandled = HandleLootContainerOpenInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::LootContainerSpawn:
		bHandled = HandleLootContainerSpawnInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::LevelTravel:
		bHandled = HandleLevelTravelInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::Quest:
		bHandled = HandleQuestInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::SelfDestruct:
		bHandled = HandleSelfDestructInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::WorldProgress:
		bHandled = HandleWorldProgressInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::PersistentDoor:
		bHandled = HandlePersistentDoorInteraction(Interactable);
		break;
	case ETunaSweeperInteractionType::WarpPoint:
		bHandled = HandleWarpPointInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::Memo:
		bHandled = HandleMemoInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::DoorOpen:
		bHandled = HandleDoorOpenInteraction(Interactable);
		break;
	case ETunaSweeperInteractionType::HousingManagement:
		bHandled = HandleHousingManagementInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::StorageOpen:
		bHandled = HandleStorageOpenInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::ShopOpen:
		bHandled = HandleShopOpenInteraction(Interactable, InstigatorPawn);
		break;
	default:
		return false;
	}

	if (bHandled)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->NotifyInteractionCompleted(
					Interactable->GetObjectiveEventId(),
					TunaSweeperInteractionQuestEvents::GetInteractionTypeName(Interactable->GetInteractionType()));
			}
		}
	}

	return bHandled;
}

bool UTunaSweeperInteractionSubsystem::CanOfferInteraction(const UTunaSweeperInteractableComponent* Interactable) const
{
	if (!IsValid(Interactable) || Interactable->GetInteractionType() == ETunaSweeperInteractionType::None)
	{
		return false;
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::Memo)
	{
		const ATunaSweeperMemoActor* MemoActor = Cast<ATunaSweeperMemoActor>(Interactable->GetOwner());
		UTunaSweeperGameInstance* TunaGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
		return MemoActor &&
			TunaGameInstance &&
			MemoActor->GetMemoId() > 0 &&
			!TunaGameInstance->IsMemoAcquired(MemoActor->GetMemoId());
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::StorageOpen)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld());
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::ShopOpen)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld());
	}

	if (Interactable->GetInteractionType() != ETunaSweeperInteractionType::Quest)
	{
		return true;
	}

	const ATunaSweeperQuestNpcActor* QuestNpcActor = Cast<ATunaSweeperQuestNpcActor>(Interactable->GetOwner());
	return QuestNpcActor && !QuestNpcActor->ResolveQuestId().IsNone();
}

bool UTunaSweeperInteractionSubsystem::HandlePickupItemInteraction(UTunaSweeperInteractableComponent* Interactable)
{
	ATunaSweeperPickupItemActor* PickupItemActor = Interactable
		? Cast<ATunaSweeperPickupItemActor>(Interactable->GetOwner())
		: nullptr;
	if (!PickupItemActor)
	{
		return false;
	}

	const FString ItemName = PickupItemActor->GetItemDisplayName().ToString();
	UTunaSweeperGameInstance* TunaGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	if (!TunaGameInstance || !TunaGameInstance->AddItemToPreferredAvailableSlot(PickupItemActor->GetItemId(), PickupItemActor->GetQuantity()))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Red,
				FString::Printf(TEXT("[Interaction] Inventory full: %s"), *ItemName));
		}
		return false;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Green,
			FString::Printf(TEXT("[Interaction] Acquired: %s"), *ItemName));
	}

	if (PickupItemActor->ShouldDestroyOnPickup())
	{
		PickupItemActor->Destroy();
	}

	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleItemSpawnInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperItemSpawnInteractableActor* ItemSpawnActor = Interactable
		? Cast<ATunaSweeperItemSpawnInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return ItemSpawnActor && ItemSpawnActor->SpawnRandomPickupItem(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleLootContainerOpenInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperLootContainerActor* LootContainerActor = Interactable
		? Cast<ATunaSweeperLootContainerActor>(Interactable->GetOwner())
		: nullptr;
	if (!LootContainerActor || !InstigatorPawn)
	{
		return false;
	}

	FTunaSweeperLootContainerInstance ContainerInstance;
	UTunaSweeperGameInstance* TunaGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	if (!LootContainerActor->OpenRuntimeContainer(TunaGameInstance, ContainerInstance))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	LootContainerActor->PlayOpenAnimation();
	TunaPlayerController->OpenLootContainerPanel(ContainerInstance);
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleLootContainerSpawnInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperLootContainerSpawnInteractableActor* SpawnActor = Interactable
		? Cast<ATunaSweeperLootContainerSpawnInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return SpawnActor && SpawnActor->SpawnRandomLootContainer(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleLevelTravelInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperLevelTravelInteractableActor* LevelTravelActor = Interactable
		? Cast<ATunaSweeperLevelTravelInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return LevelTravelActor && LevelTravelActor->TravelToTargetLevel(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleQuestInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperQuestNpcActor* QuestNpcActor = Interactable
		? Cast<ATunaSweeperQuestNpcActor>(Interactable->GetOwner())
		: nullptr;
	if (!QuestNpcActor || !InstigatorPawn)
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	const FName ResolvedQuestId = QuestNpcActor->ResolveQuestId();
	if (ResolvedQuestId.IsNone())
	{
		return false;
	}

	TunaPlayerController->OpenQuestPanel(ResolvedQuestId);
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleSelfDestructInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperSelfDestructInteractableActor* SelfDestructActor = Interactable
		? Cast<ATunaSweeperSelfDestructInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return SelfDestructActor && SelfDestructActor->StartSelfDestruct(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleWorldProgressInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperWorldProgressActor* ProgressActor = Interactable
		? Cast<ATunaSweeperWorldProgressActor>(Interactable->GetOwner())
		: nullptr;
	if (!ProgressActor || !InstigatorPawn)
	{
		return false;
	}

	return ProgressActor->RepairUsingAvailableRequiredItems(true);
}

bool UTunaSweeperInteractionSubsystem::HandlePersistentDoorInteraction(UTunaSweeperInteractableComponent* Interactable)
{
	ATunaSweeperPersistentDoorActor* DoorActor = Interactable
		? Cast<ATunaSweeperPersistentDoorActor>(Interactable->GetOwner())
		: nullptr;
	return DoorActor && DoorActor->OpenDoor(true);
}

bool UTunaSweeperInteractionSubsystem::HandleDoorOpenInteraction(UTunaSweeperInteractableComponent* Interactable)
{
	ATunaSweeperDoorActor* DoorActor = Interactable
		? Cast<ATunaSweeperDoorActor>(Interactable->GetOwner())
		: nullptr;
	return DoorActor && DoorActor->ToggleDoor();
}

bool UTunaSweeperInteractionSubsystem::HandleWarpPointInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperWarpPointActor* WarpPointActor = Interactable
		? Cast<ATunaSweeperWarpPointActor>(Interactable->GetOwner())
		: nullptr;
	return WarpPointActor && WarpPointActor->WarpInstigator(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleMemoInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperMemoActor* MemoActor = Interactable
		? Cast<ATunaSweeperMemoActor>(Interactable->GetOwner())
		: nullptr;
	if (!MemoActor || !InstigatorPawn)
	{
		return false;
	}

	const int32 MemoId = MemoActor->GetMemoId();
	UTunaSweeperGameInstance* TunaGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	if (!TunaGameInstance || TunaGameInstance->IsMemoAcquired(MemoId))
	{
		return false;
	}

	FTunaSweeperMemoDefinition MemoDefinition;
	UTunaSweeperMemoSubsystem* MemoSubsystem = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UTunaSweeperMemoSubsystem>()
		: nullptr;
	if (!MemoSubsystem || !MemoSubsystem->TryGetMemoDefinition(MemoId, MemoDefinition))
	{
		return false;
	}

	TunaGameInstance->MarkMemoAcquired(MemoId, false);

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController()))
	{
		TunaPlayerController->OpenMemoPanel(MemoId);
	}

	MemoActor->Destroy();
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleHousingManagementInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperHousingManagementActor* ManagementActor = Interactable
		? Cast<ATunaSweeperHousingManagementActor>(Interactable->GetOwner())
		: nullptr;
	if (!ManagementActor || !InstigatorPawn)
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	return TunaPlayerController && TunaPlayerController->OpenHousingMode();
}

bool UTunaSweeperInteractionSubsystem::HandleStorageOpenInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperStorageActor* StorageActor = Interactable
		? Cast<ATunaSweeperStorageActor>(Interactable->GetOwner())
		: nullptr;
	if (!StorageActor || !InstigatorPawn || !TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	TunaPlayerController->OpenStoragePanel();
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleShopOpenInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperShopActor* ShopActor = Interactable
		? Cast<ATunaSweeperShopActor>(Interactable->GetOwner())
		: nullptr;
	if (!ShopActor || !InstigatorPawn || !TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	TunaPlayerController->OpenShopPanel(ShopActor->GetShopId());
	return true;
}

void UTunaSweeperInteractionSubsystem::RefreshFocusedInteractable()
{
	UWorld* World = GetWorld();
	const APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!PlayerPawn)
	{
		FocusedInteractable.Reset();
		return;
	}

	UTunaSweeperInteractableComponent* ClosestInteractable = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (auto InteractableIt = RegisteredInteractables.CreateIterator(); InteractableIt; ++InteractableIt)
	{
		UTunaSweeperInteractableComponent* Interactable = InteractableIt->Get();
		if (!IsValid(Interactable))
		{
			InteractableIt.RemoveCurrent();
			continue;
		}

		if (!CanOfferInteraction(Interactable) || !Interactable->IsWithinInteractionDistance(PlayerPawn))
		{
			continue;
		}

		const float DistanceSquared = Interactable->GetSquaredDistance2DTo(PlayerPawn);
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestInteractable = Interactable;
		}
	}

	FocusedInteractable = ClosestInteractable;
}
