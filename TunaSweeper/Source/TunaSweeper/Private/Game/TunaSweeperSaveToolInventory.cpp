#include "Game/TunaSweeperSaveToolInternal.h"
#if WITH_EDITOR
#include "Game/TunaSweeperGameInstanceShared.h"
#include "GameMapsSettings.h"

namespace TunaSweeperSaveTool::Internal
{
	bool Fields(const FObject& Object, std::initializer_list<const TCHAR*> Allowed)
	{
		if (!Object) return false;
		for (const auto& Field : Object->Values)
		{
			bool bFound = false;
			for (const TCHAR* Key : Allowed) bFound |= Field.Key == Key;
			if (!bFound) return false;
		}
		return true;
	}

	bool Integer(const FObject& Object, const TCHAR* Key, int32& Out, int32 Min, int32 Max)
	{
		const auto* Value = Object ? Object->Values.Find(Key) : nullptr;
		double Number = 0;
		if (!Value || (*Value)->Type != EJson::Number || !(*Value)->TryGetNumber(Number) ||
			!FMath::IsFinite(Number) || Number < Min || Number > Max || FMath::FloorToDouble(Number) != Number) return false;
		Out = static_cast<int32>(Number);
		return true;
	}

	FTunaSweeperItemInstance* Find(UTunaSweeperSaveGame& Save, const FGuid& Uid)
	{
		return Save.ItemInstances.FindByPredicate([&](const auto& Item) { return Item.Uid == Uid; });
	}

	bool AllowedItem(const FTunaSweeperItemDefinition& D)
	{
		using namespace TunaSweeperInventory;
		return D.CategoryTag == GunCategoryTag || D.CategoryTag == MeleeCategoryTag || D.CategoryTag == AmmoCategoryTag ||
			D.CategoryTag == HeadCategoryTag || D.CategoryTag == BodyCategoryTag;
	}

	bool FitsEquipment(int32 Slot, const FTunaSweeperItemDefinition& D)
	{
		const auto* Rule = TunaSweeperInventory::GetEquipmentSlotRule(Slot);
		return Rule && (D.EquipmentSlotTag == Rule->EquipmentSlotTag || D.CategoryTag == Rule->CategoryTag ||
			(Slot == TunaSweeperInventory::BackpackSlotIndex && D.InventorySlotCapacity > 0));
	}

	int32 Capacity(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data)
	{
		// Read the configured Blueprint CDO; never initialize a live GameInstance or its save paths.
		const UClass* Class = GetDefault<UGameMapsSettings>()->GameInstanceClass.TryLoadClass<UTunaSweeperGameInstance>();
		if (!Class) return INDEX_NONE;
		const auto& Settings = Class->GetDefaultObject<UTunaSweeperGameInstance>()->GameplaySettings;
		const int32 Bare = FMath::Max(TunaSweeperInventory::RequiredBareInventorySlots, Settings.BareInventorySlots);
		const int32 Maximum = FMath::Max(Bare, FMath::Max(TunaSweeperInventory::RequiredMaxInventorySlots, Settings.MaxInventorySlots));
		int32 Slots = Bare;
		if (Save.EquipmentSlots.IsValidIndex(TunaSweeperInventory::BackpackSlotIndex))
		{
			const auto* Bag = Find(Save, Save.EquipmentSlots[TunaSweeperInventory::BackpackSlotIndex].ItemUid);
			FTunaSweeperItemDefinition D;
			if (Bag && Data.TryGetItemDefinition(Bag->ItemId, D)) Slots = FMath::Max(Bare, D.InventorySlotCapacity);
		}
		return TunaSweeperInventory::ClampSlotCount(Slots, Bare, Maximum);
	}

	static bool AmmoFits(const FTunaSweeperItemDefinition& Weapon, const FTunaSweeperItemDefinition& Ammo)
	{
		using namespace TunaSweeperInventory;
		return (Weapon.CategoryTag == GunCategoryTag || Weapon.EquipmentSlotTag == GunEquipmentSlotTag) &&
			Ammo.CategoryTag == AmmoCategoryTag && !Ammo.AmmoTypeTag.IsNone() &&
			(Weapon.CompatibleAmmoTypeTags.Num() ? Weapon.CompatibleAmmoTypeTags.Contains(Ammo.AmmoTypeTag) :
				GetDefaultAmmoTypeTagForWeaponType(Weapon.WeaponTypeTag) == Ammo.AmmoTypeTag);
	}

	static bool AttachmentFits(const FTunaSweeperItemDefinition& D, const FTunaSweeperItemDefinition& Attachment, FName Slot)
	{
		return !Slot.IsNone() && Attachment.AttachmentSlotTag == Slot && D.AttachmentSlotTags.Contains(Slot) &&
			(Attachment.CompatibleWeaponTypeTags.IsEmpty() || Attachment.CompatibleWeaponTypeTags.Contains(D.WeaponTypeTag));
	}

	FString Validate(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data)
	{
		TSet<FGuid> Uids, Owned;
		for (const auto& Item : Save.ItemInstances)
		{
			FTunaSweeperItemDefinition D;
			if (!Item.IsValid() || Uids.Contains(Item.Uid)) return TEXT("invalid_uid");
			Uids.Add(Item.Uid);
			if (!Data.TryGetItemDefinition(Item.ItemId, D)) return TEXT("unknown_item");
			if (Item.Quantity > FMath::Max(1, Data.ResolveItemMaxStackQuantity(D))) return TEXT("invalid_stack");
		}
		for (const auto* Slots : { &Save.InventorySlots, &Save.EquipmentSlots, &Save.AuxiliaryBagSlots, &Save.StorageSlots, &Save.UsableQuickSlots })
		{
			for (const auto& Slot : *Slots)
			{
				if (!Slot.ItemUid.IsValid()) continue;
				if (!Uids.Contains(Slot.ItemUid) || Owned.Contains(Slot.ItemUid)) return TEXT("invalid_reference");
				Owned.Add(Slot.ItemUid);
			}
		}
		for (const auto& Item : Save.ItemInstances)
		{
			FTunaSweeperItemDefinition D;
			Data.TryGetItemDefinition(Item.ItemId, D);
			int64 Magazine = D.MagazineCapacity > 0 ? D.MagazineCapacity : TunaSweeperInventory::DefaultWeaponMagazineCapacity;
			for (const auto& Pair : Item.AttachmentSlots)
			{
				if (!Pair.Value.IsValid()) continue; // Empty runtime attachment slots are allowed.
				const auto* Child = Find(Save, Pair.Value);
				FTunaSweeperItemDefinition A;
				if (!Child || Owned.Contains(Pair.Value) || !Child->AttachmentSlots.IsEmpty() || Child->Quantity != 1 ||
					!Data.TryGetItemDefinition(Child->ItemId, A) || !AttachmentFits(D, A, Pair.Key)) return TEXT("invalid_attachment");
				Owned.Add(Pair.Value);
				Magazine += FMath::Max(0, A.MagazineCapacityBonus);
			}
			if (Item.LoadedAmmoCount < 0) return TEXT("invalid_ammo");
			if (Item.LoadedAmmoItemId != INDEX_NONE)
			{
				FTunaSweeperItemDefinition Ammo;
				if (!Data.TryGetItemDefinition(Item.LoadedAmmoItemId, Ammo) || !AmmoFits(D, Ammo) || Item.LoadedAmmoCount > Magazine || Item.Quantity != 1) return TEXT("invalid_ammo");
			}
			else if (Item.LoadedAmmoCount != 0) return TEXT("invalid_ammo");
		}
		if (Owned.Num() != Uids.Num()) return TEXT("orphan_item");
		for (int32 Index = 0; Index < Save.EquipmentSlots.Num(); ++Index)
		{
			const auto* Item = Find(Save, Save.EquipmentSlots[Index].ItemUid);
			FTunaSweeperItemDefinition D;
			if (Item && (!Data.TryGetItemDefinition(Item->ItemId, D) || !FitsEquipment(Index, D))) return TEXT("invalid_equipment");
		}
		const int32 Limit = Capacity(Save, Data);
		if (Limit < 0) return TEXT("settings_unavailable");
		for (int32 Index = Limit; Index < Save.InventorySlots.Num(); ++Index)
			if (!Save.InventorySlots[Index].IsEmpty()) return TEXT("inventory_full");
		return FString();
	}

	static FString CreateItem(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data, const FObject& Spec, FTunaSweeperItemInstance& Item)
	{
		if (!Fields(Spec, { TEXT("itemId"), TEXT("quantity"), TEXT("loadedAmmoItemId"), TEXT("loadedAmmoCount"), TEXT("attachments") }) ||
			!Integer(Spec, TEXT("itemId"), Item.ItemId, 1, MAX_int32) ||
			(Spec->HasField(TEXT("quantity")) && !Integer(Spec, TEXT("quantity"), Item.Quantity, 1, 1000000))) return TEXT("invalid_item_spec");
		FTunaSweeperItemDefinition D;
		if (!Data.TryGetItemDefinition(Item.ItemId, D)) return TEXT("unknown_item");
		Item.Uid = FGuid::NewGuid();
		if (Spec->HasField(TEXT("loadedAmmoItemId")) && !Integer(Spec, TEXT("loadedAmmoItemId"), Item.LoadedAmmoItemId, 1, MAX_int32)) return TEXT("invalid_ammo");
		if (Spec->HasField(TEXT("loadedAmmoCount")) && !Integer(Spec, TEXT("loadedAmmoCount"), Item.LoadedAmmoCount, 0, 1000000)) return TEXT("invalid_ammo");
		Item.SelectedAmmoItemId = Item.LoadedAmmoItemId;
		const TArray<TSharedPtr<FJsonValue>>* Attachments = nullptr;
		if (Spec->HasField(TEXT("attachments")))
		{
			if (!Spec->TryGetArrayField(TEXT("attachments"), Attachments) || Attachments->Num() > 16) return TEXT("invalid_attachment");
			for (const auto& Value : *Attachments)
			{
				if (Value->Type != EJson::Object) return TEXT("invalid_attachment");
				auto A = Value->AsObject();
				FString Slot;
				FTunaSweeperItemInstance Child;
				FTunaSweeperItemDefinition AD;
				if (!Fields(A, { TEXT("slotTag"), TEXT("itemId") }) || !A->TryGetStringField(TEXT("slotTag"), Slot) ||
					!Integer(A, TEXT("itemId"), Child.ItemId, 1, MAX_int32) || !Data.TryGetItemDefinition(Child.ItemId, AD) ||
					!AttachmentFits(D, AD, FName(*Slot)) || Item.AttachmentSlots.Contains(FName(*Slot))) return TEXT("invalid_attachment");
				Child.Uid = FGuid::NewGuid();
				Save.ItemInstances.Add(Child);
				Save.EverAcquiredItemIds.AddUnique(Child.ItemId);
				Item.AttachmentSlots.Add(FName(*Slot), Child.Uid);
			}
		}
		if ((Item.LoadedAmmoItemId != INDEX_NONE || !Item.AttachmentSlots.IsEmpty()) && Item.Quantity != 1) return TEXT("invalid_stack");
		Save.EverAcquiredItemIds.AddUnique(Item.ItemId);
		if (Item.LoadedAmmoItemId != INDEX_NONE) Save.EverAcquiredItemIds.AddUnique(Item.LoadedAmmoItemId);
		return FString();
	}

	static bool Stackable(const FTunaSweeperItemInstance& I)
	{
		return I.AttachmentSlots.IsEmpty() && I.LoadedAmmoItemId == INDEX_NONE && I.LoadedAmmoCount == 0 && I.SelectedAmmoItemId == INDEX_NONE;
	}

	static bool Place(UTunaSweeperSaveGame& Save, const FGuid& Uid)
	{
		for (auto& Slot : Save.InventorySlots)
			if (Slot.IsEmpty() && !Slot.bSortLocked) { Slot.ItemUid = Uid; return true; }
		return false;
	}

	FString Add(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data, const FObject& Request)
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!Request->TryGetArrayField(TEXT("items"), Items) || Items->IsEmpty() || Items->Num() > 1000) return TEXT("invalid_items");
		const int32 Limit = Capacity(Save, Data);
		if (Limit < 0) return TEXT("settings_unavailable");
		Save.InventorySlots.SetNum(Limit);
		for (const auto& Value : *Items)
		{
			if (Value->Type != EJson::Object) return TEXT("invalid_item_spec");
			FTunaSweeperItemInstance Item;
			FString Error = CreateItem(Save, Data, Value->AsObject(), Item);
			if (!Error.IsEmpty()) return Error;
			FTunaSweeperItemDefinition D;
			Data.TryGetItemDefinition(Item.ItemId, D);
			if (!AllowedItem(D)) return TEXT("unsupported_item_category");
			const int32 Stack = FMath::Max(1, Data.ResolveItemMaxStackQuantity(D));
			int32 Remaining = Item.Quantity;
			if (Stackable(Item))
				for (const auto& Slot : Save.InventorySlots)
				{
					auto* Existing = Find(Save, Slot.ItemUid);
					if (Existing && Existing->ItemId == Item.ItemId && Stackable(*Existing))
					{
						int32 Count = FMath::Min(Remaining, FMath::Max(0, Stack - Existing->Quantity));
						Existing->Quantity += Count;
						Remaining -= Count;
					}
				}
			while (Remaining > 0)
			{
				Item.Uid = FGuid::NewGuid();
				Item.Quantity = FMath::Min(Remaining, Stack);
				if (!Place(Save, Item.Uid)) return TEXT("inventory_full");
				Save.ItemInstances.Add(Item);
				Remaining -= Item.Quantity;
			}
		}
		return Validate(Save, Data);
	}

	FString ApplyPreset(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data, const FObject& Preset)
	{
		int32 Version = 0;
		const TArray<TSharedPtr<FJsonValue>>* Equipment = nullptr;
		if (!Fields(Preset, { TEXT("version"), TEXT("equipment") }) || !Integer(Preset, TEXT("version"), Version, 1, 1) ||
			!Preset->TryGetArrayField(TEXT("equipment"), Equipment) || Equipment->Num() > 8) return TEXT("invalid_preset");
		TArray<FGuid> Displaced;
		for (const auto& Slot : Save.EquipmentSlots) if (!Slot.IsEmpty()) Displaced.Add(Slot.ItemUid);
		Save.EquipmentSlots.Empty();
		Save.EquipmentSlots.SetNum(TunaSweeperInventory::RequiredEquipmentSlots);
		TSet<int32> UsedSlots;
		for (const auto& Value : *Equipment)
		{
			if (Value->Type != EJson::Object) return TEXT("invalid_preset");
			const auto Row = Value->AsObject();
			const FObject* Spec = nullptr;
			int32 Slot = -1;
			if (!Fields(Row, { TEXT("slot"), TEXT("item") }) || !Integer(Row, TEXT("slot"), Slot, 0, 7) || UsedSlots.Contains(Slot) ||
				!Row->TryGetObjectField(TEXT("item"), Spec)) return TEXT("invalid_preset");
			UsedSlots.Add(Slot);
			FTunaSweeperItemInstance Item;
			FString Error = CreateItem(Save, Data, *Spec, Item);
			if (!Error.IsEmpty()) return Error;
			FTunaSweeperItemDefinition D;
			Data.TryGetItemDefinition(Item.ItemId, D);
			if (Item.Quantity != 1 || !FitsEquipment(Slot, D)) return TEXT("invalid_equipment");
			Save.ItemInstances.Add(Item);
			Save.EquipmentSlots[Slot].ItemUid = Item.Uid;
		}
		const int32 Limit = Capacity(Save, Data);
		if (Limit < 0) return TEXT("settings_unavailable");
		// Preserve positions when possible, including sort locks. Move overflow before shrinking.
		for (int32 Index = Limit; Index < Save.InventorySlots.Num(); ++Index)
			if (!Save.InventorySlots[Index].IsEmpty()) Displaced.Add(Save.InventorySlots[Index].ItemUid);
		Save.InventorySlots.SetNum(Limit);
		for (const auto& Uid : Displaced) if (!Place(Save, Uid)) return TEXT("inventory_full");
		return Validate(Save, Data);
	}

	FObject ExportPreset(UTunaSweeperSaveGame& Save)
	{
		auto Preset = MakeShared<FJsonObject>();
		Preset->SetNumberField(TEXT("version"), 1);
		TArray<TSharedPtr<FJsonValue>> Rows;
		for (int32 Slot = 0; Slot < Save.EquipmentSlots.Num(); ++Slot)
		{
			const auto* Item = Find(Save, Save.EquipmentSlots[Slot].ItemUid);
			if (!Item) continue;
			auto Spec = MakeShared<FJsonObject>();
			Spec->SetNumberField(TEXT("itemId"), Item->ItemId);
			Spec->SetNumberField(TEXT("quantity"), Item->Quantity);
			if (Item->LoadedAmmoItemId != INDEX_NONE)
			{
				Spec->SetNumberField(TEXT("loadedAmmoItemId"), Item->LoadedAmmoItemId);
				Spec->SetNumberField(TEXT("loadedAmmoCount"), Item->LoadedAmmoCount);
			}
			TArray<TSharedPtr<FJsonValue>> Attachments;
			TArray<FName> Tags;
			Item->AttachmentSlots.GetKeys(Tags);
			Tags.Sort(FNameLexicalLess());
			for (FName Tag : Tags)
			{
				const auto* Child = Find(Save, Item->AttachmentSlots[Tag]);
				if (!Child) continue;
				auto A = MakeShared<FJsonObject>();
				A->SetStringField(TEXT("slotTag"), Tag.ToString());
				A->SetNumberField(TEXT("itemId"), Child->ItemId);
				Attachments.Add(MakeShared<FJsonValueObject>(A));
			}
			Spec->SetArrayField(TEXT("attachments"), Attachments);
			auto Row = MakeShared<FJsonObject>();
			Row->SetNumberField(TEXT("slot"), Slot);
			Row->SetObjectField(TEXT("item"), Spec);
			Rows.Add(MakeShared<FJsonValueObject>(Row));
		}
		Preset->SetArrayField(TEXT("equipment"), Rows);
		return Preset;
	}
}
#endif
