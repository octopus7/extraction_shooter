#include "TunaSweeperGameInstanceShared.h"

bool UTunaSweeperGameInstance::IsEquipmentWeaponSlotOccupied(int32 WeaponSlotNumber)
{
	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	return TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition);
}

bool UTunaSweeperGameInstance::TryGetEquipmentWeaponSlotItem(
	int32 WeaponSlotNumber,
	FTunaSweeperItemInstance& OutItemInstance,
	FTunaSweeperItemDefinition& OutItemDefinition)
{
	EnsureInventoryStateInitialized();

	OutItemInstance = FTunaSweeperItemInstance();
	OutItemDefinition = FTunaSweeperItemDefinition();

	const int32 EquipmentSlotIndex = GetEquipmentSlotIndexForWeaponSlotNumber(WeaponSlotNumber);
	if (!EquipmentSlots.IsValidIndex(EquipmentSlotIndex))
	{
		return false;
	}

	const FGuid& WeaponUid = EquipmentSlots[EquipmentSlotIndex].ItemUid;
	if (!TryGetItemInstance(WeaponUid, OutItemInstance))
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	return ItemDataSubsystem &&
		ItemDataSubsystem->TryGetItemDefinition(OutItemInstance.ItemId, OutItemDefinition) &&
		IsGunItemDefinition(OutItemDefinition);
}

bool UTunaSweeperGameInstance::IsEquipmentMeleeSlotOccupied()
{
	FTunaSweeperItemInstance MeleeInstance;
	FTunaSweeperItemDefinition MeleeDefinition;
	return TryGetEquipmentMeleeSlotItem(MeleeInstance, MeleeDefinition);
}

bool UTunaSweeperGameInstance::TryGetEquipmentMeleeSlotItem(
	FTunaSweeperItemInstance& OutItemInstance,
	FTunaSweeperItemDefinition& OutItemDefinition)
{
	EnsureInventoryStateInitialized();

	OutItemInstance = FTunaSweeperItemInstance();
	OutItemDefinition = FTunaSweeperItemDefinition();

	if (!EquipmentSlots.IsValidIndex(TunaSweeperInventory::MeleeEquipmentSlotIndex))
	{
		return false;
	}

	const FGuid& MeleeUid = EquipmentSlots[TunaSweeperInventory::MeleeEquipmentSlotIndex].ItemUid;
	if (!TryGetItemInstance(MeleeUid, OutItemInstance))
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	return ItemDataSubsystem &&
		ItemDataSubsystem->TryGetItemDefinition(OutItemInstance.ItemId, OutItemDefinition) &&
		IsMeleeItemDefinition(OutItemDefinition);
}

void UTunaSweeperGameInstance::SetRuntimeSelectedWeaponSlotNumber(int32 WeaponSlotNumber)
{
	RuntimeSelectedWeaponSlotNumber = FMath::Clamp(WeaponSlotNumber, 1, 2);
	bRuntimeSelectedMeleeWeapon = false;
	bHasRuntimeSelectedWeaponSelection = true;
}

void UTunaSweeperGameInstance::SetRuntimeSelectedMeleeWeapon()
{
	RuntimeSelectedWeaponSlotNumber = 0;
	bRuntimeSelectedMeleeWeapon = true;
	bHasRuntimeSelectedWeaponSelection = true;
}

bool UTunaSweeperGameInstance::TryGetRuntimeSelectedWeaponSelection(
	bool& bOutMeleeWeaponSelected,
	int32& OutWeaponSlotNumber) const
{
	if (!bHasRuntimeSelectedWeaponSelection)
	{
		bOutMeleeWeaponSelected = false;
		OutWeaponSlotNumber = 1;
		return false;
	}

	bOutMeleeWeaponSelected = bRuntimeSelectedMeleeWeapon;
	OutWeaponSlotNumber = RuntimeSelectedWeaponSlotNumber;
	return true;
}

int32 UTunaSweeperGameInstance::GetWeaponLoadedAmmoCount(int32 WeaponSlotNumber)
{
	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	return TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition)
		? FMath::Clamp(WeaponInstance.LoadedAmmoCount, 0, CalculateWeaponMagazineCapacity(WeaponInstance, WeaponDefinition))
		: 0;
}

int32 UTunaSweeperGameInstance::GetWeaponMagazineCapacity(int32 WeaponSlotNumber)
{
	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	return TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition)
		? CalculateWeaponMagazineCapacity(WeaponInstance, WeaponDefinition)
		: 0;
}

int32 UTunaSweeperGameInstance::GetWeaponInventoryAmmoCount(int32 WeaponSlotNumber)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return 0;
	}

	FTunaSweeperItemInstance* MutableWeaponInstance = ItemInstancesByUid.Find(WeaponInstance.Uid);
	if (!MutableWeaponInstance)
	{
		return 0;
	}

	const int32 AmmoItemId = MutableWeaponInstance->LoadedAmmoItemId;
	if (AmmoItemId == INDEX_NONE)
	{
		return 0;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition AmmoDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(AmmoItemId, AmmoDefinition) ||
		!IsAmmoDefinitionCompatibleWithWeapon(WeaponDefinition, AmmoDefinition))
	{
		return 0;
	}

	return CountInventoryAmmoByItemId(AmmoItemId);
}

int32 UTunaSweeperGameInstance::GetWeaponSelectedAmmoItemId(int32 WeaponSlotNumber)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return INDEX_NONE;
	}

	const FTunaSweeperItemInstance* WeaponInstanceState = ItemInstancesByUid.Find(WeaponInstance.Uid);
	if (!WeaponInstanceState)
	{
		return INDEX_NONE;
	}

	const int32 SelectedAmmoItemId = WeaponInstanceState->LoadedAmmoItemId;
	if (SelectedAmmoItemId == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition AmmoDefinition;
	return ItemDataSubsystem &&
		ItemDataSubsystem->TryGetItemDefinition(SelectedAmmoItemId, AmmoDefinition) &&
		IsAmmoDefinitionCompatibleWithWeapon(WeaponDefinition, AmmoDefinition)
			? SelectedAmmoItemId
			: INDEX_NONE;
}

float UTunaSweeperGameInstance::GetWeaponReloadSeconds(int32 WeaponSlotNumber)
{
	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return TunaSweeperInventory::DefaultWeaponReloadSeconds;
	}

	return WeaponDefinition.ReloadSeconds > 0.0f
		? WeaponDefinition.ReloadSeconds
		: TunaSweeperInventory::DefaultWeaponReloadSeconds;
}

void UTunaSweeperGameInstance::GetCompatibleAmmoItemIdsForWeaponSlot(
	int32 WeaponSlotNumber,
	TArray<int32>& OutAmmoItemIds,
	bool bRequireInventoryAmmo)
{
	OutAmmoItemIds.Reset();

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperItemDefinition> ItemDefinitions;
	if (!ItemDataSubsystem || !ItemDataSubsystem->GetAllItemDefinitions(ItemDefinitions))
	{
		return;
	}

	for (const FTunaSweeperItemDefinition& ItemDefinition : ItemDefinitions)
	{
		if (IsAmmoDefinitionCompatibleWithWeapon(WeaponDefinition, ItemDefinition) &&
			(!bRequireInventoryAmmo || CountInventoryAmmoByItemId(ItemDefinition.Id) > 0))
		{
			OutAmmoItemIds.Add(ItemDefinition.Id);
		}
	}
}

bool UTunaSweeperGameInstance::SetSelectedAmmoItemForWeaponSlot(int32 WeaponSlotNumber, int32 AmmoItemId)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition AmmoDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(AmmoItemId, AmmoDefinition) ||
		!IsAmmoDefinitionCompatibleWithWeapon(WeaponDefinition, AmmoDefinition))
	{
		return false;
	}

	FTunaSweeperItemInstance* MutableWeaponInstance = ItemInstancesByUid.Find(WeaponInstance.Uid);
	if (!MutableWeaponInstance)
	{
		return false;
	}

	if (MutableWeaponInstance->LoadedAmmoItemId == AmmoItemId &&
		MutableWeaponInstance->SelectedAmmoItemId == AmmoItemId)
	{
		return true;
	}

	MutableWeaponInstance->LoadedAmmoItemId = AmmoItemId;
	MutableWeaponInstance->SelectedAmmoItemId = AmmoItemId;
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::TryConsumeLoadedAmmoForWeaponSlot(int32 WeaponSlotNumber)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return false;
	}

	FTunaSweeperItemInstance* MutableWeaponInstance = ItemInstancesByUid.Find(WeaponInstance.Uid);
	if (!MutableWeaponInstance)
	{
		return false;
	}

	const int32 MagazineCapacity = CalculateWeaponMagazineCapacity(*MutableWeaponInstance, WeaponDefinition);
	MutableWeaponInstance->LoadedAmmoCount = FMath::Clamp(MutableWeaponInstance->LoadedAmmoCount, 0, MagazineCapacity);
	if (MutableWeaponInstance->LoadedAmmoCount <= 0)
	{
		return false;
	}

	MutableWeaponInstance->LoadedAmmoCount = FMath::Max(0, MutableWeaponInstance->LoadedAmmoCount - 1);
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::TryReloadWeaponSlot(int32 WeaponSlotNumber, int32 AmmoItemId, int32& OutLoadedAmmoCount)
{
	EnsureInventoryStateInitialized();
	OutLoadedAmmoCount = 0;

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TryGetEquipmentWeaponSlotItem(WeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return false;
	}

	FTunaSweeperItemInstance* MutableWeaponInstance = ItemInstancesByUid.Find(WeaponInstance.Uid);
	if (!MutableWeaponInstance)
	{
		return false;
	}

	const int32 MagazineCapacity = CalculateWeaponMagazineCapacity(*MutableWeaponInstance, WeaponDefinition);
	MutableWeaponInstance->LoadedAmmoCount = FMath::Clamp(MutableWeaponInstance->LoadedAmmoCount, 0, MagazineCapacity);
	if (MagazineCapacity <= 0 || MutableWeaponInstance->LoadedAmmoCount >= MagazineCapacity)
	{
		return false;
	}

	const int32 ExistingLoadedAmmoItemId = MutableWeaponInstance->LoadedAmmoCount > 0
		? MutableWeaponInstance->LoadedAmmoItemId
		: INDEX_NONE;
	int32 ReloadAmmoItemId = ExistingLoadedAmmoItemId != INDEX_NONE
		? ExistingLoadedAmmoItemId
		: AmmoItemId;
	if (ReloadAmmoItemId == INDEX_NONE)
	{
		ReloadAmmoItemId = MutableWeaponInstance->LoadedAmmoItemId;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition AmmoDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(ReloadAmmoItemId, AmmoDefinition) ||
		!IsAmmoDefinitionCompatibleWithWeapon(WeaponDefinition, AmmoDefinition))
	{
		return false;
	}

	const int32 RequestedAmmo = MagazineCapacity - MutableWeaponInstance->LoadedAmmoCount;
	const int32 ConsumedAmmo = ConsumeInventoryAmmoByItemId(ReloadAmmoItemId, RequestedAmmo);
	if (ConsumedAmmo <= 0)
	{
		return false;
	}

	MutableWeaponInstance->LoadedAmmoItemId = ReloadAmmoItemId;
	MutableWeaponInstance->SelectedAmmoItemId = ReloadAmmoItemId;
	MutableWeaponInstance->LoadedAmmoCount = FMath::Clamp(
		MutableWeaponInstance->LoadedAmmoCount + ConsumedAmmo,
		0,
		MagazineCapacity);
	OutLoadedAmmoCount = MutableWeaponInstance->LoadedAmmoCount;
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave();
	return true;
}

bool UTunaSweeperGameInstance::IsSellableItemSlotSource(ETunaSweeperItemSlotSource Source) const
{
	return Source == ETunaSweeperItemSlotSource::Inventory ||
		Source == ETunaSweeperItemSlotSource::AuxiliaryBag ||
		Source == ETunaSweeperItemSlotSource::UsableQuickSlot;
}

TArray<FTunaSweeperInventorySlot>* UTunaSweeperGameInstance::GetMutableSlotsForSource(ETunaSweeperItemSlotSource Source)
{
	switch (Source)
	{
	case ETunaSweeperItemSlotSource::Equipment:
		return &EquipmentSlots;
	case ETunaSweeperItemSlotSource::AuxiliaryBag:
		return &AuxiliaryBagSlots;
	case ETunaSweeperItemSlotSource::Inventory:
		return &PlayerInventorySlots;
	case ETunaSweeperItemSlotSource::UsableQuickSlot:
		return &UsableQuickSlots;
	case ETunaSweeperItemSlotSource::Storage:
		return &StorageSlots;
	case ETunaSweeperItemSlotSource::LootContainer:
		return bHasActiveLootContainer ? &ActiveLootContainerSlots : nullptr;
	case ETunaSweeperItemSlotSource::SelectedWeaponAttachment:
		return &SelectedWeaponAttachmentSlots;
	default:
		return nullptr;
	}
}

const TArray<FTunaSweeperInventorySlot>* UTunaSweeperGameInstance::GetSlotsForSource(ETunaSweeperItemSlotSource Source) const
{
	switch (Source)
	{
	case ETunaSweeperItemSlotSource::Equipment:
		return &EquipmentSlots;
	case ETunaSweeperItemSlotSource::AuxiliaryBag:
		return &AuxiliaryBagSlots;
	case ETunaSweeperItemSlotSource::Inventory:
		return &PlayerInventorySlots;
	case ETunaSweeperItemSlotSource::UsableQuickSlot:
		return &UsableQuickSlots;
	case ETunaSweeperItemSlotSource::Storage:
		return &StorageSlots;
	case ETunaSweeperItemSlotSource::LootContainer:
		return bHasActiveLootContainer ? &ActiveLootContainerSlots : nullptr;
	case ETunaSweeperItemSlotSource::SelectedWeaponAttachment:
		return &SelectedWeaponAttachmentSlots;
	default:
		return nullptr;
	}
}

int32 UTunaSweeperGameInstance::CalculateInventoryCapacityForEquipmentSlots(
	const TArray<FTunaSweeperInventorySlot>& InEquipmentSlots)
{
	const int32 BareSlots = FMath::Max(TunaSweeperInventory::RequiredBareInventorySlots, GameplaySettings.BareInventorySlots);
	const int32 MaxSlots = FMath::Max(BareSlots, FMath::Max(TunaSweeperInventory::RequiredMaxInventorySlots, GameplaySettings.MaxInventorySlots));
	int32 Capacity = BareSlots;

	if (InEquipmentSlots.IsValidIndex(TunaSweeperInventory::BackpackSlotIndex))
	{
		const int32 BackpackCapacity = GetInventoryCapacityForItemUid(InEquipmentSlots[TunaSweeperInventory::BackpackSlotIndex].ItemUid);
		if (BackpackCapacity > BareSlots)
		{
			Capacity = BareSlots + (BackpackCapacity - BareSlots);
		}
	}

	return TunaSweeperInventory::ClampSlotCount(Capacity, BareSlots, MaxSlots);
}

int32 UTunaSweeperGameInstance::GetInventoryCapacityForItemUid(const FGuid& ItemUid)
{
	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance)
	{
		return 0;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition))
	{
		return 0;
	}

	return IsBackpackItemDefinition(ItemDefinition)
		? FMath::Max(0, ItemDefinition.InventorySlotCapacity)
		: 0;
}

bool UTunaSweeperGameInstance::IsItemCompatibleWithEquipmentSlot(int32 SlotIndex, const FGuid& ItemUid)
{
	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	return ItemDataSubsystem &&
		ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition) &&
		DoesItemDefinitionMatchEquipmentSlot(SlotIndex, ItemDefinition);
}

bool UTunaSweeperGameInstance::DoesItemDefinitionMatchEquipmentSlot(
	int32 SlotIndex,
	const FTunaSweeperItemDefinition& ItemDefinition) const
{
	const TunaSweeperInventory::FEquipmentSlotRule* Rule = TunaSweeperInventory::GetEquipmentSlotRule(SlotIndex);
	if (!Rule)
	{
		return false;
	}

	return ItemDefinition.EquipmentSlotTag == Rule->EquipmentSlotTag ||
		ItemDefinition.CategoryTag == Rule->CategoryTag ||
		(SlotIndex == TunaSweeperInventory::BackpackSlotIndex && IsBackpackItemDefinition(ItemDefinition));
}

bool UTunaSweeperGameInstance::IsItemCompatibleWithUsableQuickSlot(const FGuid& ItemUid)
{
	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	return ItemDataSubsystem &&
		ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition) &&
		IsUsableQuickSlotItemDefinition(ItemDefinition);
}

bool UTunaSweeperGameInstance::IsUsableQuickSlotItemDefinition(
	const FTunaSweeperItemDefinition& ItemDefinition) const
{
	return ItemDefinition.CategoryTag == TunaSweeperInventory::ConsumableCategoryTag ||
		ItemDefinition.CategoryTag == TunaSweeperInventory::ThrowableCategoryTag;
}

bool UTunaSweeperGameInstance::DoesItemDefinitionHaveUseEffect(
	const FTunaSweeperItemDefinition& ItemDefinition) const
{
	return !FMath::IsNearlyZero(ItemDefinition.UseHealthDelta) ||
		!FMath::IsNearlyZero(ItemDefinition.UseFoodDelta) ||
		!FMath::IsNearlyZero(ItemDefinition.UseHydrationDelta) ||
		ItemDefinition.ClearsDebuffIds.Num() > 0;
}

bool UTunaSweeperGameInstance::TryResolveItemAttachmentDrop(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot,
	FName& OutAttachmentSlotTag,
	FGuid& OutExistingAttachmentUid)
{
	OutAttachmentSlotTag = NAME_None;
	OutExistingAttachmentUid.Invalidate();

	const TArray<FTunaSweeperInventorySlot>* SourceSlots = GetSlotsForSource(SourceSlot.Source);
	const TArray<FTunaSweeperInventorySlot>* TargetSlots = GetSlotsForSource(TargetSlot.Source);
	if (!SourceSlots || !TargetSlots ||
		!SourceSlots->IsValidIndex(SourceSlot.SlotIndex) ||
		!TargetSlots->IsValidIndex(TargetSlot.SlotIndex))
	{
		return false;
	}

	const FGuid SourceUid = (*SourceSlots)[SourceSlot.SlotIndex].ItemUid;
	const FGuid TargetUid = (*TargetSlots)[TargetSlot.SlotIndex].ItemUid;
	if (!SourceUid.IsValid() || !TargetUid.IsValid() || SourceUid == TargetUid)
	{
		return false;
	}

	const FTunaSweeperItemInstance* SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
	const FTunaSweeperItemInstance* TargetItemInstance = ItemInstancesByUid.Find(TargetUid);
	if (!SourceItemInstance || !TargetItemInstance)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition SourceItemDefinition;
	FTunaSweeperItemDefinition TargetItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(SourceItemInstance->ItemId, SourceItemDefinition) ||
		!ItemDataSubsystem->TryGetItemDefinition(TargetItemInstance->ItemId, TargetItemDefinition) ||
		!DoesItemDefinitionAcceptAttachment(TargetItemDefinition, SourceItemDefinition))
	{
		return false;
	}

	OutAttachmentSlotTag = SourceItemDefinition.AttachmentSlotTag;
	if (const FGuid* ExistingAttachmentUid = TargetItemInstance->AttachmentSlots.Find(OutAttachmentSlotTag))
	{
		OutExistingAttachmentUid = *ExistingAttachmentUid;
	}
	return true;
}

bool UTunaSweeperGameInstance::ApplyItemAttachmentDrop(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot,
	FName AttachmentSlotTag,
	const FGuid& ExistingAttachmentUid)
{
	if (AttachmentSlotTag.IsNone())
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
	if (!SourceUid.IsValid() || !TargetUid.IsValid() || ExistingAttachmentUid == SourceUid)
	{
		return false;
	}

	FTunaSweeperItemInstance* TargetItemInstance = ItemInstancesByUid.Find(TargetUid);
	if (!TargetItemInstance)
	{
		return false;
	}

	TargetItemInstance->AttachmentSlots.Add(AttachmentSlotTag, SourceUid);
	if (ExistingAttachmentUid.IsValid())
	{
		(*SourceSlots)[SourceSlot.SlotIndex].ItemUid = ExistingAttachmentUid;
	}
	else
	{
		(*SourceSlots)[SourceSlot.SlotIndex].Clear();
	}

	if (SourceSlot.Source == ETunaSweeperItemSlotSource::Inventory &&
		SourceSlots->IsValidIndex(SourceSlot.SlotIndex) &&
		(*SourceSlots)[SourceSlot.SlotIndex].IsEmpty())
	{
		(*SourceSlots)[SourceSlot.SlotIndex].bSortLocked = false;
	}

	if (SourceSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
	{
		CommitSelectedWeaponAttachmentSlotsToSelectedItem();
	}

	ClearSelectedItemIfInvalid();
	return true;
}

bool UTunaSweeperGameInstance::DoesItemDefinitionAcceptAttachment(
	const FTunaSweeperItemDefinition& ItemDefinition,
	const FTunaSweeperItemDefinition& AttachmentDefinition) const
{
	if (AttachmentDefinition.AttachmentSlotTag.IsNone() ||
		!ItemDefinition.AttachmentSlotTags.Contains(AttachmentDefinition.AttachmentSlotTag))
	{
		return false;
	}

	return AttachmentDefinition.CompatibleWeaponTypeTags.Num() <= 0 ||
		AttachmentDefinition.CompatibleWeaponTypeTags.Contains(ItemDefinition.WeaponTypeTag);
}

bool UTunaSweeperGameInstance::IsBackpackItemUid(const FGuid& ItemUid)
{
	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	return ItemDataSubsystem &&
		ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition) &&
		IsBackpackItemDefinition(ItemDefinition);
}

bool UTunaSweeperGameInstance::IsBackpackItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const
{
	return ItemDefinition.CategoryTag == TunaSweeperInventory::BackpackCategoryTag ||
		ItemDefinition.EquipmentSlotTag == TunaSweeperInventory::BackpackEquipmentSlotTag ||
		ItemDefinition.InventorySlotCapacity > FMath::Max(TunaSweeperInventory::RequiredBareInventorySlots, GameplaySettings.BareInventorySlots);
}

float UTunaSweeperGameInstance::GetEquippedBackpackCarryStrengthBonus() const
{
	if (!EquipmentSlots.IsValidIndex(TunaSweeperInventory::BackpackSlotIndex))
	{
		return 0.0f;
	}

	const FTunaSweeperItemInstance* BackpackInstance =
		ItemInstancesByUid.Find(EquipmentSlots[TunaSweeperInventory::BackpackSlotIndex].ItemUid);
	if (!BackpackInstance)
	{
		return 0.0f;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition BackpackDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(BackpackInstance->ItemId, BackpackDefinition) ||
		!IsBackpackItemDefinition(BackpackDefinition))
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, BackpackDefinition.CarryStrengthBonus);
}

float UTunaSweeperGameInstance::CalculatePlayerCarryWeight() const
{
	TSet<FGuid> VisitedItemUids;
	float TotalWeight = 0.0f;
	auto AccumulateSlotWeights = [this, &VisitedItemUids, &TotalWeight](const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (const FTunaSweeperInventorySlot& Slot : Slots)
		{
			TotalWeight += CalculateItemInstanceCarryWeight(Slot.ItemUid, VisitedItemUids);
		}
	};

	AccumulateSlotWeights(PlayerInventorySlots);
	AccumulateSlotWeights(EquipmentSlots);
	AccumulateSlotWeights(AuxiliaryBagSlots);
	AccumulateSlotWeights(UsableQuickSlots);
	return FMath::Max(0.0f, TotalWeight);
}

float UTunaSweeperGameInstance::CalculateItemInstanceCarryWeight(
	const FGuid& ItemUid,
	TSet<FGuid>& VisitedItemUids) const
{
	if (!ItemUid.IsValid() || VisitedItemUids.Contains(ItemUid))
	{
		return 0.0f;
	}
	VisitedItemUids.Add(ItemUid);

	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance || !ItemInstance->IsValid())
	{
		return 0.0f;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	float TotalWeight = 0.0f;
	if (ItemDataSubsystem && ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, ItemDefinition))
	{
		TotalWeight += FMath::Max(0.0f, ItemDefinition.WeightKg) * FMath::Max(1, ItemInstance->Quantity);
	}

	for (const TPair<FName, FGuid>& AttachmentSlot : ItemInstance->AttachmentSlots)
	{
		TotalWeight += CalculateItemInstanceCarryWeight(AttachmentSlot.Value, VisitedItemUids);
	}

	return FMath::Max(0.0f, TotalWeight);
}

float UTunaSweeperGameInstance::CalculateMaxCarryWeight() const
{
	FTunaSweeperCarryWeightDebuffSettings CarrySettings;
	if (UTunaSweeperDebuffDataSubsystem* DebuffDataSubsystem = GetSubsystem<UTunaSweeperDebuffDataSubsystem>())
	{
		CarrySettings = DebuffDataSubsystem->GetCarryWeightSettings();
	}
	CarrySettings.Normalize();

	const FTunaSweeperExperienceLevelStatBonuses LevelBonuses = GetCurrentExperienceLevelStatBonuses();
	const float CarryStrength =
		CarrySettings.BaseStrength +
		FMath::Max(0.0f, LevelBonuses.CarryStrengthBonus) +
		GetEquippedBackpackCarryStrengthBonus();
	return FMath::Max(1.0f, CarryStrength * CarrySettings.KgPerStrength);
}

bool UTunaSweeperGameInstance::IsEquipmentWeaponSlotNumberValid(int32 WeaponSlotNumber) const
{
	return WeaponSlotNumber >= 1 && WeaponSlotNumber <= TunaSweeperInventory::WeaponEquipmentSlotCount;
}

int32 UTunaSweeperGameInstance::GetEquipmentSlotIndexForWeaponSlotNumber(int32 WeaponSlotNumber) const
{
	return IsEquipmentWeaponSlotNumberValid(WeaponSlotNumber)
		? WeaponSlotNumber - 1
		: INDEX_NONE;
}

bool UTunaSweeperGameInstance::IsGunItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const
{
	return ItemDefinition.CategoryTag == TunaSweeperInventory::GunCategoryTag ||
		ItemDefinition.EquipmentSlotTag == TunaSweeperInventory::GunEquipmentSlotTag;
}

bool UTunaSweeperGameInstance::IsMeleeItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const
{
	return ItemDefinition.CategoryTag == TunaSweeperInventory::MeleeCategoryTag ||
		ItemDefinition.EquipmentSlotTag == TunaSweeperInventory::MeleeEquipmentSlotTag;
}

bool UTunaSweeperGameInstance::IsAmmoItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const
{
	return ItemDefinition.CategoryTag == TunaSweeperInventory::AmmoCategoryTag && !ItemDefinition.AmmoTypeTag.IsNone();
}

bool UTunaSweeperGameInstance::IsAmmoDefinitionCompatibleWithWeapon(
	const FTunaSweeperItemDefinition& WeaponDefinition,
	const FTunaSweeperItemDefinition& AmmoDefinition) const
{
	if (!IsGunItemDefinition(WeaponDefinition) || !IsAmmoItemDefinition(AmmoDefinition))
	{
		return false;
	}

	if (WeaponDefinition.CompatibleAmmoTypeTags.Num() > 0)
	{
		return WeaponDefinition.CompatibleAmmoTypeTags.Contains(AmmoDefinition.AmmoTypeTag);
	}

	return TunaSweeperInventory::GetDefaultAmmoTypeTagForWeaponType(WeaponDefinition.WeaponTypeTag) == AmmoDefinition.AmmoTypeTag;
}

bool UTunaSweeperGameInstance::IsStackableItemDefinition(const FTunaSweeperItemDefinition& ItemDefinition) const
{
	const UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	return ItemDataSubsystem && ItemDataSubsystem->ResolveItemMaxStackQuantity(ItemDefinition) > 1;
}

bool UTunaSweeperGameInstance::IsStackableItemId(int32 ItemId) const
{
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	return ItemId != INDEX_NONE &&
		ItemDataSubsystem &&
		ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition) &&
		IsStackableItemDefinition(ItemDefinition);
}

bool UTunaSweeperGameInstance::DoesItemInstanceAllowStacking(const FTunaSweeperItemInstance& ItemInstance) const
{
	return ItemInstance.IsValid() &&
		ItemInstance.AttachmentSlots.IsEmpty() &&
		ItemInstance.LoadedAmmoItemId == INDEX_NONE &&
		ItemInstance.LoadedAmmoCount <= 0 &&
		ItemInstance.SelectedAmmoItemId == INDEX_NONE;
}

bool UTunaSweeperGameInstance::CanStackItemInstances(
	const FTunaSweeperItemInstance& SourceItemInstance,
	const FTunaSweeperItemInstance& TargetItemInstance) const
{
	if (!DoesItemInstanceAllowStacking(SourceItemInstance) ||
		!DoesItemInstanceAllowStacking(TargetItemInstance) ||
		SourceItemInstance.ItemId != TargetItemInstance.ItemId)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetItemDefinition(TargetItemInstance.ItemId, ItemDefinition))
	{
		return false;
	}

	const int32 MaxStackQuantity = FMath::Max(1, ItemDataSubsystem->ResolveItemMaxStackQuantity(ItemDefinition));
	return MaxStackQuantity > 1 && TargetItemInstance.Quantity < MaxStackQuantity;
}

bool UTunaSweeperGameInstance::TryFindFirstStackTargetSlot(
	const FTunaSweeperItemSlotReference& SourceSlot,
	ETunaSweeperItemSlotSource TargetSource,
	FTunaSweeperItemSlotReference& OutTargetSlot)
{
	EnsureInventoryStateInitialized();
	OutTargetSlot = FTunaSweeperItemSlotReference();

	const TArray<FTunaSweeperInventorySlot>* TargetSlots = GetSlotsForSource(TargetSource);
	if (!SourceSlot.IsValid() || !TargetSlots)
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < TargetSlots->Num(); ++SlotIndex)
	{
		if ((*TargetSlots)[SlotIndex].IsEmpty())
		{
			continue;
		}

		FTunaSweeperItemSlotReference TargetSlot;
		TargetSlot.Source = TargetSource;
		TargetSlot.SlotIndex = SlotIndex;
		if (CanStackItemBetweenSlots(SourceSlot, TargetSlot))
		{
			OutTargetSlot = TargetSlot;
			return true;
		}
	}

	return false;
}

bool UTunaSweeperGameInstance::TryMergeItemStacksBetweenSlots(
	const FTunaSweeperItemSlotReference& SourceSlot,
	const FTunaSweeperItemSlotReference& TargetSlot,
	int32& OutMergedItemId,
	int32& OutMergedQuantity)
{
	OutMergedItemId = INDEX_NONE;
	OutMergedQuantity = 0;

	if (!CanStackItemBetweenSlots(SourceSlot, TargetSlot))
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
	FTunaSweeperItemInstance* SourceItemInstance = ItemInstancesByUid.Find(SourceUid);
	FTunaSweeperItemInstance* TargetItemInstance = ItemInstancesByUid.Find(TargetUid);
	if (!SourceItemInstance || !TargetItemInstance)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetItemDefinition(TargetItemInstance->ItemId, ItemDefinition))
	{
		return false;
	}

	const int32 MaxStackQuantity = FMath::Max(1, ItemDataSubsystem->ResolveItemMaxStackQuantity(ItemDefinition));
	const int32 MergedQuantity = FMath::Min(SourceItemInstance->Quantity, MaxStackQuantity - TargetItemInstance->Quantity);
	if (MergedQuantity <= 0)
	{
		return false;
	}

	OutMergedItemId = SourceItemInstance->ItemId;
	OutMergedQuantity = MergedQuantity;
	TargetItemInstance->Quantity += MergedQuantity;
	SourceItemInstance->Quantity -= MergedQuantity;
	if (SourceItemInstance->Quantity <= 0)
	{
		ItemInstancesByUid.Remove(SourceUid);
		(*SourceSlots)[SourceSlot.SlotIndex].Clear();
		if (SourceSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
		{
			CommitSelectedWeaponAttachmentSlotsToSelectedItem();
		}
	}

	if (SourceSlot.Source == ETunaSweeperItemSlotSource::Inventory &&
		SourceSlots->IsValidIndex(SourceSlot.SlotIndex) &&
		(*SourceSlots)[SourceSlot.SlotIndex].IsEmpty())
	{
		(*SourceSlots)[SourceSlot.SlotIndex].bSortLocked = false;
	}

	return true;
}

bool UTunaSweeperGameInstance::CanGrantQuestItemRewards(const TArray<FTunaSweeperItemStack>& ItemRewards) const
{
	int32 EmptyInventorySlots = 0;
	for (const FTunaSweeperInventorySlot& InventorySlot : PlayerInventorySlots)
	{
		if (InventorySlot.IsEmpty())
		{
			++EmptyInventorySlots;
		}
	}

	TMap<int32, TArray<int32>> SimulatedStackQuantitiesByItemId;
	for (const FTunaSweeperInventorySlot& InventorySlot : PlayerInventorySlots)
	{
		const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(InventorySlot.ItemUid);
		if (ItemInstance && DoesItemInstanceAllowStacking(*ItemInstance))
		{
			SimulatedStackQuantitiesByItemId
				.FindOrAdd(ItemInstance->ItemId)
				.Add(FMath::Max(0, ItemInstance->Quantity));
		}
	}

	for (const FTunaSweeperItemStack& ItemReward : ItemRewards)
	{
		if (ItemReward.ItemId == INDEX_NONE || ItemReward.Quantity <= 0)
		{
			continue;
		}

		int32 MaxStackQuantity = 1;
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
		FTunaSweeperItemDefinition ItemDefinition;
		if (ItemDataSubsystem && ItemDataSubsystem->TryGetItemDefinition(ItemReward.ItemId, ItemDefinition))
		{
			MaxStackQuantity = FMath::Max(1, ItemDataSubsystem->ResolveItemMaxStackQuantity(ItemDefinition));
		}

		int32 RemainingQuantity = ItemReward.Quantity;
		if (MaxStackQuantity > 1)
		{
			TArray<int32>& SimulatedStackQuantities = SimulatedStackQuantitiesByItemId.FindOrAdd(ItemReward.ItemId);
			for (int32& SimulatedQuantity : SimulatedStackQuantities)
			{
				if (RemainingQuantity <= 0)
				{
					break;
				}

				if (SimulatedQuantity >= MaxStackQuantity)
				{
					continue;
				}

				const int32 AddedQuantity = FMath::Min(RemainingQuantity, MaxStackQuantity - SimulatedQuantity);
				SimulatedQuantity += AddedQuantity;
				RemainingQuantity -= AddedQuantity;
			}
		}

		const int32 RequiredInventorySlots = FMath::DivideAndRoundUp(RemainingQuantity, MaxStackQuantity);
		if (RequiredInventorySlots > EmptyInventorySlots)
		{
			return false;
		}
		EmptyInventorySlots -= RequiredInventorySlots;

		if (RequiredInventorySlots > 0 && MaxStackQuantity > 1)
		{
			TArray<int32>& SimulatedStackQuantities = SimulatedStackQuantitiesByItemId.FindOrAdd(ItemReward.ItemId);
			int32 QuantityInNewStacks = RemainingQuantity;
			for (int32 StackIndex = 0; StackIndex < RequiredInventorySlots; ++StackIndex)
			{
				const int32 NewStackQuantity = FMath::Min(QuantityInNewStacks, MaxStackQuantity);
				SimulatedStackQuantities.Add(NewStackQuantity);
				QuantityInNewStacks -= NewStackQuantity;
			}
		}
	}

	return true;
}

int32 UTunaSweeperGameInstance::CalculateWeaponMagazineCapacity(
	const FTunaSweeperItemInstance& WeaponInstance,
	const FTunaSweeperItemDefinition& WeaponDefinition) const
{
	int32 MagazineCapacity = WeaponDefinition.MagazineCapacity > 0
		? WeaponDefinition.MagazineCapacity
		: TunaSweeperInventory::DefaultWeaponMagazineCapacity;

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (ItemDataSubsystem)
	{
		for (const TPair<FName, FGuid>& AttachmentSlot : WeaponInstance.AttachmentSlots)
		{
			const FTunaSweeperItemInstance* AttachmentInstance = ItemInstancesByUid.Find(AttachmentSlot.Value);
			if (!AttachmentInstance)
			{
				continue;
			}

			FTunaSweeperItemDefinition AttachmentDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(AttachmentInstance->ItemId, AttachmentDefinition))
			{
				MagazineCapacity += FMath::Max(0, AttachmentDefinition.MagazineCapacityBonus);
			}
		}
	}

	return FMath::Max(1, MagazineCapacity);
}

int32 UTunaSweeperGameInstance::ResolveSelectedAmmoItemIdForWeapon(
	FTunaSweeperItemInstance& WeaponInstance,
	const FTunaSweeperItemDefinition& WeaponDefinition)
{
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem)
	{
		return INDEX_NONE;
	}

	auto IsCompatibleAmmoItemId = [this, ItemDataSubsystem, &WeaponDefinition](int32 AmmoItemId)
	{
		FTunaSweeperItemDefinition AmmoDefinition;
		return ItemDataSubsystem->TryGetItemDefinition(AmmoItemId, AmmoDefinition) &&
			IsAmmoDefinitionCompatibleWithWeapon(WeaponDefinition, AmmoDefinition);
	};

	if (WeaponInstance.LoadedAmmoItemId != INDEX_NONE && IsCompatibleAmmoItemId(WeaponInstance.LoadedAmmoItemId))
	{
		WeaponInstance.SelectedAmmoItemId = WeaponInstance.LoadedAmmoItemId;
		return WeaponInstance.LoadedAmmoItemId;
	}

	if (WeaponInstance.SelectedAmmoItemId != INDEX_NONE && IsCompatibleAmmoItemId(WeaponInstance.SelectedAmmoItemId))
	{
		WeaponInstance.LoadedAmmoItemId = WeaponInstance.SelectedAmmoItemId;
		return WeaponInstance.LoadedAmmoItemId;
	}

	WeaponInstance.SelectedAmmoItemId = INDEX_NONE;
	return INDEX_NONE;
}

int32 UTunaSweeperGameInstance::CountInventoryAmmoByItemId(int32 AmmoItemId) const
{
	if (AmmoItemId == INDEX_NONE)
	{
		return 0;
	}

	int32 AmmoCount = 0;
	auto CountAmmoInSlots = [this, AmmoItemId, &AmmoCount](const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (const FTunaSweeperInventorySlot& Slot : Slots)
		{
			const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
			if (ItemInstance && ItemInstance->ItemId == AmmoItemId)
			{
				AmmoCount += FMath::Max(0, ItemInstance->Quantity);
			}
		}
	};

	CountAmmoInSlots(PlayerInventorySlots);
	CountAmmoInSlots(AuxiliaryBagSlots);
	return AmmoCount;
}

int32 UTunaSweeperGameInstance::ConsumeInventoryAmmoByItemId(int32 AmmoItemId, int32 RequestedAmount)
{
	if (AmmoItemId == INDEX_NONE || RequestedAmount <= 0)
	{
		return 0;
	}

	int32 RemainingAmount = RequestedAmount;
	auto ConsumeAmmoInSlots = [this, AmmoItemId, &RemainingAmount](TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (FTunaSweeperInventorySlot& Slot : Slots)
		{
			if (RemainingAmount <= 0)
			{
				break;
			}

			FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
			if (!ItemInstance || ItemInstance->ItemId != AmmoItemId)
			{
				continue;
			}

			const int32 ConsumedAmount = FMath::Min(RemainingAmount, FMath::Max(0, ItemInstance->Quantity));
			ItemInstance->Quantity -= ConsumedAmount;
			RemainingAmount -= ConsumedAmount;

			if (ItemInstance->Quantity <= 0)
			{
				ItemInstancesByUid.Remove(Slot.ItemUid);
				Slot.Clear();
			}
		}
	};

	ConsumeAmmoInSlots(PlayerInventorySlots);
	ConsumeAmmoInSlots(AuxiliaryBagSlots);
	return RequestedAmount - RemainingAmount;
}

void UTunaSweeperGameInstance::MigrateLegacyEquipmentSlots()
{
	if (EquipmentSlots.IsValidIndex(0) &&
		EquipmentSlots.IsValidIndex(TunaSweeperInventory::BackpackSlotIndex) &&
		EquipmentSlots[0].ItemUid.IsValid() &&
		!EquipmentSlots[TunaSweeperInventory::BackpackSlotIndex].ItemUid.IsValid() &&
		IsBackpackItemUid(EquipmentSlots[0].ItemUid))
	{
		EquipmentSlots[TunaSweeperInventory::BackpackSlotIndex].ItemUid = EquipmentSlots[0].ItemUid;
		EquipmentSlots[0].Clear();
	}
}

void UTunaSweeperGameInstance::RefreshSelectedWeaponAttachmentSlots()
{
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();

	FTunaSweeperItemInstance SelectedItemInstance;
	FTunaSweeperItemDefinition SelectedItemDefinition;
	if (!TryGetSelectedItemInstance(SelectedItemInstance) ||
		!TryGetSelectedItemDefinition(SelectedItemDefinition) ||
		SelectedItemDefinition.AttachmentSlotTags.Num() <= 0)
	{
		return;
	}

	for (const FName& AttachmentSlotTag : SelectedItemDefinition.AttachmentSlotTags)
	{
		if (AttachmentSlotTag.IsNone())
		{
			continue;
		}

		SelectedWeaponAttachmentSlotTags.Add(AttachmentSlotTag);
		FTunaSweeperInventorySlot AttachmentSlot;
		if (const FGuid* AttachmentUid = SelectedItemInstance.AttachmentSlots.Find(AttachmentSlotTag))
		{
			AttachmentSlot.ItemUid = *AttachmentUid;
		}
		SelectedWeaponAttachmentSlots.Add(AttachmentSlot);
	}
}

bool UTunaSweeperGameInstance::CommitSelectedWeaponAttachmentSlotsToSelectedItem()
{
	FTunaSweeperItemInstance SelectedItemInstance;
	if (!TryGetSelectedItemInstance(SelectedItemInstance))
	{
		return false;
	}

	FTunaSweeperItemInstance* MutableSelectedItemInstance = ItemInstancesByUid.Find(SelectedItemInstance.Uid);
	if (!MutableSelectedItemInstance)
	{
		return false;
	}

	MutableSelectedItemInstance->AttachmentSlots.Reset();
	for (int32 SlotIndex = 0; SlotIndex < SelectedWeaponAttachmentSlotTags.Num(); ++SlotIndex)
	{
		if (!SelectedWeaponAttachmentSlots.IsValidIndex(SlotIndex))
		{
			continue;
		}

		const FGuid& AttachmentUid = SelectedWeaponAttachmentSlots[SlotIndex].ItemUid;
		if (AttachmentUid.IsValid())
		{
			MutableSelectedItemInstance->AttachmentSlots.Add(SelectedWeaponAttachmentSlotTags[SlotIndex], AttachmentUid);
		}
	}

	return true;
}

bool UTunaSweeperGameInstance::DoesSelectedWeaponAcceptAttachmentSlot(FName AttachmentSlotTag) const
{
	return SelectedWeaponAttachmentSlotTags.Contains(AttachmentSlotTag);
}

bool UTunaSweeperGameInstance::IsItemCompatibleWithSelectedWeaponAttachmentSlot(int32 SlotIndex, const FGuid& ItemUid)
{
	if (!SelectedWeaponAttachmentSlotTags.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid);
	if (!ItemInstance)
	{
		return false;
	}

	FTunaSweeperItemDefinition SelectedWeaponDefinition;
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition AttachmentDefinition;
	if (!ItemDataSubsystem ||
		!TryGetSelectedItemDefinition(SelectedWeaponDefinition) ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance->ItemId, AttachmentDefinition))
	{
		return false;
	}

	const FName RequiredAttachmentSlotTag = SelectedWeaponAttachmentSlotTags[SlotIndex];
	if (AttachmentDefinition.AttachmentSlotTag != RequiredAttachmentSlotTag ||
		!DoesSelectedWeaponAcceptAttachmentSlot(RequiredAttachmentSlotTag))
	{
		return false;
	}

	return AttachmentDefinition.CompatibleWeaponTypeTags.Num() <= 0 ||
		AttachmentDefinition.CompatibleWeaponTypeTags.Contains(SelectedWeaponDefinition.WeaponTypeTag);
}

void UTunaSweeperGameInstance::ClearSelectedItemIfInvalid()
{
	FTunaSweeperItemInstance SelectedItemInstance;
	if (SelectedItemSlotReference.IsValid() && !TryGetSelectedItemInstance(SelectedItemInstance))
	{
		SelectedItemSlotReference = FTunaSweeperItemSlotReference();
		SelectedWeaponAttachmentSlotTags.Reset();
		SelectedWeaponAttachmentSlots.Reset();
		OnSelectedInventoryItemChanged.Broadcast();
		return;
	}

	RefreshSelectedWeaponAttachmentSlots();
}

bool UTunaSweeperGameInstance::HasOccupiedInventorySlotsBeyondCapacity(
	const TArray<FTunaSweeperInventorySlot>& InInventorySlots,
	int32 Capacity) const
{
	for (int32 SlotIndex = FMath::Max(0, Capacity); SlotIndex < InInventorySlots.Num(); ++SlotIndex)
	{
		if (InInventorySlots[SlotIndex].ItemUid.IsValid())
		{
			return true;
		}
	}

	return false;
}

void UTunaSweeperGameInstance::CollectItemUidsFromSlots(
	const TArray<FTunaSweeperInventorySlot>& Slots,
	TSet<FGuid>& OutItemUids) const
{
	TFunction<void(const FGuid&)> CollectItemUid = [this, &OutItemUids, &CollectItemUid](const FGuid& ItemUid)
	{
		if (!ItemUid.IsValid() || OutItemUids.Contains(ItemUid))
		{
			return;
		}

		OutItemUids.Add(ItemUid);
		if (const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid))
		{
			for (const TPair<FName, FGuid>& AttachmentSlot : ItemInstance->AttachmentSlots)
			{
				CollectItemUid(AttachmentSlot.Value);
			}
		}
	};

	for (const FTunaSweeperInventorySlot& Slot : Slots)
	{
		CollectItemUid(Slot.ItemUid);
	}
}

void UTunaSweeperGameInstance::CollectPlayerOwnedItemUids(
	TSet<FGuid>& OutItemUids,
	bool bIncludeUsableQuickSlots) const
{
	CollectItemUidsFromSlots(PlayerInventorySlots, OutItemUids);
	CollectItemUidsFromSlots(EquipmentSlots, OutItemUids);
	CollectItemUidsFromSlots(AuxiliaryBagSlots, OutItemUids);
	CollectItemUidsFromSlots(StorageSlots, OutItemUids);
	if (bIncludeUsableQuickSlots)
	{
		CollectItemUidsFromSlots(UsableQuickSlots, OutItemUids);
	}
}

