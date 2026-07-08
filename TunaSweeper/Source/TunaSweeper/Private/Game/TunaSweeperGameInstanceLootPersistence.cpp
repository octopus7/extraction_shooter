#include "TunaSweeperGameInstanceShared.h"

void UTunaSweeperGameInstance::SelectItemSlot(const FTunaSweeperItemSlotReference& SlotReference)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperItemInstance ItemInstance;
	if (!TryGetSlotItemInstance(SlotReference, ItemInstance))
	{
		ClearSelectedItemSelection();
		return;
	}

	SelectedItemSlotReference = SlotReference;
	RefreshSelectedWeaponAttachmentSlots();
	OnSelectedInventoryItemChanged.Broadcast();
}

void UTunaSweeperGameInstance::ClearSelectedItemSelection()
{
	const bool bHadSelection = SelectedItemSlotReference.IsValid() ||
		SelectedWeaponAttachmentSlotTags.Num() > 0 ||
		SelectedWeaponAttachmentSlots.Num() > 0;
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();

	if (bHadSelection)
	{
		OnSelectedInventoryItemChanged.Broadcast();
	}
}

void UTunaSweeperGameInstance::SetHoveredItemSlot(const FTunaSweeperItemSlotReference& SlotReference)
{
	HoveredItemSlotReference = SlotReference.IsValid()
		? SlotReference
		: FTunaSweeperItemSlotReference();
}

void UTunaSweeperGameInstance::ClearHoveredItemSlot(const FTunaSweeperItemSlotReference& SlotReference)
{
	if (!SlotReference.IsValid() ||
		(HoveredItemSlotReference.Source == SlotReference.Source &&
			HoveredItemSlotReference.SlotIndex == SlotReference.SlotIndex))
	{
		ClearHoveredItemSlot();
	}
}

void UTunaSweeperGameInstance::ClearHoveredItemSlot()
{
	HoveredItemSlotReference = FTunaSweeperItemSlotReference();
}

void UTunaSweeperGameInstance::SetActiveLootContainerInstance(
	const FTunaSweeperLootContainerInstance& InContainerInstance,
	UObject* InOwner)
{
	EnsureInventoryStateInitialized();

	ActiveLootContainerDisplayName = InContainerInstance.DisplayName;
	ActiveLootContainerCapacity = FMath::Max(0, InContainerInstance.Capacity);
	ActiveLootContainerOwner = InOwner;
	ActiveLootContainerSlots.Reset();
	EnsureSlotArraySize(ActiveLootContainerSlots, ActiveLootContainerCapacity);

	for (int32 SlotIndex = 0; SlotIndex < ActiveLootContainerCapacity && InContainerInstance.Items.IsValidIndex(SlotIndex); ++SlotIndex)
	{
		const FTunaSweeperItemStack& ItemStack = InContainerInstance.Items[SlotIndex];
		if (ItemStack.ItemId == INDEX_NONE || ItemStack.Quantity <= 0)
		{
			continue;
		}

		ActiveLootContainerSlots[SlotIndex].ItemUid = CreateItemInstance(ItemStack.ItemId, ItemStack.Quantity);
	}

	bHasActiveLootContainer = true;
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::SetActiveLootContainerRuntimeSlots(
	const FTunaSweeperLootContainerInstance& InContainerInstance,
	const TArray<FTunaSweeperInventorySlot>& InRuntimeSlots,
	UObject* InOwner)
{
	EnsureInventoryStateInitialized();

	ActiveLootContainerDisplayName = InContainerInstance.DisplayName;
	ActiveLootContainerCapacity = FMath::Max(0, InContainerInstance.Capacity);
	ActiveLootContainerOwner = InOwner;
	ActiveLootContainerSlots = InRuntimeSlots;
	EnsureSlotArraySize(ActiveLootContainerSlots, ActiveLootContainerCapacity);
	RemoveInvalidSlotReferences(ActiveLootContainerSlots);

	bHasActiveLootContainer = true;
	BroadcastInventoryStateChanged();
}

FGuid UTunaSweeperGameInstance::CreateItemInstanceFromTemplate(const FTunaSweeperItemInstance& ItemInstanceTemplate)
{
	EnsureInventoryStateInitialized();

	if (ItemInstanceTemplate.ItemId == INDEX_NONE || ItemInstanceTemplate.Quantity <= 0)
	{
		return FGuid();
	}

	FTunaSweeperItemInstance ItemInstance = ItemInstanceTemplate;
	ItemInstance.Uid = FGuid::NewGuid();
	ItemInstance.Quantity = FMath::Max(1, ItemInstance.Quantity);
	ItemInstance.LoadedAmmoCount = FMath::Max(0, ItemInstance.LoadedAmmoCount);
	ItemInstance.LootLoadedAmmoSourceCount = FMath::Max(0, ItemInstance.LootLoadedAmmoSourceCount);
	ItemInstance.LootLoadedAmmoDeductedCount = FMath::Max(0, ItemInstance.LootLoadedAmmoDeductedCount);
	ItemInstance.LootLoadedAmmoDeductionRatio = FMath::Clamp(ItemInstance.LootLoadedAmmoDeductionRatio, 0.0f, 1.0f);
	ItemInstance.LootLoadedAmmoFlatDeduction = FMath::Max(0, ItemInstance.LootLoadedAmmoFlatDeduction);
	if (ItemInstance.LoadedAmmoItemId != INDEX_NONE)
	{
		ItemInstance.SelectedAmmoItemId = ItemInstance.LoadedAmmoItemId;
	}
	else
	{
		ItemInstance.LoadedAmmoCount = 0;
	}

	ItemInstancesByUid.Add(ItemInstance.Uid, ItemInstance);
	return ItemInstance.Uid;
}

void UTunaSweeperGameInstance::NotifyActiveLootContainerUiClosed()
{
	if (bHasActiveLootContainer)
	{
		OnActiveLootContainerUiClosed.Broadcast();
	}
}

void UTunaSweeperGameInstance::SaveGameState()
{
	EnsureInventoryStateInitialized();
	const EUsableQuickSlotSaveMode SaveMode = bPendingBunkerItemStateSave
		? EUsableQuickSlotSaveMode::PersistRuntime
		: EUsableQuickSlotSaveMode::PreserveExisting;
	if (SaveGameStateInternal(SaveMode))
	{
		bPendingBunkerItemStateSave = false;
	}
}

void UTunaSweeperGameInstance::MarkBunkerItemStateSavePending()
{
	if (IsCurrentWorldBunkerMap())
	{
		bPendingBunkerItemStateSave = true;
	}
}

bool UTunaSweeperGameInstance::FlushPendingBunkerItemStateSave()
{
	if (!bPendingBunkerItemStateSave)
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	if (!SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime))
	{
		return false;
	}

	bPendingBunkerItemStateSave = false;
	return true;
}

void UTunaSweeperGameInstance::MarkItemStateMutationForSave(bool bSaveImmediatelyOutsideBunker)
{
	if (IsCurrentWorldBunkerMap())
	{
		bPendingBunkerItemStateSave = true;
		return;
	}

	if (bSaveImmediatelyOutsideBunker)
	{
		SaveGameStateInternal();
	}
}

void UTunaSweeperGameInstance::ClearInventoryAndSave()
{
	EnsureInventoryStateInitialized();
	ClearSelectedItemSelection();
	ClearHoveredItemSlot();

	TSet<FGuid> StorageItemUids;
	CollectItemUidsFromSlots(StorageSlots, StorageItemUids);
	TMap<FGuid, FTunaSweeperItemInstance> PreservedStorageItemInstances;
	for (const FGuid& StorageItemUid : StorageItemUids)
	{
		if (const FTunaSweeperItemInstance* StorageItemInstance = ItemInstancesByUid.Find(StorageItemUid))
		{
			PreservedStorageItemInstances.Add(StorageItemUid, *StorageItemInstance);
		}
	}
	ItemInstancesByUid = MoveTemp(PreservedStorageItemInstances);

	ResetPlayerSlotArrays();
	UsableQuickSlots.Reset();
	EnsureSlotArraySize(UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
	RemoveInvalidSlotReferences(StorageSlots);
	EnsureSlotArraySize(StorageSlots, StorageSlotCapacity);
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	ClearRaidExperienceGain();
	bHasPendingBunkerEntryVitals = false;
	if (SaveGameStateInternal(EUsableQuickSlotSaveMode::Clear))
	{
		bPendingBunkerItemStateSave = false;
	}
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::HandleLevelTravelPersistence(FName SourceLevelName, FName TargetLevelName)
{
	if (IsRaidToBunkerTravel(SourceLevelName, TargetLevelName))
	{
		EnsureInventoryStateInitialized();
		CaptureBunkerEntryVitalsFromPawn(GetWorld() ? UGameplayStatics::GetPlayerPawn(GetWorld(), 0) : nullptr);
		FTunaSweeperExperienceAnimationState ExperienceAnimationState;
		CommitRaidExperienceGain(ExperienceAnimationState);
		if (SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime))
		{
			bPendingBunkerItemStateSave = false;
		}
		return;
	}

	if (IsBunkerToRaidTravel(SourceLevelName, TargetLevelName))
	{
		SaveGameState();
		BeginRaidExperienceSession();
	}
}

void UTunaSweeperGameInstance::CaptureBunkerEntryVitalsFromPawn(APawn* Pawn)
{
	bHasPendingBunkerEntryVitals = false;
	PendingBunkerEntryHealthRatio = 1.0f;
	PendingBunkerEntryFoodRatio = 1.0f;
	PendingBunkerEntryHydrationRatio = 1.0f;

	const UTunaSweeperVitalsComponent* VitalsComponent = Pawn
		? Pawn->FindComponentByClass<UTunaSweeperVitalsComponent>()
		: nullptr;
	if (!VitalsComponent)
	{
		return;
	}

	const FTunaSweeperVitalsState& VitalsState = VitalsComponent->GetVitalsState();
	PendingBunkerEntryHealthRatio = VitalsState.MaxHealth > 0.0f
		? FMath::Clamp(VitalsState.Health / VitalsState.MaxHealth, 0.0f, 1.0f)
		: 1.0f;
	PendingBunkerEntryFoodRatio = VitalsState.MaxFood > 0.0f
		? FMath::Clamp(VitalsState.Food / VitalsState.MaxFood, 0.0f, 1.0f)
		: 1.0f;
	PendingBunkerEntryHydrationRatio = VitalsState.MaxHydration > 0.0f
		? FMath::Clamp(VitalsState.Hydration / VitalsState.MaxHydration, 0.0f, 1.0f)
		: 1.0f;
	bHasPendingBunkerEntryVitals = true;
}

bool UTunaSweeperGameInstance::ConsumePendingBunkerEntryVitals(UTunaSweeperVitalsComponent* VitalsComponent)
{
	if (!bHasPendingBunkerEntryVitals || !VitalsComponent)
	{
		return false;
	}

	FTunaSweeperVitalsState BunkerEntryVitals = VitalsComponent->GetVitalsState();
	BunkerEntryVitals.Normalize();
	BunkerEntryVitals.Health = BunkerEntryVitals.MaxHealth * FMath::Clamp(PendingBunkerEntryHealthRatio, 0.0f, 1.0f);
	BunkerEntryVitals.Food = BunkerEntryVitals.MaxFood * FMath::Max(0.5f, FMath::Clamp(PendingBunkerEntryFoodRatio, 0.0f, 1.0f));
	BunkerEntryVitals.Hydration = BunkerEntryVitals.MaxHydration * FMath::Max(
		0.5f,
		FMath::Clamp(PendingBunkerEntryHydrationRatio, 0.0f, 1.0f));
	BunkerEntryVitals.Normalize();
	VitalsComponent->SetVitalsState(BunkerEntryVitals);

	bHasPendingBunkerEntryVitals = false;
	PendingBunkerEntryHealthRatio = 1.0f;
	PendingBunkerEntryFoodRatio = 1.0f;
	PendingBunkerEntryHydrationRatio = 1.0f;
	return true;
}

