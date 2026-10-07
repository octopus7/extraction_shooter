#include "Subsystem/TunaSweeperItemDataSubsystem.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogWeaponConfiguration, Log, All);

namespace
{
	bool ReadInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, int32 Minimum, int32& Out)
	{
		double Value;
		if (!Object || !Object->TryGetNumberField(Key, Value) || !FMath::IsFinite(Value) ||
			Value < Minimum || Value > MAX_int32 || FMath::FloorToDouble(Value) != Value) return false;
		Out = static_cast<int32>(Value);
		return true;
	}

	bool ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FVector& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values;
		if (!Object->TryGetArrayField(Key, Values) || Values->Num() != 3) return false;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			double Value;
			if (!(*Values)[Axis]->TryGetNumber(Value) || !FMath::IsFinite(Value)) return false;
			Out[Axis] = Value;
		}
		return true;
	}

	bool ReadTags(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, const TCHAR* Prefix, TArray<FName>& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object->TryGetArrayField(Key, Values)) return false;
		for (const auto& Value : *Values)
		{
			FString Tag;
			if (!Value || !Value->TryGetString(Tag)) return false;
			Tag.TrimStartAndEndInline();
			const FName Name(*Tag);
			if (!Tag.StartsWith(Prefix) || Tag.Len() <= FCString::Strlen(Prefix) || Out.Contains(Name)) return false;
			Out.Add(Name);
		}
		return true;
	}

	bool IsGun(const FTunaSweeperItemDefinition* Item)
	{
		return Item && Item->CategoryTag == FName(TEXT("item.category.weapon.gun")) && !Item->WeaponTypeTag.IsNone();
	}

	bool IsCompatible(const FTunaSweeperItemDefinition* Weapon, const FTunaSweeperItemDefinition* Ammo)
	{
		return IsGun(Weapon) && Ammo && !Ammo->AmmoTypeTag.IsNone() &&
			Weapon->CompatibleAmmoTypeTags.Contains(Ammo->AmmoTypeTag);
	}
}

bool UTunaSweeperItemDataSubsystem::ParseWeaponDefinitions(const FString& Json)
{
	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Rows)) return false;
	TMap<int32, FTunaSweeperItemDefinition> Parsed;
	for (const auto& Row : Rows)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		int32 Id = INDEX_NONE;
		if (!Row || !Row->TryGetObject(Object) || !ReadInt(*Object, TEXT("item_id"), 1, Id) || Parsed.Contains(Id)) return false;
		const FTunaSweeperItemDefinition* Item = ItemDefinitionsById.Find(Id);
		if (!Item || Item->CategoryTag != FName(TEXT("item.category.weapon.gun"))) return false;
		FTunaSweeperItemDefinition Weapon = *Item;
		FString Type, Mode;
		double Reload = 0.0;
		if (!(*Object)->TryGetStringField(TEXT("weapon_type_tag"), Type) ||
			!(*Object)->TryGetStringField(TEXT("fire_mode"), Mode) ||
			!ReadInt(*Object, TEXT("magazine_capacity"), 1, Weapon.MagazineCapacity) ||
			!(*Object)->TryGetNumberField(TEXT("reload_seconds"), Reload) ||
			!FMath::IsFinite(Reload) || Reload <= 0.0 || Reload > MAX_flt) return false;
		Type.TrimStartAndEndInline();
		if (!Type.StartsWith(TEXT("weapon.type.")) || Type.Len() <= 12) return false;
		Weapon.WeaponTypeTag = FName(*Type);
		Weapon.ReloadSeconds = static_cast<float>(Reload);
		if (Weapon.ReloadSeconds <= 0.0f) return false;
		if (Mode == TEXT("automatic")) Weapon.FireMode = ETunaSweeperWeaponFireMode::Automatic;
		else if (Mode == TEXT("semi_automatic")) Weapon.FireMode = ETunaSweeperWeaponFireMode::SemiAutomatic;
		else return false;
		Weapon.CompatibleAmmoTypeTags.Reset();
		Weapon.AttachmentSlotTags.Reset();
		if (!ReadTags(*Object, TEXT("compatible_ammo_type_tags"), TEXT("ammo.type."), Weapon.CompatibleAmmoTypeTags) ||
			Weapon.CompatibleAmmoTypeTags.IsEmpty() ||
			!ReadTags(*Object, TEXT("attachment_slot_tags"), TEXT("attachment.slot."), Weapon.AttachmentSlotTags)) return false;
		for (const FName AmmoType : Weapon.CompatibleAmmoTypeTags)
		{
			bool bFoundAmmo = false;
			for (const auto& Pair : ItemDefinitionsById)
			{
				if (Pair.Value.CategoryTag == FName(TEXT("item.category.ammo")) && Pair.Value.AmmoTypeTag == AmmoType)
				{
					bFoundAmmo = true;
					break;
				}
			}
			if (!bFoundAmmo) return false;
		}
		Parsed.Add(Id, MoveTemp(Weapon));
	}
	for (const auto& Pair : ItemDefinitionsById)
	{
		if (Pair.Value.CategoryTag == FName(TEXT("item.category.weapon.gun")) && !Parsed.Contains(Pair.Key)) return false;
	}
	// Commit only after every row and every gun reference has passed validation.
	for (auto& Pair : Parsed) ItemDefinitionsById.FindChecked(Pair.Key) = MoveTemp(Pair.Value);
	return true;
}

bool UTunaSweeperItemDataSubsystem::LoadWeaponConfigurationJson()
{
	struct FFileLoader { const TCHAR* File; bool (UTunaSweeperItemDataSubsystem::*Parse)(const FString&); };
	const FFileLoader Files[] = {
		{ TEXT("WeaponDefinitions.json"), &UTunaSweeperItemDataSubsystem::ParseWeaponDefinitions },
		{ TEXT("WeaponVisualDefinitions.json"), &UTunaSweeperItemDataSubsystem::ParseWeaponVisualDefinitions },
		{ TEXT("EnemyDefaultLoadout.json"), &UTunaSweeperItemDataSubsystem::ParseEnemyDefaultLoadout },
		{ TEXT("CombatLabEnemyLoadout.json"), &UTunaSweeperItemDataSubsystem::ParseCombatLabEnemyLoadout }
	};
	for (const FFileLoader& File : Files)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), File.File);
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *Path) || !(this->*File.Parse)(Json))
		{
			UE_LOG(LogWeaponConfiguration, Error, TEXT("Invalid or missing weapon configuration: %s"), *Path);
			return false;
		}
	}
	return true;
}

bool UTunaSweeperItemDataSubsystem::ParseWeaponVisualDefinitions(const FString& Json)
{
	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Rows)) return false;
	TMap<int32, FTunaSweeperWeaponVisualDefinition> Parsed;
	for (const auto& Row : Rows)
	{
		const TSharedPtr<FJsonObject>* Object;
		if (!Row || !Row->TryGetObject(Object)) return false;
		int32 Id;
		FString Mesh, Material;
		FVector Rotation;
		double FitLength;
		FTunaSweeperWeaponVisualDefinition Visual;
		if (!ReadInt(*Object, TEXT("item_id"), 1, Id) || Parsed.Contains(Id) ||
			!(*Object)->TryGetStringField(TEXT("mesh"), Mesh) ||
			!(*Object)->TryGetStringField(TEXT("material"), Material) ||
			!ReadVector(*Object, TEXT("location"), Visual.Location) ||
			!ReadVector(*Object, TEXT("rotation"), Rotation) ||
			!ReadVector(*Object, TEXT("scale"), Visual.Scale) || Visual.Scale.GetMin() <= 0 ||
			!(*Object)->TryGetBoolField(TEXT("align_longest_axis_to_x"), Visual.bAlignLongestAxisToX) ||
			!(*Object)->TryGetBoolField(TEXT("center_on_bounds"), Visual.bCenterOnBounds) ||
			!(*Object)->TryGetNumberField(TEXT("fit_length_cm"), FitLength) ||
			!FMath::IsFinite(FitLength) || FitLength < 0 || FitLength > MAX_flt) return false;
		const auto* Item = ItemDefinitionsById.Find(Id);
		if (!Item || (Item->CategoryTag != FName(TEXT("item.category.weapon.melee")) && !IsGun(Item))) return false;
		Visual.Mesh = FSoftObjectPath(Mesh);
		Visual.Material = FSoftObjectPath(Material);
		if (!Visual.Mesh.IsValid() || !Visual.Material.IsValid()) return false;
		Visual.Rotation = FRotator(Rotation.X, Rotation.Y, Rotation.Z);
		Visual.FitLengthCm = static_cast<float>(FitLength);
		Parsed.Add(Id, Visual);
	}
	WeaponVisualsByItemId = MoveTemp(Parsed);
	return true;
}

bool UTunaSweeperItemDataSubsystem::ParseEnemyDefaultLoadout(const FString& Json)
{
	TSharedPtr<FJsonObject> Object;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) || !Object) return false;
	FTunaSweeperEnemyDefaultLoadout Parsed;
	const TArray<TSharedPtr<FJsonValue>>* Rows;
	if (!ReadInt(Object, TEXT("weapon_item_id"), 1, Parsed.WeaponItemId) ||
		!IsGun(ItemDefinitionsById.Find(Parsed.WeaponItemId)) ||
		!ReadInt(Object, TEXT("reserve_magazine_count"), 0, Parsed.ReserveMagazineCount) ||
		!Object->TryGetArrayField(TEXT("ammo_by_weapon_type"), Rows)) return false;
	for (const auto& Row : *Rows)
	{
		const TSharedPtr<FJsonObject>* Entry;
		FString Tag;
		int32 AmmoId;
		if (!Row || !Row->TryGetObject(Entry) || !(*Entry)->TryGetStringField(TEXT("weapon_type_tag"), Tag) ||
			!ReadInt(*Entry, TEXT("ammo_item_id"), 1, AmmoId)) return false;
		const FName Type(*Tag);
		if (Type.IsNone() || Parsed.AmmoItemIdsByWeaponType.Contains(Type)) return false;
		bool bFoundWeapon = false;
		for (const auto& Pair : ItemDefinitionsById)
		{
			if (!IsGun(&Pair.Value) || Pair.Value.WeaponTypeTag != Type) continue;
			if (!IsCompatible(&Pair.Value, ItemDefinitionsById.Find(AmmoId)) ||
				static_cast<int64>(FMath::Max(1, Pair.Value.MagazineCapacity)) * Parsed.ReserveMagazineCount > MAX_int32) return false;
			bFoundWeapon = true;
		}
		if (!bFoundWeapon) return false;
		Parsed.AmmoItemIdsByWeaponType.Add(Type, AmmoId);
	}
	const auto& DefaultWeapon = ItemDefinitionsById.FindChecked(Parsed.WeaponItemId);
	if (!Parsed.AmmoItemIdsByWeaponType.Contains(DefaultWeapon.WeaponTypeTag)) return false;
	EnemyDefaultLoadout = MoveTemp(Parsed);
	return true;
}

bool UTunaSweeperItemDataSubsystem::ParseCombatLabEnemyLoadout(const FString& Json)
{
	TSharedPtr<FJsonObject> Object;
	FTunaSweeperEnemyWeaponLoadout Parsed;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) || !Object ||
		!ReadInt(Object, TEXT("weapon_item_id"), 1, Parsed.WeaponItemId) ||
		!ReadInt(Object, TEXT("ammo_item_id"), 1, Parsed.AmmoItemId) ||
		!ReadInt(Object, TEXT("reserve_ammo_count"), 0, Parsed.ReserveAmmoCount) ||
		!IsCompatible(ItemDefinitionsById.Find(Parsed.WeaponItemId), ItemDefinitionsById.Find(Parsed.AmmoItemId))) return false;
	CombatLabEnemyLoadout = Parsed;
	return true;
}

bool UTunaSweeperItemDataSubsystem::TryGetWeaponVisualDefinition(int32 ItemId, FTunaSweeperWeaponVisualDefinition& OutDefinition)
{
	OutDefinition = {};
	if (!EnsureItemDataLoaded()) return false;
	const auto* Definition = WeaponVisualsByItemId.Find(ItemId);
	if (!Definition) return false;
	OutDefinition = *Definition;
	return true;
}

bool UTunaSweeperItemDataSubsystem::TryResolveEnemyLoadout(int32 WeaponItemId, int32 AmmoItemId,
	int32 ReserveAmmoCount, FTunaSweeperEnemyWeaponLoadout& OutLoadout)
{
	OutLoadout = {};
	if (!EnsureItemDataLoaded()) return false;
	const auto* Weapon = ItemDefinitionsById.Find(WeaponItemId);
	if (!IsGun(Weapon)) Weapon = ItemDefinitionsById.Find(EnemyDefaultLoadout.WeaponItemId);
	if (!IsGun(Weapon)) return false;
	const auto* Ammo = ItemDefinitionsById.Find(AmmoItemId);
	if (!IsCompatible(Weapon, Ammo))
	{
		const int32* DefaultAmmo = EnemyDefaultLoadout.AmmoItemIdsByWeaponType.Find(Weapon->WeaponTypeTag);
		Ammo = DefaultAmmo ? ItemDefinitionsById.Find(*DefaultAmmo) : nullptr;
	}
	if (!IsCompatible(Weapon, Ammo)) return false;
	const int64 Reserve = ReserveAmmoCount == INDEX_NONE
		? static_cast<int64>(FMath::Max(1, Weapon->MagazineCapacity)) * EnemyDefaultLoadout.ReserveMagazineCount
		: FMath::Max(0, ReserveAmmoCount);
	if (Reserve > MAX_int32) return false;
	OutLoadout = { Weapon->Id, Ammo->Id, static_cast<int32>(Reserve) };
	return true;
}

bool UTunaSweeperItemDataSubsystem::TryGetCombatLabEnemyLoadout(FTunaSweeperEnemyWeaponLoadout& OutLoadout)
{
	OutLoadout = {};
	if (!EnsureItemDataLoaded()) return false;
	OutLoadout = CombatLabEnemyLoadout;
	return true;
}
