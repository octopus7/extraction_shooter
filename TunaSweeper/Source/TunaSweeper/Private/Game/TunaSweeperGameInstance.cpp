#include "Game/TunaSweeperGameInstance.h"

#include "Engine/Texture2D.h"
#include "Inventory/TunaSweeperInventorySaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace TunaSweeperInventory
{
	const TCHAR* SaveSlotName = TEXT("TunaSweeperInventoryState");
	constexpr int32 SaveUserIndex = 0;
	constexpr int32 RequiredBareInventorySlots = 40;
	constexpr int32 RequiredMaxInventorySlots = 100;
	constexpr int32 RequiredEquipmentSlots = 8;
	constexpr int32 BackpackSlotIndex = 7;
	const FName GunCategoryTag(TEXT("item.category.weapon.gun"));
	const FName GunEquipmentSlotTag(TEXT("equipment.slot.gun"));
	const FName MeleeCategoryTag(TEXT("item.category.weapon.melee"));
	const FName MeleeEquipmentSlotTag(TEXT("equipment.slot.melee"));
	const FName HeadCategoryTag(TEXT("item.category.head"));
	const FName HeadEquipmentSlotTag(TEXT("equipment.slot.head"));
	const FName BodyCategoryTag(TEXT("item.category.body"));
	const FName BodyEquipmentSlotTag(TEXT("equipment.slot.body"));
	const FName FaceCategoryTag(TEXT("item.category.face"));
	const FName FaceEquipmentSlotTag(TEXT("equipment.slot.face"));
	const FName EarCategoryTag(TEXT("item.category.ear"));
	const FName EarEquipmentSlotTag(TEXT("equipment.slot.ear"));
	const FName BackpackCategoryTag(TEXT("item.category.bag"));
	const FName BackpackEquipmentSlotTag(TEXT("equipment.slot.backpack"));
	const FName RifleWeaponTypeTag(TEXT("weapon.type.rifle"));
	const FName MagazineAttachmentSlotTag(TEXT("attachment.slot.magazine"));
	const FName OpticAttachmentSlotTag(TEXT("attachment.slot.optic"));

	struct FEquipmentSlotRule
	{
		FName CategoryTag;
		FName EquipmentSlotTag;
	};

	const FEquipmentSlotRule* GetEquipmentSlotRule(int32 SlotIndex)
	{
		static const FEquipmentSlotRule Rules[] = {
			{ GunCategoryTag, GunEquipmentSlotTag },
			{ GunCategoryTag, GunEquipmentSlotTag },
			{ MeleeCategoryTag, MeleeEquipmentSlotTag },
			{ HeadCategoryTag, HeadEquipmentSlotTag },
			{ BodyCategoryTag, BodyEquipmentSlotTag },
			{ FaceCategoryTag, FaceEquipmentSlotTag },
			{ EarCategoryTag, EarEquipmentSlotTag },
			{ BackpackCategoryTag, BackpackEquipmentSlotTag }
		};

		return SlotIndex >= 0 && SlotIndex < UE_ARRAY_COUNT(Rules) ? &Rules[SlotIndex] : nullptr;
	}

	int32 ClampSlotCount(int32 SlotCount, int32 MinSlots, int32 MaxSlots)
	{
		return FMath::Clamp(SlotCount, FMath::Max(1, MinSlots), FMath::Max(MinSlots, MaxSlots));
	}
}

namespace TunaSweeperTempOpenLoot
{
	struct FTempLootSeed
	{
		const TCHAR* DisplayName;
		const TCHAR* TexturePath;
		int32 MinQuantity;
		int32 MaxQuantity;
	};

	const FTempLootSeed TempLootSeeds[] = {
		{ TEXT("Pistol"), TEXT("/Game/UI/Icons/T_UIIcon_Pistol.T_UIIcon_Pistol"), 1, 2 },
		{ TEXT("Rifle"), TEXT("/Game/UI/Icons/T_UIIcon_Rifle.T_UIIcon_Rifle"), 1, 1 },
		{ TEXT("Shotgun"), TEXT("/Game/UI/Icons/T_UIIcon_Shotgun.T_UIIcon_Shotgun"), 1, 1 },
		{ TEXT("Pistol Ammo"), TEXT("/Game/UI/Icons/T_UIIcon_PistolAmmo.T_UIIcon_PistolAmmo"), 18, 72 },
		{ TEXT("Rifle Ammo"), TEXT("/Game/UI/Icons/T_UIIcon_RifleAmmo.T_UIIcon_RifleAmmo"), 30, 120 },
		{ TEXT("Shotgun Ammo"), TEXT("/Game/UI/Icons/T_UIIcon_ShotgunAmmo.T_UIIcon_ShotgunAmmo"), 6, 32 },
		{ TEXT("Canned Food"), TEXT("/Game/UI/Icons/T_UIIcon_CannedFood.T_UIIcon_CannedFood"), 1, 5 },
		{ TEXT("Canned Tuna"), TEXT("/Game/UI/Icons/T_UIIcon_CannedTuna.T_UIIcon_CannedTuna"), 1, 4 },
		{ TEXT("Water Bottle"), TEXT("/Game/UI/Icons/T_UIIcon_WaterBottle.T_UIIcon_WaterBottle"), 1, 4 },
		{ TEXT("Energy Bar"), TEXT("/Game/UI/Icons/T_UIIcon_EnergyBar.T_UIIcon_EnergyBar"), 1, 6 },
		{ TEXT("Bandage"), TEXT("/Game/UI/Icons/T_UIIcon_Bandage.T_UIIcon_Bandage"), 1, 8 },
		{ TEXT("First Aid Kit"), TEXT("/Game/UI/Icons/T_UIIcon_FirstAidKit.T_UIIcon_FirstAidKit"), 1, 3 },
		{ TEXT("Painkillers"), TEXT("/Game/UI/Icons/T_UIIcon_Painkillers.T_UIIcon_Painkillers"), 1, 6 },
		{ TEXT("Antibiotics"), TEXT("/Game/UI/Icons/T_UIIcon_Antibiotics.T_UIIcon_Antibiotics"), 1, 5 },
		{ TEXT("Combat Knife"), TEXT("/Game/UI/Icons/T_UIIcon_CombatKnife.T_UIIcon_CombatKnife"), 1, 1 },
		{ TEXT("Rifle Extended Magazine"), TEXT("/Game/UI/Icons/T_UIIcon_RifleExtendedMagazine.T_UIIcon_RifleExtendedMagazine"), 1, 1 },
		{ TEXT("Red Dot Optic"), TEXT("/Game/UI/Icons/T_UIIcon_RedDotOptic.T_UIIcon_RedDotOptic"), 1, 1 },
		{ TEXT("Ballistic Helmet"), TEXT("/Game/UI/Icons/T_UIIcon_BallisticHelmet.T_UIIcon_BallisticHelmet"), 1, 1 },
		{ TEXT("Body Armor"), TEXT("/Game/UI/Icons/T_UIIcon_BodyArmor.T_UIIcon_BodyArmor"), 1, 1 },
		{ TEXT("Tactical Sunglasses"), TEXT("/Game/UI/Icons/T_UIIcon_TacticalSunglasses.T_UIIcon_TacticalSunglasses"), 1, 1 },
		{ TEXT("Tactical Earphones I"), TEXT("/Game/UI/Icons/T_UIIcon_TacticalEarphones_Tier1.T_UIIcon_TacticalEarphones_Tier1"), 1, 1 },
		{ TEXT("Tactical Earphones II"), TEXT("/Game/UI/Icons/T_UIIcon_TacticalEarphones_Tier2.T_UIIcon_TacticalEarphones_Tier2"), 1, 1 },
		{ TEXT("Bag I"), TEXT("/Game/UI/Icons/T_UIIcon_Backpack_Tier1.T_UIIcon_Backpack_Tier1"), 1, 1 },
		{ TEXT("Valuables Crate"), TEXT("/Game/UI/Icons/T_UIIcon_ValuablesCrate.T_UIIcon_ValuablesCrate"), 1, 2 },
	};
}

void FTunaSweeperPlayerHudState::NormalizeWeightLimits()
{
	CurrentCarryWeight = FMath::Max(0.0f, CurrentCarryWeight);
	MaxCarryWeight = FMath::Max(1.0f, MaxCarryWeight);
	MovementBlockedWeight = FMath::Max(MovementBlockedWeight, MaxCarryWeight * 2.0f);
	Health = FMath::Clamp(Health, 0.0f, 100.0f);
	Food = FMath::Clamp(Food, 0.0f, 100.0f);
	Hydration = FMath::Clamp(Hydration, 0.0f, 100.0f);
}

bool FTunaSweeperPlayerHudState::IsCarryWeightOverLimit() const
{
	return MaxCarryWeight > 0.0f && CurrentCarryWeight >= MaxCarryWeight;
}

bool FTunaSweeperPlayerHudState::IsCarryWeightMovementBlocked() const
{
	return MovementBlockedWeight > 0.0f && CurrentCarryWeight >= MovementBlockedWeight;
}

float FTunaSweeperPlayerHudState::GetCarryWeightMovementSpeedMultiplier() const
{
	if (IsCarryWeightMovementBlocked())
	{
		return 0.0f;
	}

	return IsCarryWeightOverLimit() ? 0.5f : 1.0f;
}

void UTunaSweeperGameInstance::Init()
{
	Super::Init();
	EnsureInventoryStateInitialized();
}

void UTunaSweeperGameInstance::SetGameplayInfo(FName Key, const FString& Value)
{
	if (!Key.IsNone())
	{
		GameplayInfo.Add(Key, Value);
	}
}

bool UTunaSweeperGameInstance::TryGetGameplayInfo(FName Key, FString& OutValue) const
{
	if (const FString* FoundValue = GameplayInfo.Find(Key))
	{
		OutValue = *FoundValue;
		return true;
	}

	OutValue.Reset();
	return false;
}

void UTunaSweeperGameInstance::SetNumberSetting(FName Key, float Value)
{
	if (!Key.IsNone())
	{
		NumberSettings.Add(Key, Value);
	}
}

bool UTunaSweeperGameInstance::TryGetNumberSetting(FName Key, float& OutValue) const
{
	if (const float* FoundValue = NumberSettings.Find(Key))
	{
		OutValue = *FoundValue;
		return true;
	}

	OutValue = 0.0f;
	return false;
}

void UTunaSweeperGameInstance::SetBoolSetting(FName Key, bool bValue)
{
	if (!Key.IsNone())
	{
		BoolSettings.Add(Key, bValue);
	}
}

bool UTunaSweeperGameInstance::TryGetBoolSetting(FName Key, bool& bOutValue) const
{
	if (const bool* FoundValue = BoolSettings.Find(Key))
	{
		bOutValue = *FoundValue;
		return true;
	}

	bOutValue = false;
	return false;
}

void UTunaSweeperGameInstance::ClearRuntimeState()
{
	GameplayInfo.Reset();
	NumberSettings.Reset();
	BoolSettings.Reset();
	PlayerHudState = FTunaSweeperPlayerHudState();
	TempOpenLootItems.Reset();
	bHasGeneratedTempOpenLootItems = false;
	PlayerInventoryItems.Reset();
	bHasGeneratedPlayerInventoryItems = false;
	ItemInstancesByUid.Reset();
	PlayerInventorySlots.Reset();
	EquipmentSlots.Reset();
	AuxiliaryBagSlots.Reset();
	ActiveLootContainerSlots.Reset();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	bInventoryStateInitialized = false;
	EnsureInventoryStateInitialized();
}

void UTunaSweeperGameInstance::SetPlayerHudState(const FTunaSweeperPlayerHudState& InHudState)
{
	PlayerHudState = InHudState;
	PlayerHudState.NormalizeWeightLimits();
}

void UTunaSweeperGameInstance::SetCarryWeight(float CurrentCarryWeight, float MaxCarryWeight, float MovementBlockedWeight)
{
	PlayerHudState.CurrentCarryWeight = CurrentCarryWeight;
	PlayerHudState.MaxCarryWeight = MaxCarryWeight;
	PlayerHudState.MovementBlockedWeight = MovementBlockedWeight;
	PlayerHudState.NormalizeWeightLimits();
}

float UTunaSweeperGameInstance::GetCarryWeightMovementSpeedMultiplier() const
{
	FTunaSweeperPlayerHudState NormalizedHudState = PlayerHudState;
	NormalizedHudState.NormalizeWeightLimits();
	return NormalizedHudState.GetCarryWeightMovementSpeedMultiplier();
}

const TArray<FTunaSweeperTempOpenLootItemData>& UTunaSweeperGameInstance::GetOrCreateTempOpenLootItems()
{
	if (!bHasGeneratedTempOpenLootItems)
	{
		GenerateTempOpenLootItems();
	}

	return TempOpenLootItems;
}

void UTunaSweeperGameInstance::GetTempOpenLootItems(TArray<FTunaSweeperTempOpenLootItemData>& OutItems)
{
	OutItems = GetOrCreateTempOpenLootItems();
}

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
	TArray<FTunaSweeperInventorySlot> SimLootContainerSlots = ActiveLootContainerSlots;
	TArray<FTunaSweeperInventorySlot> SimSelectedWeaponAttachmentSlots = SelectedWeaponAttachmentSlots;

	auto GetSimSlots = [&SimInventorySlots, &SimEquipmentSlots, &SimAuxiliaryBagSlots, &SimLootContainerSlots, &SimSelectedWeaponAttachmentSlots](
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
	(*SourceSlots)[SourceSlot.SlotIndex].ItemUid = TargetUid;
	(*TargetSlots)[TargetSlot.SlotIndex].ItemUid = SourceUid;
	if (SourceSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment ||
		TargetSlot.Source == ETunaSweeperItemSlotSource::SelectedWeaponAttachment)
	{
		CommitSelectedWeaponAttachmentSlotsToSelectedItem();
	}

	const int32 NewInventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots);
	EnsureSlotArraySize(PlayerInventorySlots, NewInventoryCapacity);
	BroadcastInventoryStateChanged();
	return true;
}

bool UTunaSweeperGameInstance::AddItemToFirstAvailableInventorySlot(int32 ItemId, int32 Quantity)
{
	EnsureInventoryStateInitialized();
	if (ItemId == INDEX_NONE || Quantity <= 0)
	{
		return false;
	}

	const FGuid ItemUid = CreateItemInstance(ItemId, Quantity);
	if (!AddItemUidToFirstEmptySlot(ItemUid, PlayerInventorySlots))
	{
		ItemInstancesByUid.Remove(ItemUid);
		return false;
	}

	BroadcastInventoryStateChanged();
	return true;
}

void UTunaSweeperGameInstance::CompactInventorySlots()
{
	EnsureInventoryStateInitialized();

	TArray<FGuid> OccupiedItemUids;
	for (const FTunaSweeperInventorySlot& Slot : PlayerInventorySlots)
	{
		if (Slot.ItemUid.IsValid())
		{
			OccupiedItemUids.Add(Slot.ItemUid);
		}
	}

	for (FTunaSweeperInventorySlot& Slot : PlayerInventorySlots)
	{
		Slot.Clear();
	}

	for (int32 Index = 0; Index < OccupiedItemUids.Num() && PlayerInventorySlots.IsValidIndex(Index); ++Index)
	{
		PlayerInventorySlots[Index].ItemUid = OccupiedItemUids[Index];
	}

	BroadcastInventoryStateChanged();
}

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

void UTunaSweeperGameInstance::SetActiveLootContainerInstance(
	const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	EnsureInventoryStateInitialized();

	ActiveLootContainerDisplayName = InContainerInstance.DisplayName;
	ActiveLootContainerCapacity = FMath::Max(0, InContainerInstance.Capacity);
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

void UTunaSweeperGameInstance::SaveInventoryState()
{
	EnsureInventoryStateInitialized();
	SaveInventoryStateInternal();
}

void UTunaSweeperGameInstance::ClearInventoryAndSave()
{
	EnsureInventoryStateInitialized();
	ClearSelectedItemSelection();
	ItemInstancesByUid.Reset();
	ResetPlayerSlotArrays();
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	SaveInventoryStateInternal();
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::HandleLevelTravelPersistence(FName SourceLevelName, FName TargetLevelName)
{
	if (IsBunkerToRaidTravel(SourceLevelName, TargetLevelName) ||
		IsRaidToBunkerTravel(SourceLevelName, TargetLevelName))
	{
		SaveInventoryState();
	}
}

void UTunaSweeperGameInstance::GenerateTempOpenLootItems()
{
	TempOpenLootItems.Reset();

	FRandomStream QuantityStream(static_cast<int32>(FDateTime::Now().GetTicks() & 0x7fffffff));
	for (const TunaSweeperTempOpenLoot::FTempLootSeed& Seed : TunaSweeperTempOpenLoot::TempLootSeeds)
	{
		FTunaSweeperTempOpenLootItemData ItemData;
		ItemData.DisplayName = FText::FromString(FString(Seed.DisplayName));
		ItemData.Quantity = QuantityStream.RandRange(Seed.MinQuantity, Seed.MaxQuantity);
		ItemData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Seed.TexturePath));
		TempOpenLootItems.Add(ItemData);
	}

	bHasGeneratedTempOpenLootItems = true;
}

void UTunaSweeperGameInstance::GeneratePlayerInventoryItems()
{
	EnsureInventoryStateInitialized();
	RefreshLegacyPlayerInventoryItems();
}

void UTunaSweeperGameInstance::EnsureInventoryStateInitialized()
{
	if (bInventoryStateInitialized)
	{
		return;
	}

	if (!LoadInventoryState())
	{
		GenerateDefaultInventoryState();
	}

	bInventoryStateInitialized = true;
	RefreshLegacyPlayerInventoryItems();
}

bool UTunaSweeperGameInstance::LoadInventoryState()
{
	if (!UGameplayStatics::DoesSaveGameExist(TunaSweeperInventory::SaveSlotName, TunaSweeperInventory::SaveUserIndex))
	{
		return false;
	}

	UTunaSweeperInventorySaveGame* SaveGame = Cast<UTunaSweeperInventorySaveGame>(UGameplayStatics::LoadGameFromSlot(
		TunaSweeperInventory::SaveSlotName,
		TunaSweeperInventory::SaveUserIndex));
	if (!SaveGame)
	{
		return false;
	}

	ItemInstancesByUid.Reset();
	for (const FTunaSweeperItemInstance& ItemInstance : SaveGame->ItemInstances)
	{
		if (ItemInstance.IsValid())
		{
			ItemInstancesByUid.Add(ItemInstance.Uid, ItemInstance);
		}
	}

	PlayerInventorySlots = SaveGame->InventorySlots;
	EquipmentSlots = SaveGame->EquipmentSlots;
	AuxiliaryBagSlots = SaveGame->AuxiliaryBagSlots;
	RemoveInvalidSlotReferences(PlayerInventorySlots);
	RemoveInvalidSlotReferences(EquipmentSlots);
	RemoveInvalidSlotReferences(AuxiliaryBagSlots);

	EnsureSlotArraySize(EquipmentSlots, FMath::Max(TunaSweeperInventory::RequiredEquipmentSlots, GameplaySettings.EquipmentSlotCount));
	EnsureSlotArraySize(AuxiliaryBagSlots, FMath::Max(0, GameplaySettings.AuxiliaryBagSlotCount));
	MigrateLegacyEquipmentSlots();

	int32 InventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots);
	for (int32 SlotIndex = PlayerInventorySlots.Num() - 1; SlotIndex >= InventoryCapacity; --SlotIndex)
	{
		if (PlayerInventorySlots[SlotIndex].ItemUid.IsValid())
		{
			InventoryCapacity = FMath::Min(
				FMath::Max(TunaSweeperInventory::RequiredMaxInventorySlots, GameplaySettings.MaxInventorySlots),
				SlotIndex + 1);
			break;
		}
	}
	EnsureSlotArraySize(PlayerInventorySlots, InventoryCapacity);

	ActiveLootContainerSlots.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
	return true;
}

bool UTunaSweeperGameInstance::SaveInventoryStateInternal() const
{
	UTunaSweeperInventorySaveGame* SaveGame = Cast<UTunaSweeperInventorySaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTunaSweeperInventorySaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	TSet<FGuid> PlayerOwnedItemUids;
	CollectPlayerOwnedItemUids(PlayerOwnedItemUids);
	for (const FGuid& ItemUid : PlayerOwnedItemUids)
	{
		if (const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid))
		{
			SaveGame->ItemInstances.Add(*ItemInstance);
		}
	}

	SaveGame->InventorySlots = PlayerInventorySlots;
	SaveGame->EquipmentSlots = EquipmentSlots;
	SaveGame->AuxiliaryBagSlots = AuxiliaryBagSlots;

	return UGameplayStatics::SaveGameToSlot(
		SaveGame,
		TunaSweeperInventory::SaveSlotName,
		TunaSweeperInventory::SaveUserIndex);
}

void UTunaSweeperGameInstance::GenerateDefaultInventoryState()
{
	ItemInstancesByUid.Reset();
	ResetPlayerSlotArrays();
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();

	const TPair<int32, int32> DefaultInventoryItems[] = {
		TPair<int32, int32>(1001, 1),
		TPair<int32, int32>(1002, 1),
		TPair<int32, int32>(2001, 36),
		TPair<int32, int32>(2004, 1),
		TPair<int32, int32>(2005, 1),
		TPair<int32, int32>(3001, 2),
		TPair<int32, int32>(3002, 2),
		TPair<int32, int32>(4001, 4),
		TPair<int32, int32>(4003, 1),
		TPair<int32, int32>(1004, 1),
		TPair<int32, int32>(5001, 1),
		TPair<int32, int32>(5002, 1),
		TPair<int32, int32>(5006, 1),
		TPair<int32, int32>(5007, 1),
		TPair<int32, int32>(5008, 1)
	};

	for (const TPair<int32, int32>& InventoryItem : DefaultInventoryItems)
	{
		AddItemUidToFirstEmptySlot(
			CreateItemInstance(InventoryItem.Key, InventoryItem.Value),
			PlayerInventorySlots);
	}
}

void UTunaSweeperGameInstance::ResetPlayerSlotArrays()
{
	EquipmentSlots.Reset();
	AuxiliaryBagSlots.Reset();
	PlayerInventorySlots.Reset();
	EnsureSlotArraySize(EquipmentSlots, FMath::Max(TunaSweeperInventory::RequiredEquipmentSlots, GameplaySettings.EquipmentSlotCount));
	EnsureSlotArraySize(AuxiliaryBagSlots, FMath::Max(0, GameplaySettings.AuxiliaryBagSlotCount));
	EnsureSlotArraySize(PlayerInventorySlots, FMath::Max(TunaSweeperInventory::RequiredBareInventorySlots, GameplaySettings.BareInventorySlots));
}

void UTunaSweeperGameInstance::RefreshLegacyPlayerInventoryItems()
{
	PlayerInventoryItems.Reset();
	for (const FTunaSweeperInventorySlot& InventorySlot : PlayerInventorySlots)
	{
		FTunaSweeperItemStack ItemStack;
		if (const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(InventorySlot.ItemUid))
		{
			ItemStack.ItemId = ItemInstance->ItemId;
			ItemStack.Quantity = ItemInstance->Quantity;
		}
		else
		{
			ItemStack.ItemId = INDEX_NONE;
		}

		PlayerInventoryItems.Add(ItemStack);
	}

	bHasGeneratedPlayerInventoryItems = true;
}

void UTunaSweeperGameInstance::BroadcastInventoryStateChanged()
{
	bHasGeneratedPlayerInventoryItems = false;
	RefreshLegacyPlayerInventoryItems();
	ClearSelectedItemIfInvalid();
	OnInventoryStateChanged.Broadcast();
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

void UTunaSweeperGameInstance::CollectPlayerOwnedItemUids(TSet<FGuid>& OutItemUids) const
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

	auto CollectSlots = [&CollectItemUid](const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (const FTunaSweeperInventorySlot& Slot : Slots)
		{
			CollectItemUid(Slot.ItemUid);
		}
	};

	CollectSlots(PlayerInventorySlots);
	CollectSlots(EquipmentSlots);
	CollectSlots(AuxiliaryBagSlots);
}

bool UTunaSweeperGameInstance::IsBunkerToRaidTravel(FName SourceLevelName, FName TargetLevelName) const
{
	return IsMapNameMatch(SourceLevelName, TEXT("BunkerMap")) &&
		IsMapNameMatch(TargetLevelName, TEXT("RaidMap"));
}

bool UTunaSweeperGameInstance::IsRaidToBunkerTravel(FName SourceLevelName, FName TargetLevelName) const
{
	return IsMapNameMatch(SourceLevelName, TEXT("RaidMap")) &&
		IsMapNameMatch(TargetLevelName, TEXT("BunkerMap"));
}

bool UTunaSweeperGameInstance::IsMapNameMatch(FName MapName, const TCHAR* ExpectedMapName) const
{
	return MapName.ToString().EndsWith(ExpectedMapName);
}
