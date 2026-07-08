#include "TunaSweeperGameInstanceShared.h"

const TArray<FTunaSweeperItemStack>& UTunaSweeperGameInstance::GetOrCreatePlayerInventoryItems()
{
	EnsureInventoryStateInitialized();
	if (!bHasGeneratedPlayerInventoryItems)
	{
		RefreshLegacyPlayerInventoryItems();
	}

	return PlayerInventoryItems;
}

void UTunaSweeperGameInstance::GetPlayerInventoryItems(TArray<FTunaSweeperItemStack>& OutItems)
{
	OutItems = GetOrCreatePlayerInventoryItems();
}

int32 UTunaSweeperGameInstance::GetCurrentInventorySlotCapacity()
{
	EnsureInventoryStateInitialized();
	return CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots);
}

int32 UTunaSweeperGameInstance::GetEquippedBackpackSlotBonus()
{
	EnsureInventoryStateInitialized();
	return FMath::Max(0, GetCurrentInventorySlotCapacity() - FMath::Max(TunaSweeperInventory::RequiredBareInventorySlots, GameplaySettings.BareInventorySlots));
}

int32 UTunaSweeperGameInstance::GetEquippedDefenseValue()
{
	EnsureInventoryStateInitialized();

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem)
	{
		return 0;
	}

	int32 DefenseValue = 0;
	for (const FTunaSweeperInventorySlot& EquipmentSlot : EquipmentSlots)
	{
		FTunaSweeperItemInstance ItemInstance;
		if (!TryGetItemInstance(EquipmentSlot.ItemUid, ItemInstance))
		{
			continue;
		}

		FTunaSweeperItemDefinition ItemDefinition;
		if (ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			DefenseValue += FMath::Max(0, ItemDefinition.DefenseValue);
		}
	}

	return DefenseValue;
}

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetInventorySlots()
{
	EnsureInventoryStateInitialized();
	return PlayerInventorySlots;
}

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetEquipmentSlots()
{
	EnsureInventoryStateInitialized();
	return EquipmentSlots;
}

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetAuxiliaryBagSlots()
{
	EnsureInventoryStateInitialized();
	return AuxiliaryBagSlots;
}

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetUsableQuickSlots()
{
	EnsureInventoryStateInitialized();
	return UsableQuickSlots;
}

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetStorageSlots()
{
	EnsureInventoryStateInitialized();
	return StorageSlots;
}

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetActiveLootContainerSlots()
{
	EnsureInventoryStateInitialized();
	return ActiveLootContainerSlots;
}

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetSelectedWeaponAttachmentSlots()
{
	EnsureInventoryStateInitialized();
	RefreshSelectedWeaponAttachmentSlots();
	return SelectedWeaponAttachmentSlots;
}

bool UTunaSweeperGameInstance::TryGetItemInstance(const FGuid& ItemUid, FTunaSweeperItemInstance& OutItemInstance) const
{
	if (const FTunaSweeperItemInstance* FoundItemInstance = ItemInstancesByUid.Find(ItemUid))
	{
		OutItemInstance = *FoundItemInstance;
		return FoundItemInstance->IsValid();
	}

	OutItemInstance = FTunaSweeperItemInstance();
	return false;
}

bool UTunaSweeperGameInstance::TryGetSlotItemInstance(
	const FTunaSweeperItemSlotReference& SlotReference,
	FTunaSweeperItemInstance& OutItemInstance)
{
	EnsureInventoryStateInitialized();
	const TArray<FTunaSweeperInventorySlot>* Slots = GetSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		OutItemInstance = FTunaSweeperItemInstance();
		return false;
	}

	const FGuid& ItemUid = (*Slots)[SlotReference.SlotIndex].ItemUid;
	return TryGetItemInstance(ItemUid, OutItemInstance);
}

bool UTunaSweeperGameInstance::TryGetSlotItemUid(
	const FTunaSweeperItemSlotReference& SlotReference,
	FGuid& OutItemUid)
{
	EnsureInventoryStateInitialized();
	const TArray<FTunaSweeperInventorySlot>* Slots = GetSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		OutItemUid.Invalidate();
		return false;
	}

	OutItemUid = (*Slots)[SlotReference.SlotIndex].ItemUid;
	return OutItemUid.IsValid() && ItemInstancesByUid.Contains(OutItemUid);
}

bool UTunaSweeperGameInstance::TryGetSelectedItemInstance(FTunaSweeperItemInstance& OutItemInstance)
{
	EnsureInventoryStateInitialized();
	return TryGetSlotItemInstance(SelectedItemSlotReference, OutItemInstance);
}

bool UTunaSweeperGameInstance::TryGetSelectedItemDefinition(FTunaSweeperItemDefinition& OutItemDefinition)
{
	FTunaSweeperItemInstance SelectedItemInstance;
	if (!TryGetSelectedItemInstance(SelectedItemInstance))
	{
		OutItemDefinition = FTunaSweeperItemDefinition();
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	return ItemDataSubsystem && ItemDataSubsystem->TryGetItemDefinition(SelectedItemInstance.ItemId, OutItemDefinition);
}

bool UTunaSweeperGameInstance::CanSlotAcceptItem(const FTunaSweeperItemSlotReference& SlotReference, const FGuid& ItemUid)
{
	EnsureInventoryStateInitialized();
	if (!ItemUid.IsValid())
	{
		return true;
	}

	const TArray<FTunaSweeperInventorySlot>* Slots = GetSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		return false;
	}

	if (SlotReference.Source == ETunaSweeperItemSlotSource::Equipment)
	{
		return IsItemCompatibleWithEquipmentSlot(SlotReference.SlotIndex, ItemUid);
	}

	if (SlotReference.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
	{
		return IsItemCompatibleWithSelectedWeaponAttachmentSlot(SlotReference.SlotIndex, ItemUid);
	}

	if (SlotReference.Source == ETunaSweeperItemSlotSource::UsableQuickSlot)
	{
		return IsItemCompatibleWithUsableQuickSlot(ItemUid);
	}

	return true;
}

bool UTunaSweeperGameInstance::CanUseItemInSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	APawn* InstigatorPawn)
{
	EnsureInventoryStateInitialized();
	if (!SlotReference.IsValid() || !InstigatorPawn)
	{
		return false;
	}

	const TArray<FTunaSweeperInventorySlot>* Slots = GetSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		return false;
	}

	const FGuid ItemUid = (*Slots)[SlotReference.SlotIndex].ItemUid;
	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance || !ItemInstance->IsValid())
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition) ||
		ItemDefinition.CategoryTag != TunaSweeperInventory::ConsumableCategoryTag ||
		!DoesItemDefinitionHaveUseEffect(ItemDefinition))
	{
		return false;
	}

	const bool bHasVitalsEffect =
		!FMath::IsNearlyZero(ItemDefinition.UseHealthDelta) ||
		!FMath::IsNearlyZero(ItemDefinition.UseFoodDelta) ||
		!FMath::IsNearlyZero(ItemDefinition.UseHydrationDelta);
	const bool bClearsDebuffs = ItemDefinition.ClearsDebuffIds.Num() > 0;

	const UTunaSweeperVitalsComponent* VitalsComponent = bHasVitalsEffect
		? InstigatorPawn->FindComponentByClass<UTunaSweeperVitalsComponent>()
		: nullptr;
	const UTunaSweeperDebuffComponent* DebuffComponent = bClearsDebuffs
		? InstigatorPawn->FindComponentByClass<UTunaSweeperDebuffComponent>()
		: nullptr;
	return (!bHasVitalsEffect || VitalsComponent) && (!bClearsDebuffs || DebuffComponent);
}

float UTunaSweeperGameInstance::GetItemUseSecondsInSlot(const FTunaSweeperItemSlotReference& SlotReference)
{
	EnsureInventoryStateInitialized();
	const TArray<FTunaSweeperInventorySlot>* Slots = GetSlotsForSource(SlotReference.Source);
	if (!SlotReference.IsValid() || !Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		return 0.0f;
	}

	const FGuid ItemUid = (*Slots)[SlotReference.SlotIndex].ItemUid;
	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance || !ItemInstance->IsValid())
	{
		return 0.0f;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition) ||
		ItemDefinition.CategoryTag != TunaSweeperInventory::ConsumableCategoryTag ||
		!DoesItemDefinitionHaveUseEffect(ItemDefinition))
	{
		return 0.0f;
	}

	return ItemDefinition.UseSeconds > 0.0f
		? ItemDefinition.UseSeconds
		: TunaSweeperInventory::DefaultItemUseSeconds;
}

bool UTunaSweeperGameInstance::TryUseItemInSlot(const FTunaSweeperItemSlotReference& SlotReference, APawn* InstigatorPawn)
{
	EnsureInventoryStateInitialized();
	if (!SlotReference.IsValid() || !InstigatorPawn)
	{
		return false;
	}

	TArray<FTunaSweeperInventorySlot>* Slots = GetMutableSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		return false;
	}

	const FGuid ItemUid = (*Slots)[SlotReference.SlotIndex].ItemUid;
	FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance || !ItemInstance->IsValid())
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition) ||
		ItemDefinition.CategoryTag != TunaSweeperInventory::ConsumableCategoryTag ||
		!DoesItemDefinitionHaveUseEffect(ItemDefinition))
	{
		return false;
	}

	const bool bHasVitalsEffect =
		!FMath::IsNearlyZero(ItemDefinition.UseHealthDelta) ||
		!FMath::IsNearlyZero(ItemDefinition.UseFoodDelta) ||
		!FMath::IsNearlyZero(ItemDefinition.UseHydrationDelta);
	const bool bClearsDebuffs = ItemDefinition.ClearsDebuffIds.Num() > 0;

	UTunaSweeperVitalsComponent* VitalsComponent = bHasVitalsEffect
		? InstigatorPawn->FindComponentByClass<UTunaSweeperVitalsComponent>()
		: nullptr;
	UTunaSweeperDebuffComponent* DebuffComponent = bClearsDebuffs
		? InstigatorPawn->FindComponentByClass<UTunaSweeperDebuffComponent>()
		: nullptr;
	if ((bHasVitalsEffect && !VitalsComponent) || (bClearsDebuffs && !DebuffComponent))
	{
		return false;
	}

	if (VitalsComponent)
	{
		FTunaSweeperVitalsDelta Effect;
		Effect.Health = ItemDefinition.UseHealthDelta;
		Effect.Food = ItemDefinition.UseFoodDelta;
		Effect.Hydration = ItemDefinition.UseHydrationDelta;
		VitalsComponent->ApplyConsumableVitalsEffect(Effect);
	}
	if (DebuffComponent)
	{
		DebuffComponent->RemoveDebuffs(ItemDefinition.ClearsDebuffIds);
	}

	ItemInstance->Quantity -= 1;
	if (ItemInstance->Quantity <= 0)
	{
		ItemInstancesByUid.Remove(ItemUid);
		(*Slots)[SlotReference.SlotIndex].Clear();
		RemoveInvalidSlotReferences(PlayerInventorySlots);
		RemoveInvalidSlotReferences(EquipmentSlots);
		RemoveInvalidSlotReferences(AuxiliaryBagSlots);
		RemoveInvalidSlotReferences(UsableQuickSlots);
		RemoveInvalidSlotReferences(StorageSlots);
		RemoveInvalidSlotReferences(ActiveLootContainerSlots);
	}

	ClearSelectedItemIfInvalid();
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::TryUseHoveredItem(APawn* InstigatorPawn)
{
	EnsureInventoryStateInitialized();
	if (!HoveredItemSlotReference.IsValid())
	{
		return false;
	}

	const FTunaSweeperItemSlotReference SlotReference = HoveredItemSlotReference;
	const bool bUsedItem = TryUseItemInSlot(SlotReference, InstigatorPawn);
	if (bUsedItem)
	{
		ClearHoveredItemSlot(SlotReference);
	}

	return bUsedItem;
}

bool UTunaSweeperGameInstance::ToggleInventorySlotSortLock(const FTunaSweeperItemSlotReference& SlotReference)
{
	EnsureInventoryStateInitialized();

	if (SlotReference.Source != ETunaSweeperItemSlotSource::Inventory ||
		!PlayerInventorySlots.IsValidIndex(SlotReference.SlotIndex) ||
		PlayerInventorySlots[SlotReference.SlotIndex].IsEmpty())
	{
		return false;
	}

	PlayerInventorySlots[SlotReference.SlotIndex].bSortLocked =
		!PlayerInventorySlots[SlotReference.SlotIndex].bSortLocked;
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::ToggleHoveredInventorySlotSortLock()
{
	EnsureInventoryStateInitialized();
	return HoveredItemSlotReference.IsValid() && ToggleInventorySlotSortLock(HoveredItemSlotReference);
}

bool UTunaSweeperGameInstance::CanStackItemBetweenSlots(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot,
	FString* OutFailureReason)
{
	EnsureInventoryStateInitialized();
	RefreshSelectedWeaponAttachmentSlots();

	auto SetFailure = [OutFailureReason](const TCHAR* Reason)
	{
		if (OutFailureReason)
		{
			*OutFailureReason = Reason;
		}
		return false;
	};

	if (!SourceSlot.IsValid() || !TargetSlot.IsValid())
	{
		return SetFailure(TEXT("Invalid slot."));
	}

	if (SourceSlot.Source == TargetSlot.Source && SourceSlot.SlotIndex == TargetSlot.SlotIndex)
	{
		return SetFailure(TEXT("Same slot."));
	}

	const TArray<FTunaSweeperInventorySlot>* SourceSlots = GetSlotsForSource(SourceSlot.Source);
	const TArray<FTunaSweeperInventorySlot>* TargetSlots = GetSlotsForSource(TargetSlot.Source);
	if (!SourceSlots || !TargetSlots ||
		!SourceSlots->IsValidIndex(SourceSlot.SlotIndex) ||
		!TargetSlots->IsValidIndex(TargetSlot.SlotIndex))
	{
		return SetFailure(TEXT("Slot is out of range."));
	}

	const FGuid SourceUid = (*SourceSlots)[SourceSlot.SlotIndex].ItemUid;
	const FGuid TargetUid = (*TargetSlots)[TargetSlot.SlotIndex].ItemUid;
	if (!SourceUid.IsValid() || !TargetUid.IsValid())
	{
		return SetFailure(TEXT("Both slots must contain items."));
	}

	if (!CanSlotAcceptItem(TargetSlot, SourceUid))
	{
		return SetFailure(TEXT("Target slot does not accept this item."));
	}

	const FTunaSweeperItemInstance* SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
	const FTunaSweeperItemInstance* TargetItemInstance = ItemInstancesByUid.Find(TargetUid);
	if (!SourceItemInstance || !TargetItemInstance ||
		!CanStackItemInstances(*SourceItemInstance, *TargetItemInstance))
	{
		return SetFailure(TEXT("Items cannot be stacked."));
	}

	if (OutFailureReason)
	{
		OutFailureReason->Reset();
	}
	return true;
}

bool UTunaSweeperGameInstance::CanMoveItemBetweenSlots(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot,
	FString* OutFailureReason)
{
	EnsureInventoryStateInitialized();
	RefreshSelectedWeaponAttachmentSlots();

	auto SetFailure = [OutFailureReason](const TCHAR* Reason)
	{
		if (OutFailureReason)
		{
			*OutFailureReason = Reason;
		}
		return false;
	};

	if (!SourceSlot.IsValid() || !TargetSlot.IsValid())
	{
		return SetFailure(TEXT("Invalid slot."));
	}

	if (SourceSlot.Source == TargetSlot.Source && SourceSlot.SlotIndex == TargetSlot.SlotIndex)
	{
		return SetFailure(TEXT("Same slot."));
	}

	const TArray<FTunaSweeperInventorySlot>* SourceSlots = GetSlotsForSource(SourceSlot.Source);
	const TArray<FTunaSweeperInventorySlot>* TargetSlots = GetSlotsForSource(TargetSlot.Source);
	if (!SourceSlots || !TargetSlots ||
		!SourceSlots->IsValidIndex(SourceSlot.SlotIndex) ||
		!TargetSlots->IsValidIndex(TargetSlot.SlotIndex))
	{
		return SetFailure(TEXT("Slot is out of range."));
	}

	const FGuid SourceUid = (*SourceSlots)[SourceSlot.SlotIndex].ItemUid;
	const FGuid TargetUid = (*TargetSlots)[TargetSlot.SlotIndex].ItemUid;
	if (!SourceUid.IsValid())
	{
		return SetFailure(TEXT("Source slot is empty."));
	}

	FName AttachmentDropSlotTag;
	FGuid ExistingAttachmentUid;
	if (TryResolveItemAttachmentDrop(SourceSlot, TargetSlot, AttachmentDropSlotTag, ExistingAttachmentUid))
	{
		if (ExistingAttachmentUid == SourceUid)
		{
			return SetFailure(TEXT("Item is already attached to target item."));
		}

		if (ExistingAttachmentUid.IsValid() && !CanSlotAcceptItem(SourceSlot, ExistingAttachmentUid))
		{
			return SetFailure(TEXT("Source slot does not accept swapped attachment item."));
		}

		if (OutFailureReason)
		{
			OutFailureReason->Reset();
		}
		return true;
	}

	if (CanStackItemBetweenSlots(SourceSlot, TargetSlot))
	{
		if (OutFailureReason)
		{
			OutFailureReason->Reset();
		}
		return true;
	}

	if (TargetUid.IsValid())
	{
		const FTunaSweeperItemInstance* SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
		const FTunaSweeperItemInstance* TargetItemInstance = ItemInstancesByUid.Find(TargetUid);
		if (SourceItemInstance &&
			TargetItemInstance &&
			SourceItemInstance->ItemId == TargetItemInstance->ItemId &&
			IsStackableItemId(SourceItemInstance->ItemId))
		{
			return SetFailure(TEXT("Target stack is full."));
		}
	}

	if (!CanSlotAcceptItem(TargetSlot, SourceUid))
	{
		return SetFailure(TEXT("Target slot does not accept this item."));
	}

	if (TargetUid.IsValid() && !CanSlotAcceptItem(SourceSlot, TargetUid))
	{
		return SetFailure(TEXT("Source slot does not accept swapped item."));
	}

	TArray<FTunaSweeperInventorySlot> SimInventorySlots = PlayerInventorySlots;
	TArray<FTunaSweeperInventorySlot> SimEquipmentSlots = EquipmentSlots;
	TArray<FTunaSweeperInventorySlot> SimAuxiliaryBagSlots = AuxiliaryBagSlots;
	TArray<FTunaSweeperInventorySlot> SimUsableQuickSlots = UsableQuickSlots;
	TArray<FTunaSweeperInventorySlot> SimStorageSlots = StorageSlots;
	TArray<FTunaSweeperInventorySlot> SimLootContainerSlots = ActiveLootContainerSlots;
	TArray<FTunaSweeperInventorySlot> SimSelectedWeaponAttachmentSlots = SelectedWeaponAttachmentSlots;

	auto GetSimSlots = [
		&SimInventorySlots,
		&SimEquipmentSlots,
		&SimAuxiliaryBagSlots,
		&SimUsableQuickSlots,
		&SimStorageSlots,
		&SimLootContainerSlots,
		&SimSelectedWeaponAttachmentSlots](
		ETunaSweeperItemSlotSource Source) -> TArray<FTunaSweeperInventorySlot>*
	{
		switch (Source)
		{
		case ETunaSweeperItemSlotSource::Equipment:
			return &SimEquipmentSlots;
		case ETunaSweeperItemSlotSource::AuxiliaryBag:
			return &SimAuxiliaryBagSlots;
		case ETunaSweeperItemSlotSource::Inventory:
			return &SimInventorySlots;
		case ETunaSweeperItemSlotSource::UsableQuickSlot:
			return &SimUsableQuickSlots;
		case ETunaSweeperItemSlotSource::Storage:
			return &SimStorageSlots;
		case ETunaSweeperItemSlotSource::LootContainer:
			return &SimLootContainerSlots;
		case ETunaSweeperItemSlotSource::SelectedWeaponAttachment:
			return &SimSelectedWeaponAttachmentSlots;
		default:
			return nullptr;
		}
	};

	TArray<FTunaSweeperInventorySlot>* SimSourceSlots = GetSimSlots(SourceSlot.Source);
	TArray<FTunaSweeperInventorySlot>* SimTargetSlots = GetSimSlots(TargetSlot.Source);
	if (!SimSourceSlots || !SimTargetSlots ||
		!SimSourceSlots->IsValidIndex(SourceSlot.SlotIndex) ||
		!SimTargetSlots->IsValidIndex(TargetSlot.SlotIndex))
	{
		return SetFailure(TEXT("Could not simulate slot move."));
	}

	(*SimSourceSlots)[SourceSlot.SlotIndex].ItemUid = TargetUid;
	(*SimTargetSlots)[TargetSlot.SlotIndex].ItemUid = SourceUid;

	const int32 SimInventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(SimEquipmentSlots);
	if (HasOccupiedInventorySlotsBeyondCapacity(SimInventorySlots, SimInventoryCapacity))
	{
		return SetFailure(TEXT("Inventory overflow would be created."));
	}

	if (OutFailureReason)
	{
		OutFailureReason->Reset();
	}
	return true;
}

bool UTunaSweeperGameInstance::MoveItemBetweenSlots(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot)
{
	FString FailureReason;
	if (!CanMoveItemBetweenSlots(SourceSlot, TargetSlot, &FailureReason))
	{
		return false;
	}

	TArray<FTunaSweeperInventorySlot>* SourceSlots = GetMutableSlotsForSource(SourceSlot.Source);
	TArray<FTunaSweeperInventorySlot>* TargetSlots = GetMutableSlotsForSource(TargetSlot.Source);
	if (!SourceSlots || !TargetSlots ||
		!SourceSlots->IsValidIndex(SourceSlot.SlotIndex) ||
		!TargetSlots->IsValidIndex(TargetSlot.SlotIndex))
	{
		return false;
	}

	const FGuid SourceUid = (*SourceSlots)[SourceSlot.SlotIndex].ItemUid;
	const FGuid TargetUid = (*TargetSlots)[TargetSlot.SlotIndex].ItemUid;
	const bool bAcquiredFromLootContainer =
		SourceSlot.Source == ETunaSweeperItemSlotSource::LootContainer &&
		TargetSlot.Source != ETunaSweeperItemSlotSource::LootContainer;
	int32 AcquiredItemId = INDEX_NONE;
	int32 AcquiredQuantity = 0;
	if (bAcquiredFromLootContainer)
	{
		if (const FTunaSweeperItemInstance* AcquiredItemInstance = ItemInstancesByUid.Find(SourceUid))
		{
			AcquiredItemId = AcquiredItemInstance->ItemId;
			AcquiredQuantity = AcquiredItemInstance->Quantity;
		}
	}

	FName AttachmentDropSlotTag;
	FGuid ExistingAttachmentUid;
	if (TryResolveItemAttachmentDrop(SourceSlot, TargetSlot, AttachmentDropSlotTag, ExistingAttachmentUid))
	{
		if (ExistingAttachmentUid == SourceUid ||
			(ExistingAttachmentUid.IsValid() && !CanSlotAcceptItem(SourceSlot, ExistingAttachmentUid)) ||
			!ApplyItemAttachmentDrop(SourceSlot, TargetSlot, AttachmentDropSlotTag, ExistingAttachmentUid))
		{
			return false;
		}

		const int32 NewInventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots);
		EnsureSlotArraySize(PlayerInventorySlots, NewInventoryCapacity);
		BroadcastInventoryStateChanged();
		if (bAcquiredFromLootContainer && AcquiredItemId != INDEX_NONE && AcquiredQuantity > 0)
		{
			MarkItemEverAcquired(AcquiredItemId);
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->NotifyItemAcquired(AcquiredItemId, AcquiredQuantity, !IsCurrentWorldBunkerMap());
			}
			AddRaidExperienceForItem(AcquiredItemId, AcquiredQuantity);
		}
		MarkItemStateMutationForSave();
		return true;
	}

	int32 MergedItemId = INDEX_NONE;
	int32 MergedQuantity = 0;
	if (TryMergeItemStacksBetweenSlots(SourceSlot, TargetSlot, MergedItemId, MergedQuantity))
	{
		ClearSelectedItemIfInvalid();
		BroadcastInventoryStateChanged();
		if (bAcquiredFromLootContainer && MergedItemId != INDEX_NONE && MergedQuantity > 0)
		{
			MarkItemEverAcquired(MergedItemId);
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->NotifyItemAcquired(MergedItemId, MergedQuantity, !IsCurrentWorldBunkerMap());
			}
			AddRaidExperienceForItem(MergedItemId, MergedQuantity);
		}
		MarkItemStateMutationForSave();
		return true;
	}

	if (!TargetUid.IsValid())
	{
		FTunaSweeperItemInstance* SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
		FTunaSweeperItemDefinition SourceItemDefinition;
		if (SourceItemInstance &&
			ItemDataSubsystem &&
			ItemDataSubsystem->TryGetItemDefinition(SourceItemInstance->ItemId, SourceItemDefinition))
		{
			const int32 MaxStackQuantity = FMath::Max(1, ItemDataSubsystem->ResolveItemMaxStackQuantity(SourceItemDefinition));
			if (MaxStackQuantity > 1 && SourceItemInstance->Quantity > MaxStackQuantity)
			{
				const int32 MovedQuantity = MaxStackQuantity;
				const int32 MovedItemId = SourceItemInstance->ItemId;
				const FGuid NewStackUid = CreateItemInstance(MovedItemId, MovedQuantity);
				if (!NewStackUid.IsValid())
				{
					return false;
				}

				SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
				if (!SourceItemInstance)
				{
					ItemInstancesByUid.Remove(NewStackUid);
					return false;
				}
				SourceItemInstance->Quantity -= MovedQuantity;
				(*TargetSlots)[TargetSlot.SlotIndex].ItemUid = NewStackUid;
				if (TargetSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
				{
					CommitSelectedWeaponAttachmentSlotsToSelectedItem();
				}

				ClearSelectedItemIfInvalid();
				BroadcastInventoryStateChanged();
				if (bAcquiredFromLootContainer)
				{
					MarkItemEverAcquired(MovedItemId);
					if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
					{
						QuestSubsystem->NotifyItemAcquired(MovedItemId, MovedQuantity, !IsCurrentWorldBunkerMap());
					}
					AddRaidExperienceForItem(MovedItemId, MovedQuantity);
				}
				MarkItemStateMutationForSave();
				return true;
			}
		}
	}

	(*SourceSlots)[SourceSlot.SlotIndex].ItemUid = TargetUid;
	(*TargetSlots)[TargetSlot.SlotIndex].ItemUid = SourceUid;
	if (SourceSlot.Source == ETunaSweeperItemSlotSource::Inventory &&
		SourceSlots->IsValidIndex(SourceSlot.SlotIndex) &&
		(*SourceSlots)[SourceSlot.SlotIndex].IsEmpty())
	{
		(*SourceSlots)[SourceSlot.SlotIndex].bSortLocked = false;
	}
	if (TargetSlot.Source == ETunaSweeperItemSlotSource::Inventory &&
		TargetSlots->IsValidIndex(TargetSlot.SlotIndex) &&
		(*TargetSlots)[TargetSlot.SlotIndex].IsEmpty())
	{
		(*TargetSlots)[TargetSlot.SlotIndex].bSortLocked = false;
	}
	if (SourceSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment ||
		TargetSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
	{
		CommitSelectedWeaponAttachmentSlotsToSelectedItem();
	}

	const int32 NewInventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots);
	EnsureSlotArraySize(PlayerInventorySlots, NewInventoryCapacity);
	BroadcastInventoryStateChanged();
	if (bAcquiredFromLootContainer && AcquiredItemId != INDEX_NONE && AcquiredQuantity > 0)
	{
		MarkItemEverAcquired(AcquiredItemId);
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyItemAcquired(AcquiredItemId, AcquiredQuantity, !IsCurrentWorldBunkerMap());
		}
		AddRaidExperienceForItem(AcquiredItemId, AcquiredQuantity);
	}
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::CanSplitItemStackBetweenSlots(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot,
	int32& OutDefaultSplitQuantity,
	int32& OutMaxSplitQuantity,
	FString* OutFailureReason)
{
	EnsureInventoryStateInitialized();
	RefreshSelectedWeaponAttachmentSlots();

	OutDefaultSplitQuantity = 0;
	OutMaxSplitQuantity = 0;

	auto SetFailure = [OutFailureReason](const TCHAR* Reason)
	{
		if (OutFailureReason)
		{
			*OutFailureReason = Reason;
		}
		return false;
	};

	if (!SourceSlot.IsValid() || !TargetSlot.IsValid())
	{
		return SetFailure(TEXT("Invalid slot."));
	}

	if (SourceSlot.Source == TargetSlot.Source && SourceSlot.SlotIndex == TargetSlot.SlotIndex)
	{
		return SetFailure(TEXT("Same slot."));
	}

	const TArray<FTunaSweeperInventorySlot>* SourceSlots = GetSlotsForSource(SourceSlot.Source);
	const TArray<FTunaSweeperInventorySlot>* TargetSlots = GetSlotsForSource(TargetSlot.Source);
	if (!SourceSlots || !TargetSlots ||
		!SourceSlots->IsValidIndex(SourceSlot.SlotIndex) ||
		!TargetSlots->IsValidIndex(TargetSlot.SlotIndex))
	{
		return SetFailure(TEXT("Slot is out of range."));
	}

	const FGuid SourceUid = (*SourceSlots)[SourceSlot.SlotIndex].ItemUid;
	const FGuid TargetUid = (*TargetSlots)[TargetSlot.SlotIndex].ItemUid;
	if (!SourceUid.IsValid())
	{
		return SetFailure(TEXT("Source slot is empty."));
	}

	if (TargetUid.IsValid())
	{
		return SetFailure(TEXT("Target slot is not empty."));
	}

	const FTunaSweeperItemInstance* SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
	if (!SourceItemInstance || !SourceItemInstance->IsValid())
	{
		return SetFailure(TEXT("Source item is invalid."));
	}

	if (SourceItemInstance->Quantity <= 1)
	{
		return SetFailure(TEXT("Source item is not stackable."));
	}

	if (!SourceItemInstance->AttachmentSlots.IsEmpty() ||
		SourceItemInstance->LoadedAmmoItemId != INDEX_NONE ||
		SourceItemInstance->LoadedAmmoCount > 0 ||
		SourceItemInstance->SelectedAmmoItemId != INDEX_NONE)
	{
		return SetFailure(TEXT("Source item has per-instance state."));
	}

	if (!CanSlotAcceptItem(TargetSlot, SourceUid))
	{
		return SetFailure(TEXT("Target slot does not accept this item."));
	}

	OutMaxSplitQuantity = FMath::Max(0, SourceItemInstance->Quantity - 1);
	OutDefaultSplitQuantity = FMath::FloorToInt(static_cast<float>(SourceItemInstance->Quantity) * 0.5f);
	if (OutDefaultSplitQuantity <= 0 || OutMaxSplitQuantity <= 0)
	{
		OutDefaultSplitQuantity = 0;
		OutMaxSplitQuantity = 0;
		return SetFailure(TEXT("Split quantity is empty."));
	}

	OutDefaultSplitQuantity = FMath::Clamp(OutDefaultSplitQuantity, 1, OutMaxSplitQuantity);
	if (OutFailureReason)
	{
		OutFailureReason->Reset();
	}
	return true;
}

bool UTunaSweeperGameInstance::SplitItemStackBetweenSlots(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot,
	int32 SplitQuantity)
{
	int32 DefaultSplitQuantity = 0;
	int32 MaxSplitQuantity = 0;
	FString FailureReason;
	if (!CanSplitItemStackBetweenSlots(SourceSlot, TargetSlot, DefaultSplitQuantity, MaxSplitQuantity, &FailureReason))
	{
		return false;
	}

	SplitQuantity = FMath::Clamp(SplitQuantity, 1, MaxSplitQuantity);

	TArray<FTunaSweeperInventorySlot>* SourceSlots = GetMutableSlotsForSource(SourceSlot.Source);
	TArray<FTunaSweeperInventorySlot>* TargetSlots = GetMutableSlotsForSource(TargetSlot.Source);
	if (!SourceSlots || !TargetSlots ||
		!SourceSlots->IsValidIndex(SourceSlot.SlotIndex) ||
		!TargetSlots->IsValidIndex(TargetSlot.SlotIndex) ||
		!(*TargetSlots)[TargetSlot.SlotIndex].IsEmpty())
	{
		return false;
	}

	const FGuid SourceUid = (*SourceSlots)[SourceSlot.SlotIndex].ItemUid;
	FTunaSweeperItemInstance* SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
	if (!SourceItemInstance || !SourceItemInstance->IsValid() || SourceItemInstance->Quantity <= SplitQuantity)
	{
		return false;
	}

	const int32 SplitItemId = SourceItemInstance->ItemId;
	SourceItemInstance->Quantity -= SplitQuantity;
	const FGuid SplitItemUid = CreateItemInstance(SplitItemId, SplitQuantity);
	if (!SplitItemUid.IsValid())
	{
		SourceItemInstance->Quantity += SplitQuantity;
		return false;
	}

	(*TargetSlots)[TargetSlot.SlotIndex].ItemUid = SplitItemUid;
	if (TargetSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
	{
		CommitSelectedWeaponAttachmentSlotsToSelectedItem();
	}

	ClearSelectedItemIfInvalid();
	BroadcastInventoryStateChanged();

	if (SourceSlot.Source == ETunaSweeperItemSlotSource::LootContainer &&
		TargetSlot.Source != ETunaSweeperItemSlotSource::LootContainer)
	{
		MarkItemEverAcquired(SplitItemId);
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyItemAcquired(SplitItemId, SplitQuantity, !IsCurrentWorldBunkerMap());
		}
		AddRaidExperienceForItem(SplitItemId, SplitQuantity);
	}
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::RemoveItemFromSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	FTunaSweeperItemInstance& OutRemovedItemInstance)
{
	EnsureInventoryStateInitialized();
	RefreshSelectedWeaponAttachmentSlots();

	OutRemovedItemInstance = FTunaSweeperItemInstance();
	if (!SlotReference.IsValid())
	{
		return false;
	}

	TArray<FTunaSweeperInventorySlot>* Slots = GetMutableSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		return false;
	}

	const FGuid ItemUid = (*Slots)[SlotReference.SlotIndex].ItemUid;
	if (!TryGetItemInstance(ItemUid, OutRemovedItemInstance))
	{
		return false;
	}

	if (SlotReference.Source == ETunaSweeperItemSlotSource::Equipment)
	{
		TArray<FTunaSweeperInventorySlot> SimEquipmentSlots = EquipmentSlots;
		if (SimEquipmentSlots.IsValidIndex(SlotReference.SlotIndex))
		{
			SimEquipmentSlots[SlotReference.SlotIndex].Clear();
			const int32 SimInventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(SimEquipmentSlots);
			if (HasOccupiedInventorySlotsBeyondCapacity(PlayerInventorySlots, SimInventoryCapacity))
			{
				OutRemovedItemInstance = FTunaSweeperItemInstance();
				return false;
			}
		}
	}

	(*Slots)[SlotReference.SlotIndex].Clear();
	if (SlotReference.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
	{
		CommitSelectedWeaponAttachmentSlotsToSelectedItem();
	}

	TFunction<void(const FGuid&)> RemoveItemUid = [this, &RemoveItemUid](const FGuid& Uid)
	{
		if (!Uid.IsValid())
		{
			return;
		}

		TArray<FGuid> AttachmentUids;
		if (const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Uid))
		{
			for (const TPair<FName, FGuid>& AttachmentSlot : ItemInstance->AttachmentSlots)
			{
				AttachmentUids.Add(AttachmentSlot.Value);
			}
		}

		ItemInstancesByUid.Remove(Uid);
		for (const FGuid& AttachmentUid : AttachmentUids)
		{
			RemoveItemUid(AttachmentUid);
		}
	};
	RemoveItemUid(ItemUid);

	if (HoveredItemSlotReference.Source == SlotReference.Source &&
		HoveredItemSlotReference.SlotIndex == SlotReference.SlotIndex)
	{
		HoveredItemSlotReference = FTunaSweeperItemSlotReference();
	}

	const int32 NewInventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots);
	EnsureSlotArraySize(PlayerInventorySlots, NewInventoryCapacity);
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::AddItemToFirstAvailableInventorySlot(int32 ItemId, int32 Quantity)
{
	EnsureInventoryStateInitialized();
	if (ItemId == INDEX_NONE || Quantity <= 0)
	{
		return false;
	}

	const TMap<FGuid, FTunaSweeperItemInstance> PreviousItemInstances = ItemInstancesByUid;
	const TArray<FTunaSweeperInventorySlot> PreviousInventorySlots = PlayerInventorySlots;
	int32 RemainingQuantity = Quantity;
	TryAddItemQuantityToExistingStacks(ItemId, RemainingQuantity, PlayerInventorySlots);
	TryAddItemQuantityToFirstEmptySlots(ItemId, RemainingQuantity, PlayerInventorySlots);
	if (RemainingQuantity > 0)
	{
		ItemInstancesByUid = PreviousItemInstances;
		PlayerInventorySlots = PreviousInventorySlots;
		return false;
	}

	BroadcastInventoryStateChanged();
	MarkItemEverAcquired(ItemId);
	MarkItemStateMutationForSave();
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->NotifyItemAcquired(ItemId, Quantity, !IsCurrentWorldBunkerMap());
	}
	AddRaidExperienceForItem(ItemId, Quantity);
	return true;
}

bool UTunaSweeperGameInstance::AddItemToPreferredAvailableSlot(int32 ItemId, int32 Quantity)
{
	EnsureInventoryStateInitialized();
	if (ItemId == INDEX_NONE || Quantity <= 0)
	{
		return false;
	}

	const TMap<FGuid, FTunaSweeperItemInstance> PreviousItemInstances = ItemInstancesByUid;
	const TArray<FTunaSweeperInventorySlot> PreviousInventorySlots = PlayerInventorySlots;
	const TArray<FTunaSweeperInventorySlot> PreviousEquipmentSlots = EquipmentSlots;

	bool bAdded = false;
	if (IsStackableItemId(ItemId))
	{
		int32 RemainingQuantity = Quantity;
		TryAddItemQuantityToExistingStacks(ItemId, RemainingQuantity, PlayerInventorySlots);
		TryAddItemQuantityToFirstEmptySlots(ItemId, RemainingQuantity, PlayerInventorySlots);
		bAdded = RemainingQuantity <= 0;
	}
	else if (Quantity == 1)
	{
		const FGuid ItemUid = CreateItemInstance(ItemId, Quantity);
		bAdded = AddItemUidToFirstEmptyCompatibleEquipmentSlot(ItemUid) ||
			AddItemUidToFirstEmptySlot(ItemUid, PlayerInventorySlots);
		if (!bAdded)
		{
			ItemInstancesByUid.Remove(ItemUid);
		}
	}
	else
	{
		int32 RemainingQuantity = Quantity;
		TryAddItemQuantityToFirstEmptySlots(ItemId, RemainingQuantity, PlayerInventorySlots);
		bAdded = RemainingQuantity <= 0;
	}

	if (!bAdded)
	{
		ItemInstancesByUid = PreviousItemInstances;
		PlayerInventorySlots = PreviousInventorySlots;
		EquipmentSlots = PreviousEquipmentSlots;
		return false;
	}

	BroadcastInventoryStateChanged();
	MarkItemEverAcquired(ItemId);
	MarkItemStateMutationForSave();
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->NotifyItemAcquired(ItemId, Quantity, !IsCurrentWorldBunkerMap());
	}
	AddRaidExperienceForItem(ItemId, Quantity);
	return true;
}

int32 UTunaSweeperGameInstance::CountInventoryItemById(int32 ItemId)
{
	EnsureInventoryStateInitialized();
	return CountInventoryAmmoByItemId(ItemId);
}

int32 UTunaSweeperGameInstance::ConsumeInventoryItemById(int32 ItemId, int32 RequestedAmount)
{
	EnsureInventoryStateInitialized();
	const int32 ConsumedAmount = ConsumeInventoryAmmoByItemId(ItemId, RequestedAmount);
	if (ConsumedAmount > 0)
	{
		ClearSelectedItemIfInvalid();
		BroadcastInventoryStateChanged();
		MarkItemStateMutationForSave();
	}
	return ConsumedAmount;
}

bool UTunaSweeperGameInstance::GrantQuestItemRewards(const TArray<FTunaSweeperItemStack>& ItemRewards)
{
	EnsureInventoryStateInitialized();
	if (!CanGrantQuestItemRewards(ItemRewards))
	{
		return false;
	}

	const TMap<FGuid, FTunaSweeperItemInstance> PreviousItemInstances = ItemInstancesByUid;
	const TArray<FTunaSweeperInventorySlot> PreviousInventorySlots = PlayerInventorySlots;
	TArray<int32> GrantedItemIds;
	for (const FTunaSweeperItemStack& ItemReward : ItemRewards)
	{
		if (ItemReward.ItemId == INDEX_NONE || ItemReward.Quantity <= 0)
		{
			continue;
		}

		int32 RemainingQuantity = ItemReward.Quantity;
		TryAddItemQuantityToExistingStacks(ItemReward.ItemId, RemainingQuantity, PlayerInventorySlots);
		TryAddItemQuantityToFirstEmptySlots(ItemReward.ItemId, RemainingQuantity, PlayerInventorySlots);
		if (RemainingQuantity > 0)
		{
			ItemInstancesByUid = PreviousItemInstances;
			PlayerInventorySlots = PreviousInventorySlots;
			return false;
		}

		GrantedItemIds.AddUnique(ItemReward.ItemId);
	}

	if (GrantedItemIds.Num() > 0)
	{
		for (const int32 GrantedItemId : GrantedItemIds)
		{
			MarkItemEverAcquired(GrantedItemId);
		}
		BroadcastInventoryStateChanged();
		MarkItemStateMutationForSave();
	}
	return true;
}

void UTunaSweeperGameInstance::CompactInventorySlots()
{
	EnsureInventoryStateInitialized();

	TArray<FGuid> MovableItemUids;
	for (const FTunaSweeperInventorySlot& Slot : PlayerInventorySlots)
	{
		if (!Slot.bSortLocked && Slot.ItemUid.IsValid())
		{
			MovableItemUids.Add(Slot.ItemUid);
		}
	}

	for (FTunaSweeperInventorySlot& Slot : PlayerInventorySlots)
	{
		if (!Slot.bSortLocked)
		{
			Slot.Clear();
		}
	}

	int32 MovableItemIndex = 0;
	for (FTunaSweeperInventorySlot& Slot : PlayerInventorySlots)
	{
		if (Slot.bSortLocked)
		{
			continue;
		}

		if (MovableItemUids.IsValidIndex(MovableItemIndex))
		{
			Slot.ItemUid = MovableItemUids[MovableItemIndex++];
		}
	}

	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
}

void UTunaSweeperGameInstance::CompactStorageSlots()
{
	EnsureInventoryStateInitialized();

	TArray<FGuid> MovableItemUids;
	for (const FTunaSweeperInventorySlot& Slot : StorageSlots)
	{
		if (!Slot.bSortLocked && Slot.ItemUid.IsValid())
		{
			MovableItemUids.Add(Slot.ItemUid);
		}
	}

	for (FTunaSweeperInventorySlot& Slot : StorageSlots)
	{
		if (!Slot.bSortLocked)
		{
			Slot.Clear();
		}
	}

	int32 MovableItemIndex = 0;
	for (FTunaSweeperInventorySlot& Slot : StorageSlots)
	{
		if (Slot.bSortLocked)
		{
			continue;
		}

		if (MovableItemUids.IsValidIndex(MovableItemIndex))
		{
			Slot.ItemUid = MovableItemUids[MovableItemIndex++];
		}
	}

	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
}

int32 UTunaSweeperGameInstance::GetStorageSlotCapacity()
{
	EnsureInventoryStateInitialized();
	return StorageSlotCapacity;
}

bool UTunaSweeperGameInstance::SetStorageSlotCapacity(int32 NewCapacity, bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	NewCapacity = NormalizeStorageSlotCapacity(NewCapacity);
	for (int32 SlotIndex = NewCapacity; SlotIndex < StorageSlots.Num(); ++SlotIndex)
	{
		if (StorageSlots[SlotIndex].ItemUid.IsValid())
		{
			return false;
		}
	}

	if (StorageSlotCapacity == NewCapacity && StorageSlots.Num() == NewCapacity)
	{
		return true;
	}

	StorageSlotCapacity = NewCapacity;
	EnsureSlotArraySize(StorageSlots, StorageSlotCapacity);
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave(bSaveImmediately);
	return true;
}

void UTunaSweeperGameInstance::BroadcastInventoryStateChanged()
{
	bHasGeneratedPlayerInventoryItems = false;
	RefreshLegacyPlayerInventoryItems();
	ClearSelectedItemIfInvalid();
	RefreshCarryWeightState();
	OnInventoryStateChanged.Broadcast();
}

void UTunaSweeperGameInstance::MarkItemEverAcquired(int32 ItemId)
{
	if (ItemId != INDEX_NONE)
	{
		EverAcquiredItemIds.Add(ItemId);
	}
}

void UTunaSweeperGameInstance::BackfillEverAcquiredItemIdsFromCurrentItems()
{
	for (const TPair<FGuid, FTunaSweeperItemInstance>& ItemPair : ItemInstancesByUid)
	{
		if (ItemPair.Value.ItemId != INDEX_NONE && ItemPair.Value.Quantity > 0)
		{
			EverAcquiredItemIds.Add(ItemPair.Value.ItemId);
		}
	}
}

FGuid UTunaSweeperGameInstance::CreateItemInstance(int32 ItemId, int32 Quantity)
{
	FTunaSweeperItemInstance ItemInstance;
	ItemInstance.Uid = FGuid::NewGuid();
	ItemInstance.ItemId = ItemId;
	ItemInstance.Quantity = FMath::Max(1, Quantity);
	ItemInstancesByUid.Add(ItemInstance.Uid, ItemInstance);
	return ItemInstance.Uid;
}

bool UTunaSweeperGameInstance::TryAddItemQuantityToExistingStacks(
	int32 ItemId,
	int32& InOutQuantity,
	TArray<FTunaSweeperInventorySlot>& Slots)
{
	if (ItemId == INDEX_NONE || InOutQuantity <= 0 || !IsStackableItemId(ItemId))
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition))
	{
		return false;
	}

	const int32 MaxStackQuantity = FMath::Max(1, ItemDataSubsystem->ResolveItemMaxStackQuantity(ItemDefinition));
	bool bAddedAny = false;
	for (FTunaSweeperInventorySlot& Slot : Slots)
	{
		if (InOutQuantity <= 0)
		{
			break;
		}

		FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
		if (!ItemInstance ||
			ItemInstance->ItemId != ItemId ||
			!DoesItemInstanceAllowStacking(*ItemInstance) ||
			ItemInstance->Quantity >= MaxStackQuantity)
		{
			continue;
		}

		const int32 AddedQuantity = FMath::Min(InOutQuantity, MaxStackQuantity - ItemInstance->Quantity);
		if (AddedQuantity <= 0)
		{
			continue;
		}

		ItemInstance->Quantity += AddedQuantity;
		InOutQuantity -= AddedQuantity;
		bAddedAny = true;
	}

	return bAddedAny;
}

bool UTunaSweeperGameInstance::TryAddItemQuantityToFirstEmptySlots(
	int32 ItemId,
	int32& InOutQuantity,
	TArray<FTunaSweeperInventorySlot>& Slots,
	TArray<FGuid>* OutCreatedItemUids)
{
	if (ItemId == INDEX_NONE || InOutQuantity <= 0)
	{
		return false;
	}

	int32 MaxStackQuantity = 1;
	if (UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>())
	{
		FTunaSweeperItemDefinition ItemDefinition;
		if (ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition))
		{
			MaxStackQuantity = FMath::Max(1, ItemDataSubsystem->ResolveItemMaxStackQuantity(ItemDefinition));
		}
	}

	bool bAddedAny = false;
	for (FTunaSweeperInventorySlot& Slot : Slots)
	{
		if (InOutQuantity <= 0)
		{
			break;
		}

		if (!Slot.IsEmpty())
		{
			continue;
		}

		const int32 NewStackQuantity = FMath::Min(InOutQuantity, MaxStackQuantity);
		const FGuid ItemUid = CreateItemInstance(ItemId, NewStackQuantity);
		if (!ItemUid.IsValid())
		{
			continue;
		}

		Slot.ItemUid = ItemUid;
		if (OutCreatedItemUids)
		{
			OutCreatedItemUids->Add(ItemUid);
		}
		InOutQuantity -= NewStackQuantity;
		bAddedAny = true;
	}

	return bAddedAny;
}

bool UTunaSweeperGameInstance::AddItemUidToFirstEmptySlot(
	const FGuid& ItemUid,
	TArray<FTunaSweeperInventorySlot>& Slots)
{
	if (!ItemUid.IsValid())
	{
		return false;
	}

	for (FTunaSweeperInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			Slot.ItemUid = ItemUid;
			return true;
		}
	}

	return false;
}

bool UTunaSweeperGameInstance::AddItemUidToFirstEmptyCompatibleEquipmentSlot(const FGuid& ItemUid)
{
	if (!ItemUid.IsValid())
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
	{
		if (!EquipmentSlots[SlotIndex].IsEmpty() ||
			!IsItemCompatibleWithEquipmentSlot(SlotIndex, ItemUid))
		{
			continue;
		}

		EquipmentSlots[SlotIndex].ItemUid = ItemUid;
		return true;
	}

	return false;
}

void UTunaSweeperGameInstance::RemoveInvalidSlotReferences(TArray<FTunaSweeperInventorySlot>& Slots) const
{
	for (FTunaSweeperInventorySlot& Slot : Slots)
	{
		if (Slot.ItemUid.IsValid() && !ItemInstancesByUid.Contains(Slot.ItemUid))
		{
			Slot.Clear();
		}
	}
}

void UTunaSweeperGameInstance::EnsureSlotArraySize(
	TArray<FTunaSweeperInventorySlot>& Slots,
	int32 DesiredSize) const
{
	DesiredSize = FMath::Max(0, DesiredSize);
	while (Slots.Num() < DesiredSize)
	{
		Slots.AddDefaulted();
	}

	if (Slots.Num() > DesiredSize)
	{
		Slots.SetNum(DesiredSize);
	}
}

int32 UTunaSweeperGameInstance::GetDefaultStorageSlotCapacity() const
{
	return FMath::Max(TunaSweeperInventory::DefaultStorageSlotCount, GameplaySettings.DefaultStorageSlotCount);
}

int32 UTunaSweeperGameInstance::GetMaxStorageSlotCapacity() const
{
	return FMath::Max(GetDefaultStorageSlotCapacity(), FMath::Max(TunaSweeperInventory::MaxStorageSlotCount, GameplaySettings.MaxStorageSlotCount));
}

int32 UTunaSweeperGameInstance::NormalizeStorageSlotCapacity(int32 RequestedCapacity) const
{
	return FMath::Clamp(RequestedCapacity, GetDefaultStorageSlotCapacity(), GetMaxStorageSlotCapacity());
}
