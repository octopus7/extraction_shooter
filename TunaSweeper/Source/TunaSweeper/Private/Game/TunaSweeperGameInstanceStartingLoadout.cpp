#include "TunaSweeperGameInstanceShared.h"

#include "Misc/ScopeExit.h"

namespace TunaSweeperStartingLoadout
{
	bool ReadInteger(const TSharedPtr<FJsonValue>& Value, int32& OutValue, int32 Minimum = 0)
	{
		double Number = 0;
		if (!Value.IsValid() || Value->Type != EJson::Number || !Value->TryGetNumber(Number) ||
			!FMath::IsFinite(Number) || Number < Minimum || Number > MAX_int32)
		{
			return false;
		}
		OutValue = static_cast<int32>(Number);
		return Number == OutValue;
	}

	bool ReadInteger(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, int32& OutValue, int32 Minimum = 0)
	{
		return Object.IsValid() && ReadInteger(Object->TryGetField(Key), OutValue, Minimum);
	}
}

bool UTunaSweeperGameInstance::InitializeDemoStartingLoadout()
{
	// The caller reports unavailable gameplay initialization separately from malformed data.
	if (!GetSubsystem<UTunaSweeperItemDataSubsystem>()) return false;
	FString Json;
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/DemoStartingLoadout.json"));
	if (!FFileHelper::LoadFileToString(Json, *Path) || !ApplyStartingLoadoutJson(Json))
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Invalid or missing starting loadout: %s"), *Path);
		return false;
	}
	return true;
}

bool UTunaSweeperGameInstance::ApplyStartingLoadoutJson(const FString& JsonContent)
{
	using namespace TunaSweeperStartingLoadout;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), Root) || !Root.IsValid()) return false;
	const TArray<TSharedPtr<FJsonValue>>* Equipment = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Inventory = nullptr;
	int32 SelectedWeaponSlot = 0;
	if (!Root->TryGetArrayField(TEXT("equipment"), Equipment) ||
		!Root->TryGetArrayField(TEXT("inventory"), Inventory) ||
		!ReadInteger(Root, TEXT("selected_weapon_slot"), SelectedWeaponSlot) ||
		SelectedWeaponSlot > TunaSweeperInventory::WeaponEquipmentSlotCount)
	{
		return false;
	}
	// Starting equipment is only applied to a fresh carried layout. Storage is independent.
	for (const FTunaSweeperInventorySlot& Slot : EquipmentSlots) if (!Slot.IsEmpty()) return false;
	for (const FTunaSweeperInventorySlot& Slot : PlayerInventorySlots) if (!Slot.IsEmpty()) return false;
	UTunaSweeperItemDataSubsystem* Items = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!Items || !Items->LoadItemData()) return false;

	const TArray<FTunaSweeperInventorySlot> PreviousEquipment = EquipmentSlots;
	const TArray<FTunaSweeperInventorySlot> PreviousInventory = PlayerInventorySlots;
	TArray<FGuid> CreatedUids;
	TSet<int32> AcquiredIds;
	bool bApplied = false;
	ON_SCOPE_EXIT
	{
		if (!bApplied)
		{
			EquipmentSlots = PreviousEquipment;
			PlayerInventorySlots = PreviousInventory;
			for (const FGuid& Uid : CreatedUids) ItemInstancesByUid.Remove(Uid);
		}
	};
	auto Create = [&](int32 ItemId, int32 Quantity)
	{
		const FGuid Uid = CreateItemInstance(ItemId, Quantity);
		CreatedUids.Add(Uid);
		AcquiredIds.Add(ItemId);
		return Uid;
	};

	for (const TSharedPtr<FJsonValue>& Value : *Equipment)
	{
		if (!Value.IsValid() || Value->Type != EJson::Object) return false;
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		int32 SlotIndex = INDEX_NONE;
		int32 ItemId = INDEX_NONE;
		FTunaSweeperItemDefinition Definition;
		if (!ReadInteger(Entry, TEXT("slot_index"), SlotIndex) ||
			!ReadInteger(Entry, TEXT("item_id"), ItemId, 1) ||
			!EquipmentSlots.IsValidIndex(SlotIndex) || !EquipmentSlots[SlotIndex].IsEmpty() ||
			!Items->TryGetItemDefinition(ItemId, Definition) || !DoesItemDefinitionMatchEquipmentSlot(SlotIndex, Definition))
		{
			return false;
		}
		const FGuid Uid = Create(ItemId, 1);
		EquipmentSlots[SlotIndex].ItemUid = Uid;
		const TArray<TSharedPtr<FJsonValue>>* Attachments = nullptr;
		if (Entry->HasField(TEXT("attachments")))
		{
			if (!Entry->TryGetArrayField(TEXT("attachments"), Attachments)) return false;
			for (const TSharedPtr<FJsonValue>& Attachment : *Attachments)
			{
				int32 AttachmentId = INDEX_NONE;
				FTunaSweeperItemDefinition AttachmentDefinition;
				if (!ReadInteger(Attachment, AttachmentId, 1) ||
					!Items->TryGetItemDefinition(AttachmentId, AttachmentDefinition) ||
					!DoesItemDefinitionAcceptAttachment(Definition, AttachmentDefinition) ||
					ItemInstancesByUid.FindChecked(Uid).AttachmentSlots.Contains(AttachmentDefinition.AttachmentSlotTag))
				{
					return false;
				}
				const FGuid AttachmentUid = Create(AttachmentId, 1);
				// Adding an item can reallocate the map, so look up the parent again.
				ItemInstancesByUid.FindChecked(Uid).AttachmentSlots.Add(AttachmentDefinition.AttachmentSlotTag, AttachmentUid);
			}
		}
		if (Entry->HasField(TEXT("loaded_ammo")))
		{
			const TSharedPtr<FJsonObject>* Ammo = nullptr;
			int32 AmmoId = INDEX_NONE;
			int32 Quantity = 0;
			FTunaSweeperItemDefinition AmmoDefinition;
			if (!Entry->TryGetObjectField(TEXT("loaded_ammo"), Ammo) ||
				!ReadInteger(*Ammo, TEXT("item_id"), AmmoId, 1) || !ReadInteger(*Ammo, TEXT("quantity"), Quantity) ||
				!Items->TryGetItemDefinition(AmmoId, AmmoDefinition) || !IsAmmoDefinitionCompatibleWithWeapon(Definition, AmmoDefinition) ||
				Quantity > CalculateWeaponMagazineCapacity(ItemInstancesByUid.FindChecked(Uid), Definition))
			{
				return false;
			}
			FTunaSweeperItemInstance& Instance = ItemInstancesByUid.FindChecked(Uid);
			Instance.LoadedAmmoItemId = AmmoId;
			Instance.SelectedAmmoItemId = AmmoId;
			Instance.LoadedAmmoCount = Quantity;
			if (Quantity > 0) AcquiredIds.Add(AmmoId);
		}
	}

	PlayerInventorySlots.SetNum(CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots));
	for (const TSharedPtr<FJsonValue>& Value : *Inventory)
	{
		if (!Value.IsValid() || Value->Type != EJson::Object) return false;
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		int32 SlotIndex = INDEX_NONE;
		int32 ItemId = INDEX_NONE;
		int32 Quantity = 0;
		FTunaSweeperItemDefinition Definition;
		if (!ReadInteger(Entry, TEXT("slot_index"), SlotIndex) || !ReadInteger(Entry, TEXT("item_id"), ItemId, 1) ||
			!ReadInteger(Entry, TEXT("quantity"), Quantity, 1) || !PlayerInventorySlots.IsValidIndex(SlotIndex) ||
			!PlayerInventorySlots[SlotIndex].IsEmpty() || !Items->TryGetItemDefinition(ItemId, Definition) ||
			Quantity > Items->ResolveItemMaxStackQuantity(Definition))
		{
			return false;
		}
		PlayerInventorySlots[SlotIndex].ItemUid = Create(ItemId, Quantity);
	}
	const int32 SelectedIndex = SelectedWeaponSlot == 0
		? TunaSweeperInventory::MeleeEquipmentSlotIndex : SelectedWeaponSlot - 1;
	if (!EquipmentSlots.IsValidIndex(SelectedIndex) || EquipmentSlots[SelectedIndex].IsEmpty()) return false;

	for (int32 ItemId : AcquiredIds) MarkItemEverAcquired(ItemId);
	if (SelectedWeaponSlot == 0) SetRuntimeSelectedMeleeWeapon();
	else SetRuntimeSelectedWeaponSlotNumber(SelectedWeaponSlot);
	bApplied = true;
	return true;
}
