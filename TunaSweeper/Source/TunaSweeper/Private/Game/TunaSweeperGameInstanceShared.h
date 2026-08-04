#pragma once

#include "Game/TunaSweeperGameInstance.h"

#include "Component/TunaSweeperDebuffComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperDebuffDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

#include <initializer_list>

DECLARE_LOG_CATEGORY_EXTERN(LogTunaSweeperGameInstance, Log, All);

namespace TunaSweeperSave
{
	inline const TCHAR* SaveSlotNamePrefix = TEXT("TunaSweeperSave_Slot");
	inline const TCHAR* SaveSettingsSlotName = TEXT("TunaSweeperSaveSettings");
	constexpr int32 CurrentSaveVersion = 19;
	constexpr int32 SaveUserIndex = 0;
	constexpr int32 MinSaveSlotIndex = 1;
	constexpr int32 MaxSaveSlotIndex = 3;
	constexpr int32 MinDifficultyStage = 1;
	constexpr int32 MaxDifficultyStage = 3;
	constexpr int32 DefaultDifficultyStage = 1;
	constexpr int32 MaxSaveGameBackupCount = 30;

	inline int32 SanitizeDifficultyStage(int32 DifficultyStage)
	{
		return FMath::Clamp(DifficultyStage, MinDifficultyStage, MaxDifficultyStage);
	}
}

namespace TunaSweeperShop
{
	inline FName MakeStockKey(int32 ShopId, int32 SlotIndex, int32 ItemId)
	{
		return FName(*FString::Printf(TEXT("%d:%d:%d"), ShopId, SlotIndex, ItemId));
	}

	inline bool IsValidShopSlotKey(int32 ShopId, int32 SlotIndex, int32 ItemId)
	{
		return ShopId > 0 && SlotIndex != INDEX_NONE && ItemId != INDEX_NONE;
	}
}

namespace TunaSweeperScenario
{
	const FName OpeningScenarioFlag(TEXT("scenario.opening.awakening"));
	const FName OpeningScenarioMapName(TEXT("OpeningScenarioMap"));
	const FName BunkerMapName(TEXT("BunkerMap"));
}

namespace TunaSweeperDialogue
{
	const FName CharactersPerSecondSettingKey(TEXT("dialogue.characters_per_second"));
}

namespace TunaSweeperDebug
{
	const FName VisionDebugSettingKey(TEXT("debug.vision"));
	const FName BunkerVisionDebugSettingKey(TEXT("debug.bunker_vision"));
}

namespace TunaSweeperExperience
{
	inline const TCHAR* LevelTableJsonRelativePath = TEXT("Data/ExperienceLevelTable.json");
	inline const TCHAR* LevelRewardsJsonRelativePath = TEXT("Data/ExperienceLevelRewards.json");
	constexpr int32 DefaultMaxExperienceLevel = 30;
	constexpr int64 BaseExperienceForNextLevel = 100;
	constexpr int64 ExperienceIncreasePerLevel = 50;
	constexpr float RaidReturnAnimationDurationSeconds = 3.2f;

	inline bool TryReadDataValueField(
		const TSharedPtr<FJsonObject>& JsonObject,
		std::initializer_list<const TCHAR*> FieldNames,
		int32& OutValue)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		double NumericValue = 0.0;
		for (const TCHAR* FieldName : FieldNames)
		{
			if (JsonObject->TryGetNumberField(FieldName, NumericValue))
			{
				OutValue = TunaSweeperDataValues::ClampRatioValue(FMath::RoundToInt(NumericValue));
				return true;
			}
		}

		return false;
	}

	inline bool TryReadInt64Field(
		const TSharedPtr<FJsonObject>& JsonObject,
		std::initializer_list<const TCHAR*> FieldNames,
		int64& OutValue)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		double NumericValue = 0.0;
		for (const TCHAR* FieldName : FieldNames)
		{
			if (JsonObject->TryGetNumberField(FieldName, NumericValue))
			{
				OutValue = static_cast<int64>(FMath::Max(0.0, NumericValue) + 0.5);
				return true;
			}
		}

		return false;
	}

	inline bool ParseLevelTableRow(
		const TSharedPtr<FJsonObject>& JsonObject,
		int32& OutLevel,
		int64& OutTotalExperience)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		double NumericLevel = 0.0;
		if (!JsonObject->TryGetNumberField(TEXT("level"), NumericLevel))
		{
			return false;
		}

		int64 TotalExperience = 0;
		if (!TryReadInt64Field(
			JsonObject,
			{ TEXT("total_experience"), TEXT("total_xp"), TEXT("experience"), TEXT("required_total_experience") },
			TotalExperience))
		{
			return false;
		}

		OutLevel = FMath::RoundToInt(NumericLevel);
		OutTotalExperience = FMath::Max<int64>(0, TotalExperience);
		return OutLevel >= 1;
	}

	inline bool ParseLevelReward(
		const TSharedPtr<FJsonObject>& JsonObject,
		FTunaSweeperExperienceLevelReward& OutReward)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		double NumericLevel = 0.0;
		if (!JsonObject->TryGetNumberField(TEXT("level"), NumericLevel))
		{
			return false;
		}

		OutReward = FTunaSweeperExperienceLevelReward();
		OutReward.Level = FMath::RoundToInt(NumericLevel);
		TryReadDataValueField(
			JsonObject,
			{ TEXT("max_health_increase"), TEXT("max_health_bonus"), TEXT("maxHealthIncrease") },
			OutReward.MaxHealthIncrease);
		TryReadDataValueField(
			JsonObject,
			{ TEXT("max_food_increase"), TEXT("max_fullness_increase"), TEXT("max_hunger_increase"), TEXT("maxFoodIncrease") },
			OutReward.MaxFoodIncrease);
		TryReadDataValueField(
			JsonObject,
			{ TEXT("max_hydration_increase"), TEXT("max_water_increase"), TEXT("maxHydrationIncrease") },
			OutReward.MaxHydrationIncrease);
		TryReadDataValueField(
			JsonObject,
			{ TEXT("max_stamina_increase"), TEXT("maxStaminaIncrease") },
			OutReward.MaxStaminaIncrease);
		double NumericCarryStrengthIncrease = 0.0;
		if (JsonObject->TryGetNumberField(TEXT("carry_strength_increase"), NumericCarryStrengthIncrease) ||
			JsonObject->TryGetNumberField(TEXT("strength_increase"), NumericCarryStrengthIncrease))
		{
			OutReward.CarryStrengthIncrease = static_cast<float>(NumericCarryStrengthIncrease);
		}
		OutReward.Normalize();
		return OutReward.Level >= 2;
	}
}

namespace TunaSweeperProjectileHitEffects
{
	inline const TCHAR* DefaultDataAssetPath = TEXT("/Game/Effects/DA_ProjectileHitEffects.DA_ProjectileHitEffects");
}

namespace TunaSweeperLanguage
{
	inline const TCHAR* SectionName = TEXT("TunaSweeper.InterfaceSettings");
	inline const TCHAR* LanguageKey = TEXT("Language");
	inline const TCHAR* EnglishCode = TEXT("en");
	inline const TCHAR* KoreanCode = TEXT("ko");
	inline const TCHAR* JapaneseCode = TEXT("ja");

	inline const TCHAR* ToLanguageCode(ETunaSweeperItemTextLanguage Language)
	{
		switch (Language)
		{
		case ETunaSweeperItemTextLanguage::Korean:
			return KoreanCode;
		case ETunaSweeperItemTextLanguage::Japanese:
			return JapaneseCode;
		case ETunaSweeperItemTextLanguage::English:
		default:
			return EnglishCode;
		}
	}

	inline bool TryParseLanguageCode(const FString& LanguageCode, ETunaSweeperItemTextLanguage& OutLanguage)
	{
		const FString NormalizedCode = LanguageCode.TrimStartAndEnd().ToLower();
		if (NormalizedCode.StartsWith(KoreanCode))
		{
			OutLanguage = ETunaSweeperItemTextLanguage::Korean;
			return true;
		}

		if (NormalizedCode.StartsWith(JapaneseCode))
		{
			OutLanguage = ETunaSweeperItemTextLanguage::Japanese;
			return true;
		}

		if (NormalizedCode.StartsWith(EnglishCode))
		{
			OutLanguage = ETunaSweeperItemTextLanguage::English;
			return true;
		}

		return false;
	}
}

namespace TunaSweeperMapMarkers
{
	constexpr int32 MaxMarkerIconIndex = 3;
	constexpr int32 MaxMarkerColorIndex = 5;

	inline FVector2D ClampMapPosition(const FVector2D& MapPosition)
	{
		return FVector2D(
			FMath::Clamp(MapPosition.X, 0.0, 1.0),
			FMath::Clamp(MapPosition.Y, 0.0, 1.0));
	}

	inline FTunaSweeperMapMarkerSaveData SanitizeMarker(const FTunaSweeperMapMarkerSaveData& Marker)
	{
		FTunaSweeperMapMarkerSaveData SanitizedMarker = Marker;
		SanitizedMarker.MapPosition = ClampMapPosition(SanitizedMarker.MapPosition);
		SanitizedMarker.MarkerIconIndex = FMath::Clamp(SanitizedMarker.MarkerIconIndex, 0, MaxMarkerIconIndex);
		SanitizedMarker.MarkerColorIndex = FMath::Clamp(SanitizedMarker.MarkerColorIndex, 0, MaxMarkerColorIndex);
		return SanitizedMarker;
	}
}

namespace TunaSweeperInventory
{
	constexpr int32 RequiredBareInventorySlots = 40;
	constexpr int32 RequiredMaxInventorySlots = 100;
	constexpr int32 RequiredEquipmentSlots = 8;
	constexpr int32 BackpackSlotIndex = 7;
	constexpr int32 WeaponEquipmentSlotCount = 2;
	constexpr int32 MeleeEquipmentSlotIndex = 2;
	constexpr int32 UsableQuickSlotCount = 6;
	constexpr int32 DefaultStorageSlotCount = 100;
	constexpr int32 MaxStorageSlotCount = 1000;
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
	const FName ConsumableCategoryTag(TEXT("item.category.consumable"));
	const FName ThrowableCategoryTag(TEXT("item.category.throwable"));
	const FName AmmoCategoryTag(TEXT("item.category.ammo"));
	const FName PistolWeaponTypeTag(TEXT("weapon.type.pistol"));
	const FName RifleWeaponTypeTag(TEXT("weapon.type.rifle"));
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));
	const FName SmgWeaponTypeTag(TEXT("weapon.type.smg"));
	const FName PistolAmmoTypeTag(TEXT("ammo.type.pistol"));
	const FName RifleAmmoTypeTag(TEXT("ammo.type.rifle"));
	const FName ShotgunAmmoTypeTag(TEXT("ammo.type.shotgun"));
	const FName MagazineAttachmentSlotTag(TEXT("attachment.slot.magazine"));
	const FName OpticAttachmentSlotTag(TEXT("attachment.slot.optic"));
	constexpr int32 DefaultWeaponMagazineCapacity = 12;
	constexpr float DefaultWeaponReloadSeconds = 1.8f;
	constexpr float DefaultItemUseSeconds = 1.25f;

	struct FEquipmentSlotRule
	{
		FName CategoryTag;
		FName EquipmentSlotTag;
	};

	inline const FEquipmentSlotRule* GetEquipmentSlotRule(int32 SlotIndex)
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

	inline int32 ClampSlotCount(int32 SlotCount, int32 MinSlots, int32 MaxSlots)
	{
		return FMath::Clamp(SlotCount, FMath::Max(1, MinSlots), FMath::Max(MinSlots, MaxSlots));
	}

	inline FName GetDefaultAmmoTypeTagForWeaponType(FName WeaponTypeTag)
	{
		if (WeaponTypeTag == PistolWeaponTypeTag)
		{
			return PistolAmmoTypeTag;
		}

		if (WeaponTypeTag == RifleWeaponTypeTag)
		{
			return RifleAmmoTypeTag;
		}

		if (WeaponTypeTag == ShotgunWeaponTypeTag)
		{
			return ShotgunAmmoTypeTag;
		}

		if (WeaponTypeTag == SmgWeaponTypeTag)
		{
			return PistolAmmoTypeTag;
		}

		return NAME_None;
	}

	inline void NormalizeLoadedAmmoPersistenceFields(FTunaSweeperItemInstance& ItemInstance)
	{
		ItemInstance.LoadedAmmoCount = FMath::Max(0, ItemInstance.LoadedAmmoCount);

		if (ItemInstance.LoadedAmmoItemId == INDEX_NONE)
		{
			if (ItemInstance.SelectedAmmoItemId != INDEX_NONE)
			{
				ItemInstance.LoadedAmmoItemId = ItemInstance.SelectedAmmoItemId;
			}
			else
			{
				ItemInstance.LoadedAmmoCount = 0;
			}
		}

		ItemInstance.SelectedAmmoItemId = ItemInstance.LoadedAmmoItemId;
	}

	inline FTunaSweeperItemInstance MakeItemInstanceForSave(const FTunaSweeperItemInstance& ItemInstance)
	{
		FTunaSweeperItemInstance SaveItemInstance = ItemInstance;
		NormalizeLoadedAmmoPersistenceFields(SaveItemInstance);
		return SaveItemInstance;
	}
}

namespace TunaSweeperWeaponSpreadRecoil
{
	inline const TCHAR* DefaultDataAssetPath = TEXT("/Game/Weapons/DA_WeaponSpreadRecoil.DA_WeaponSpreadRecoil");

	inline void NormalizeDefinition(FTunaSweeperWeaponSpreadRecoilDefinition& Definition)
	{
		Definition.IncreasePerShot = FMath::Max(0.0f, Definition.IncreasePerShot);
		Definition.MinimumSpreadHalfAngleDegrees = FMath::Max(0.01f, Definition.MinimumSpreadHalfAngleDegrees);
		Definition.MaximumSpreadHalfAngleDegrees = FMath::Max(
			Definition.MinimumSpreadHalfAngleDegrees,
			Definition.MaximumSpreadHalfAngleDegrees);
		Definition.AimedSpreadMultiplier = Definition.AimedSpreadMultiplier > 0.0f
			? FMath::Max(0.01f, Definition.AimedSpreadMultiplier)
			: FTunaSweeperWeaponSpreadRecoilDefinition().AimedSpreadMultiplier;
		Definition.DecreasePerSecond = FMath::Max(0.0f, Definition.DecreasePerSecond);
	}

	inline bool TryGetFallbackDefinition(FName WeaponTypeTag, FTunaSweeperWeaponSpreadRecoilDefinition& OutDefinition)
	{
		OutDefinition = FTunaSweeperWeaponSpreadRecoilDefinition();
		OutDefinition.WeaponTypeTag = WeaponTypeTag;

		if (WeaponTypeTag == TunaSweeperInventory::PistolWeaponTypeTag)
		{
			OutDefinition.IncreasePerShot = 1.2f;
			OutDefinition.MinimumSpreadHalfAngleDegrees = 1.4f;
			OutDefinition.MaximumSpreadHalfAngleDegrees = 7.0f;
			OutDefinition.AimedSpreadMultiplier = 0.5f;
			OutDefinition.DecreasePerSecond = 5.0f;
			return true;
		}

		if (WeaponTypeTag == TunaSweeperInventory::RifleWeaponTypeTag)
		{
			OutDefinition.IncreasePerShot = 0.8f;
			OutDefinition.MinimumSpreadHalfAngleDegrees = 1.0f;
			OutDefinition.MaximumSpreadHalfAngleDegrees = 6.0f;
			OutDefinition.AimedSpreadMultiplier = 0.45f;
			OutDefinition.DecreasePerSecond = 6.5f;
			return true;
		}

		if (WeaponTypeTag == TunaSweeperInventory::ShotgunWeaponTypeTag)
		{
			OutDefinition.IncreasePerShot = 2.0f;
			OutDefinition.MinimumSpreadHalfAngleDegrees = 4.5f;
			OutDefinition.MaximumSpreadHalfAngleDegrees = 12.0f;
			OutDefinition.AimedSpreadMultiplier = 0.65f;
			OutDefinition.DecreasePerSecond = 4.5f;
			return true;
		}

		if (WeaponTypeTag == TunaSweeperInventory::SmgWeaponTypeTag)
		{
			OutDefinition.IncreasePerShot = 1.0f;
			OutDefinition.MinimumSpreadHalfAngleDegrees = 1.8f;
			OutDefinition.MaximumSpreadHalfAngleDegrees = 8.5f;
			OutDefinition.AimedSpreadMultiplier = 0.5f;
			OutDefinition.DecreasePerSecond = 7.5f;
			return true;
		}

		OutDefinition.IncreasePerShot = 1.0f;
		OutDefinition.MinimumSpreadHalfAngleDegrees = 1.0f;
		OutDefinition.MaximumSpreadHalfAngleDegrees = 8.0f;
		OutDefinition.AimedSpreadMultiplier = 0.5f;
		OutDefinition.DecreasePerSecond = 5.0f;
		return true;
	}
}

