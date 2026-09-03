#include "Subsystem/TunaSweeperInteractionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Character/TunaSweeperFacilityNpcActor.h"
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperBlockedIntakeScreenActor.h"
#include "Interaction/TunaSweeperDifficultyAdjustmentActor.h"
#include "../../TunaSweeperDebugRifleSupplyActor.h"
#include "Interaction/TunaSweeperDoorActor.h"
#include "Interaction/TunaSweeperHousingManagementActor.h"
#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"
#include "Interaction/TunaSweeperMemoActor.h"
#include "Interaction/TunaSweeperPersistentDoorActor.h"
#include "Interaction/TunaSweeperPiggyBankActor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "Interaction/TunaSweeperResearchStationActor.h"
#include "Interaction/TunaSweeperSelfDestructInteractableActor.h"
#include "Interaction/TunaSweeperShopActor.h"
#include "Interaction/TunaSweeperStorageActor.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Interaction/TunaSweeperWorkbenchActor.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TunaSweeperPlayerController.h"
#include "GameFramework/Actor.h"
#include "Stats/Stats.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperMemoSubsystem.h"

namespace TunaSweeperInteractionQuestEvents
{
	bool IsBunkerMap(const UWorld* World)
	{
		return World && World->GetMapName().EndsWith(TEXT("BunkerMap"));
	}

	bool IsHousingInteractionSuppressed(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UTunaSweeperHousingSubsystem* HousingSubsystem = GameInstance
			? GameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr;
		return HousingSubsystem && HousingSubsystem->IsHousingModeOpen();
	}

	FName ResolveQuestIdForActor(const AActor* Actor)
	{
		if (const ATunaSweeperMoleCompanionActor* MoleActor = Cast<ATunaSweeperMoleCompanionActor>(Actor))
		{
			return MoleActor->ResolveQuestId();
		}

		if (const ATunaSweeperFacilityNpcActor* FacilityNpcActor = Cast<ATunaSweeperFacilityNpcActor>(Actor))
		{
			return FacilityNpcActor->ResolveQuestId();
		}

		return NAME_None;
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
		case ETunaSweeperInteractionType::WorkbenchOpen:
			return FName(TEXT("workbench_open"));
		case ETunaSweeperInteractionType::WorkbenchCraft:
			return FName(TEXT("workbench_craft"));
		case ETunaSweeperInteractionType::WorkbenchDismantle:
			return FName(TEXT("workbench_dismantle"));
		case ETunaSweeperInteractionType::WorkbenchBlueprintRegister:
			return FName(TEXT("workbench_blueprint_register"));
		case ETunaSweeperInteractionType::PiggyBank:
			return FName(TEXT("piggy_bank"));
		case ETunaSweeperInteractionType::PiggyBankDeposit:
			return FName(TEXT("piggy_bank_deposit"));
		case ETunaSweeperInteractionType::PiggyBankWithdraw:
			return FName(TEXT("piggy_bank_withdraw"));
		case ETunaSweeperInteractionType::MoleDialogue:
			return FName(TEXT("mole_dialogue"));
		case ETunaSweeperInteractionType::DifficultyAdjustment:
			return FName(TEXT("difficulty_adjustment"));
		case ETunaSweeperInteractionType::Research:
			return FName(TEXT("research"));
		default:
			return NAME_None;
		}
	}

	void SortInteractablesForDisplay(TArray<UTunaSweeperInteractableComponent*>& Interactables)
	{
		Interactables.Sort([](const UTunaSweeperInteractableComponent& Left, const UTunaSweeperInteractableComponent& Right)
		{
			if (Left.GetInteractionOrder() != Right.GetInteractionOrder())
			{
				return Left.GetInteractionOrder() < Right.GetInteractionOrder();
			}

			return Left.GetFName().LexicalLess(Right.GetFName());
		});
	}
}

void UTunaSweeperInteractionSubsystem::Tick(float DeltaTime)
{
	if (TunaSweeperInteractionQuestEvents::IsHousingInteractionSuppressed(GetWorld()))
	{
		FocusedInteractable.Reset();
		FocusedInteractionOwner.Reset();
		FocusedInteractionIndex = 0;
		return;
	}

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

	if (Interactable && FocusedInteractionOwner.Get() == Interactable->GetOwner())
	{
		FocusedInteractionOwner.Reset();
		FocusedInteractionIndex = 0;
	}
}

UTunaSweeperInteractableComponent* UTunaSweeperInteractionSubsystem::GetFocusedInteractable() const
{
	return FocusedInteractable.Get();
}

bool UTunaSweeperInteractionSubsystem::MoveFocusedInteractionSelection(int32 SelectionDelta, APawn* InstigatorPawn)
{
	if (!InstigatorPawn || SelectionDelta == 0)
	{
		return false;
	}

	RefreshFocusedInteractable();

	AActor* Owner = FocusedInteractionOwner.Get();
	if (!Owner)
	{
		return false;
	}

	TArray<UTunaSweeperInteractableComponent*> OwnerInteractables;
	GatherCandidateInteractablesForOwner(Owner, InstigatorPawn, true, OwnerInteractables);
	if (OwnerInteractables.Num() <= 1)
	{
		return false;
	}

	const int32 CurrentIndex = OwnerInteractables.IsValidIndex(FocusedInteractionIndex)
		? FocusedInteractionIndex
		: FindInteractableIndex(OwnerInteractables, FocusedInteractable.Get());
	const int32 BaseIndex = CurrentIndex == INDEX_NONE ? 0 : CurrentIndex;
	const int32 WrappedIndex = (BaseIndex + SelectionDelta % OwnerInteractables.Num() + OwnerInteractables.Num()) % OwnerInteractables.Num();

	FocusedInteractionIndex = WrappedIndex;
	FocusedInteractable = OwnerInteractables[FocusedInteractionIndex];
	return true;
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

	const ETunaSweeperInteractionType RequestedInteractionType = Interactable->GetInteractionType();
	const FName RequestedObjectiveEventId = Interactable->GetObjectiveEventId();
	bool bHandled = false;
	switch (RequestedInteractionType)
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
	case ETunaSweeperInteractionType::WorkbenchOpen:
		bHandled = HandleWorkbenchOpenInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::WorkbenchCraft:
		bHandled = HandleWorkbenchCraftInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::WorkbenchDismantle:
		bHandled = HandleWorkbenchDismantleInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::WorkbenchBlueprintRegister:
		bHandled = HandleWorkbenchBlueprintRegisterInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::PiggyBank:
		bHandled = HandlePiggyBankInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::PiggyBankDeposit:
		bHandled = HandlePiggyBankDepositInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::PiggyBankWithdraw:
		bHandled = HandlePiggyBankWithdrawInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::MoleDialogue:
		bHandled = HandleMoleDialogueInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::DifficultyAdjustment:
		bHandled = HandleDifficultyAdjustmentInteraction(Interactable, InstigatorPawn);
		break;
	case ETunaSweeperInteractionType::Research:
		bHandled = HandleResearchInteraction(Interactable, InstigatorPawn);
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
					RequestedObjectiveEventId,
					TunaSweeperInteractionQuestEvents::GetInteractionTypeName(RequestedInteractionType));
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

	if (!IsValid(Interactable->GetOwner()) || Interactable->GetOwner()->IsHidden())
	{
		return false;
	}

	if (TunaSweeperInteractionQuestEvents::IsHousingInteractionSuppressed(GetWorld()))
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

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::WorkbenchOpen)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld());
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::WorkbenchCraft ||
		Interactable->GetInteractionType() == ETunaSweeperInteractionType::WorkbenchDismantle ||
		Interactable->GetInteractionType() == ETunaSweeperInteractionType::WorkbenchBlueprintRegister)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld());
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::PiggyBank ||
		Interactable->GetInteractionType() == ETunaSweeperInteractionType::PiggyBankDeposit ||
		Interactable->GetInteractionType() == ETunaSweeperInteractionType::PiggyBankWithdraw)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld());
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::MoleDialogue)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()) &&
			Cast<ATunaSweeperMoleCompanionActor>(Interactable->GetOwner());
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::DifficultyAdjustment)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()) &&
			Cast<ATunaSweeperDifficultyAdjustmentActor>(Interactable->GetOwner());
	}

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::Research)
	{
		return TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()) &&
			Cast<ATunaSweeperResearchStationActor>(Interactable->GetOwner());
	}

	if (Interactable->GetInteractionType() != ETunaSweeperInteractionType::Quest)
	{
		return true;
	}

	return !TunaSweeperInteractionQuestEvents::ResolveQuestIdForActor(Interactable->GetOwner()).IsNone();
}

bool UTunaSweeperInteractionSubsystem::ShouldDisplayMarkerForInteractable(
	const UTunaSweeperInteractableComponent* Interactable) const
{
	if (!IsValid(Interactable) || !CanOfferInteraction(Interactable))
	{
		return false;
	}

	return ResolveMarkerInteractableForOwner(Interactable->GetOwner()) == Interactable;
}

bool UTunaSweeperInteractionSubsystem::IsFocusedInteractionGroupMarker(
	const UTunaSweeperInteractableComponent* Interactable) const
{
	if (!IsValid(Interactable) || !FocusedInteractable.IsValid() || !FocusedInteractionOwner.IsValid())
	{
		return false;
	}

	return FocusedInteractionOwner.Get() == Interactable->GetOwner() &&
		ResolveMarkerInteractableForOwner(FocusedInteractionOwner.Get()) == Interactable;
}

void UTunaSweeperInteractionSubsystem::GetMarkerInteractionOptions(
	const UTunaSweeperInteractableComponent* MarkerInteractable,
	TArray<FText>& OutDisplayNames,
	int32& OutFocusedIndex) const
{
	OutDisplayNames.Reset();
	OutFocusedIndex = INDEX_NONE;

	if (!IsValid(MarkerInteractable) ||
		!FocusedInteractionOwner.IsValid() ||
		MarkerInteractable->GetOwner() != FocusedInteractionOwner.Get() ||
		ResolveMarkerInteractableForOwner(FocusedInteractionOwner.Get()) != MarkerInteractable)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	TArray<UTunaSweeperInteractableComponent*> OwnerInteractables;
	GatherCandidateInteractablesForOwner(FocusedInteractionOwner.Get(), PlayerPawn, true, OwnerInteractables);
	if (OwnerInteractables.Num() <= 1)
	{
		return;
	}

	OutFocusedIndex = FindInteractableIndex(OwnerInteractables, FocusedInteractable.Get());
	if (OutFocusedIndex == INDEX_NONE)
	{
		OutFocusedIndex = FMath::Clamp(FocusedInteractionIndex, 0, OwnerInteractables.Num() - 1);
	}

	for (const UTunaSweeperInteractableComponent* OwnerInteractable : OwnerInteractables)
	{
		OutDisplayNames.Add(OwnerInteractable ? OwnerInteractable->GetInteractionDisplayName() : FText::GetEmpty());
	}
}

void UTunaSweeperInteractionSubsystem::GatherCandidateInteractablesForOwner(
	const AActor* Owner,
	const APawn* PlayerPawn,
	bool bRequireInteractionDistance,
	TArray<UTunaSweeperInteractableComponent*>& OutInteractables) const
{
	OutInteractables.Reset();

	if (!Owner)
	{
		return;
	}

	for (auto InteractableIt = RegisteredInteractables.CreateConstIterator(); InteractableIt; ++InteractableIt)
	{
		UTunaSweeperInteractableComponent* Interactable = InteractableIt->Get();
		if (!IsValid(Interactable) || Interactable->GetOwner() != Owner || !CanOfferInteraction(Interactable))
		{
			continue;
		}

		if (bRequireInteractionDistance && (!PlayerPawn || !Interactable->IsWithinInteractionDistance(PlayerPawn)))
		{
			continue;
		}

		OutInteractables.Add(Interactable);
	}

	TunaSweeperInteractionQuestEvents::SortInteractablesForDisplay(OutInteractables);
}

UTunaSweeperInteractableComponent* UTunaSweeperInteractionSubsystem::ResolveMarkerInteractableForOwner(
	const AActor* Owner) const
{
	TArray<UTunaSweeperInteractableComponent*> OwnerInteractables;
	GatherCandidateInteractablesForOwner(Owner, nullptr, false, OwnerInteractables);
	return OwnerInteractables.Num() > 0 ? OwnerInteractables[0] : nullptr;
}

int32 UTunaSweeperInteractionSubsystem::FindInteractableIndex(
	const TArray<UTunaSweeperInteractableComponent*>& Interactables,
	const UTunaSweeperInteractableComponent* Interactable) const
{
	for (int32 Index = 0; Index < Interactables.Num(); ++Index)
	{
		if (Interactables[Index] == Interactable)
		{
			return Index;
		}
	}

	return INDEX_NONE;
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
	ATunaSweeperDebugRifleSupplyActor* DebugSupplyActor = Interactable
		? Cast<ATunaSweeperDebugRifleSupplyActor>(Interactable->GetOwner())
		: nullptr;
	if (DebugSupplyActor)
	{
		return DebugSupplyActor->SupplyRifleAndAmmo(InstigatorPawn);
	}

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
	AActor* QuestOwner = Interactable ? Interactable->GetOwner() : nullptr;
	if (!QuestOwner || !InstigatorPawn)
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	const FName ResolvedQuestId = TunaSweeperInteractionQuestEvents::ResolveQuestIdForActor(QuestOwner);
	if (ResolvedQuestId.IsNone())
	{
		return false;
	}

	TunaPlayerController->OpenQuestPanel(ResolvedQuestId);
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleMoleDialogueInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperMoleCompanionActor* MoleActor = Interactable
		? Cast<ATunaSweeperMoleCompanionActor>(Interactable->GetOwner())
		: nullptr;
	if (!MoleActor || !InstigatorPawn)
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	return TunaPlayerController && TunaPlayerController->StartScenarioForTrigger(FName(TEXT("interaction.mole")), true);
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
	AActor* ProgressOwner = Interactable ? Interactable->GetOwner() : nullptr;
	if (!ProgressOwner || !InstigatorPawn)
	{
		return false;
	}

	if (ATunaSweeperWorldProgressActor* ProgressActor = Cast<ATunaSweeperWorldProgressActor>(ProgressOwner))
	{
		return ProgressActor->RepairUsingAvailableRequiredItems(true);
	}

	if (ATunaSweeperBlockedIntakeScreenActor* IntakeScreenActor = Cast<ATunaSweeperBlockedIntakeScreenActor>(ProgressOwner))
	{
		return IntakeScreenActor->Interact(true);
	}

	return false;
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

bool UTunaSweeperInteractionSubsystem::HandleWorkbenchOpenInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	return HandleWorkbenchCraftInteraction(Interactable, InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleWorkbenchCraftInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperWorkbenchActor* WorkbenchActor = Interactable
		? Cast<ATunaSweeperWorkbenchActor>(Interactable->GetOwner())
		: nullptr;
	if (!WorkbenchActor || !InstigatorPawn || !TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	TunaPlayerController->OpenWorkbenchCraftPanel(WorkbenchActor->GetWorkbenchId());
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleWorkbenchDismantleInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperWorkbenchActor* WorkbenchActor = Interactable
		? Cast<ATunaSweeperWorkbenchActor>(Interactable->GetOwner())
		: nullptr;
	if (!WorkbenchActor || !InstigatorPawn || !TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	TunaPlayerController->OpenWorkbenchDismantlePanel(WorkbenchActor->GetWorkbenchId());
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleWorkbenchBlueprintRegisterInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperWorkbenchActor* WorkbenchActor = Interactable
		? Cast<ATunaSweeperWorkbenchActor>(Interactable->GetOwner())
		: nullptr;
	if (!WorkbenchActor || !InstigatorPawn || !TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	TunaPlayerController->OpenWorkbenchBlueprintRegisterPanel(WorkbenchActor->GetWorkbenchId());
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandlePiggyBankInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperPiggyBankActor* PiggyBankActor = Interactable
		? Cast<ATunaSweeperPiggyBankActor>(Interactable->GetOwner())
		: nullptr;
	return PiggyBankActor && PiggyBankActor->GrantCurrency(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandlePiggyBankDepositInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperPiggyBankActor* PiggyBankActor = Interactable
		? Cast<ATunaSweeperPiggyBankActor>(Interactable->GetOwner())
		: nullptr;
	return PiggyBankActor && PiggyBankActor->DepositAncientCurrencyItems(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandlePiggyBankWithdrawInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperPiggyBankActor* PiggyBankActor = Interactable
		? Cast<ATunaSweeperPiggyBankActor>(Interactable->GetOwner())
		: nullptr;
	return PiggyBankActor && PiggyBankActor->ShowWithdrawNotImplemented(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleDifficultyAdjustmentInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperDifficultyAdjustmentActor* DifficultyAdjustmentActor = Interactable
		? Cast<ATunaSweeperDifficultyAdjustmentActor>(Interactable->GetOwner())
		: nullptr;
	if (!DifficultyAdjustmentActor || !InstigatorPawn || !TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	return TunaPlayerController && TunaPlayerController->OpenDifficultyAdjustmentPanel();
}

bool UTunaSweeperInteractionSubsystem::HandleResearchInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	const ATunaSweeperResearchStationActor* ResearchStationActor = Interactable
		? Cast<ATunaSweeperResearchStationActor>(Interactable->GetOwner())
		: nullptr;
	if (!ResearchStationActor || !InstigatorPawn || !TunaSweeperInteractionQuestEvents::IsBunkerMap(GetWorld()))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	return TunaPlayerController && TunaPlayerController->OpenResearchPanel();
}

void UTunaSweeperInteractionSubsystem::RefreshFocusedInteractable()
{
	UWorld* World = GetWorld();
	const APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!PlayerPawn)
	{
		FocusedInteractable.Reset();
		FocusedInteractionOwner.Reset();
		FocusedInteractionIndex = 0;
		return;
	}

	AActor* ClosestOwner = nullptr;
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
			ClosestOwner = Interactable->GetOwner();
		}
	}

	if (!ClosestOwner)
	{
		FocusedInteractable.Reset();
		FocusedInteractionOwner.Reset();
		FocusedInteractionIndex = 0;
		return;
	}

	TArray<UTunaSweeperInteractableComponent*> OwnerInteractables;
	GatherCandidateInteractablesForOwner(ClosestOwner, PlayerPawn, true, OwnerInteractables);
	if (OwnerInteractables.Num() == 0)
	{
		FocusedInteractable.Reset();
		FocusedInteractionOwner.Reset();
		FocusedInteractionIndex = 0;
		return;
	}

	if (FocusedInteractionOwner.Get() != ClosestOwner)
	{
		FocusedInteractionOwner = ClosestOwner;
		FocusedInteractionIndex = 0;
	}
	else
	{
		const int32 ExistingFocusedIndex = FindInteractableIndex(OwnerInteractables, FocusedInteractable.Get());
		if (ExistingFocusedIndex != INDEX_NONE)
		{
			FocusedInteractionIndex = ExistingFocusedIndex;
		}
		else
		{
			FocusedInteractionIndex = FMath::Clamp(FocusedInteractionIndex, 0, OwnerInteractables.Num() - 1);
		}
	}

	FocusedInteractionIndex = FMath::Clamp(FocusedInteractionIndex, 0, OwnerInteractables.Num() - 1);
	FocusedInteractable = OwnerInteractables[FocusedInteractionIndex];
}
