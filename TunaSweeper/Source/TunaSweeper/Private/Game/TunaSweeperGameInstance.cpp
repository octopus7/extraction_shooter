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

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperGameInstance, Log, All);

namespace TunaSweeperSave
{
	const TCHAR* SaveSlotNamePrefix = TEXT("TunaSweeperSave_Slot");
	const TCHAR* SaveSettingsSlotName = TEXT("TunaSweeperSaveSettings");
	constexpr int32 CurrentSaveVersion = 17;
	constexpr int32 SaveUserIndex = 0;
	constexpr int32 MinSaveSlotIndex = 1;
	constexpr int32 MaxSaveSlotIndex = 3;
	constexpr int32 MinDifficultyStage = 1;
	constexpr int32 MaxDifficultyStage = 3;
	constexpr int32 DefaultDifficultyStage = 1;
	constexpr int32 MaxSaveGameBackupCount = 30;

	int32 SanitizeDifficultyStage(int32 DifficultyStage)
	{
		return FMath::Clamp(DifficultyStage, MinDifficultyStage, MaxDifficultyStage);
	}
}

namespace TunaSweeperShop
{
	FName MakeStockKey(int32 ShopId, int32 SlotIndex, int32 ItemId)
	{
		return FName(*FString::Printf(TEXT("%d:%d:%d"), ShopId, SlotIndex, ItemId));
	}

	bool IsValidShopSlotKey(int32 ShopId, int32 SlotIndex, int32 ItemId)
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
	const TCHAR* LevelTableJsonRelativePath = TEXT("Data/ExperienceLevelTable.json");
	const TCHAR* LevelRewardsJsonRelativePath = TEXT("Data/ExperienceLevelRewards.json");
	constexpr int32 DefaultMaxExperienceLevel = 30;
	constexpr int64 BaseExperienceForNextLevel = 100;
	constexpr int64 ExperienceIncreasePerLevel = 50;
	constexpr float RaidReturnAnimationDurationSeconds = 3.2f;

	bool TryReadDataValueField(
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

	bool TryReadInt64Field(
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

	bool ParseLevelTableRow(
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

	bool ParseLevelReward(
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
	const TCHAR* DefaultDataAssetPath = TEXT("/Game/Effects/DA_ProjectileHitEffects.DA_ProjectileHitEffects");
}

namespace TunaSweeperLanguage
{
	const TCHAR* SectionName = TEXT("TunaSweeper.InterfaceSettings");
	const TCHAR* LanguageKey = TEXT("Language");
	const TCHAR* EnglishCode = TEXT("en");
	const TCHAR* KoreanCode = TEXT("ko");
	const TCHAR* JapaneseCode = TEXT("ja");

	const TCHAR* ToLanguageCode(ETunaSweeperItemTextLanguage Language)
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

	bool TryParseLanguageCode(const FString& LanguageCode, ETunaSweeperItemTextLanguage& OutLanguage)
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

	FVector2D ClampMapPosition(const FVector2D& MapPosition)
	{
		return FVector2D(
			FMath::Clamp(MapPosition.X, 0.0, 1.0),
			FMath::Clamp(MapPosition.Y, 0.0, 1.0));
	}

	FTunaSweeperMapMarkerSaveData SanitizeMarker(const FTunaSweeperMapMarkerSaveData& Marker)
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

	FName GetDefaultAmmoTypeTagForWeaponType(FName WeaponTypeTag)
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

	void NormalizeLoadedAmmoPersistenceFields(FTunaSweeperItemInstance& ItemInstance)
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

	FTunaSweeperItemInstance MakeItemInstanceForSave(const FTunaSweeperItemInstance& ItemInstance)
	{
		FTunaSweeperItemInstance SaveItemInstance = ItemInstance;
		NormalizeLoadedAmmoPersistenceFields(SaveItemInstance);
		return SaveItemInstance;
	}
}

namespace TunaSweeperWeaponSpreadRecoil
{
	const TCHAR* DefaultDataAssetPath = TEXT("/Game/Weapons/DA_WeaponSpreadRecoil.DA_WeaponSpreadRecoil");

	void NormalizeDefinition(FTunaSweeperWeaponSpreadRecoilDefinition& Definition)
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

	bool TryGetFallbackDefinition(FName WeaponTypeTag, FTunaSweeperWeaponSpreadRecoilDefinition& OutDefinition)
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

void FTunaSweeperExperienceLevelStatBonuses::ClampNonNegative()
{
	MaxHealthBonus = FMath::Max(0.0f, MaxHealthBonus);
	MaxFoodBonus = FMath::Max(0.0f, MaxFoodBonus);
	MaxHydrationBonus = FMath::Max(0.0f, MaxHydrationBonus);
	MaxStaminaBonus = FMath::Max(0.0f, MaxStaminaBonus);
	CarryStrengthBonus = FMath::Max(0.0f, CarryStrengthBonus);
}

void FTunaSweeperExperienceLevelReward::Normalize()
{
	Level = FMath::Max(2, Level);
	MaxHealthIncrease = TunaSweeperDataValues::ClampRatioValue(MaxHealthIncrease);
	MaxFoodIncrease = TunaSweeperDataValues::ClampRatioValue(MaxFoodIncrease);
	MaxHydrationIncrease = TunaSweeperDataValues::ClampRatioValue(MaxHydrationIncrease);
	MaxStaminaIncrease = TunaSweeperDataValues::ClampRatioValue(MaxStaminaIncrease);
	CarryStrengthIncrease = FMath::Max(0.0f, CarryStrengthIncrease);
}

void FTunaSweeperPlayerHudState::NormalizeWeightLimits()
{
	CurrentCarryWeight = FMath::Max(0.0f, CurrentCarryWeight);
	MaxCarryWeight = FMath::Max(1.0f, MaxCarryWeight);
	OverweightThreshold = TunaSweeperDataValues::ClampRatioValue(OverweightThreshold);
	OverweightSpeedMultiplier = TunaSweeperDataValues::ClampRatioValue(OverweightSpeedMultiplier);
	OverweightCarryWeight = OverweightCarryWeight > 0.0f
		? OverweightCarryWeight
		: MaxCarryWeight * TunaSweeperDataValues::ToRatioFloat(OverweightThreshold);
	OverweightCarryWeight = FMath::Clamp(OverweightCarryWeight, 0.0f, MaxCarryWeight);
	MovementBlockedWeight = FMath::Max(1.0f, MovementBlockedWeight);
	if (MovementBlockedWeight < OverweightCarryWeight)
	{
		MovementBlockedWeight = OverweightCarryWeight;
	}
	Health = FMath::Clamp(Health, 0.0f, 100.0f);
	Food = FMath::Clamp(Food, 0.0f, 100.0f);
	Hydration = FMath::Clamp(Hydration, 0.0f, 100.0f);
}

bool FTunaSweeperPlayerHudState::IsCarryWeightOverLimit() const
{
	return OverweightCarryWeight > 0.0f && CurrentCarryWeight >= OverweightCarryWeight;
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

	return IsCarryWeightOverLimit()
		? TunaSweeperDataValues::ToRatioFloat(TunaSweeperDataValues::ClampRatioValue(OverweightSpeedMultiplier))
		: 1.0f;
}

float FTunaSweeperPlayerHudState::GetOverweightThresholdRatio() const
{
	return MaxCarryWeight > 0.0f
		? FMath::Clamp(OverweightCarryWeight / MaxCarryWeight, 0.0f, 1.0f)
		: 0.0f;
}

void UTunaSweeperGameInstance::Init()
{
	Super::Init();
	InitializeGlobalLanguageSetting();

	int32 LoadedSaveSlotIndex = 1;
	if (LoadActiveSaveSlotSelection(LoadedSaveSlotIndex))
	{
		ActiveSaveSlotIndex = SanitizeSaveSlotIndex(LoadedSaveSlotIndex);
	}
	else
	{
		ActiveSaveSlotIndex = FindFirstExistingSaveSlotIndex();
		SaveActiveSaveSlotSelection();
	}

	ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
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

float UTunaSweeperGameInstance::GetDialogueCharactersPerSecond() const
{
	if (const float* RuntimeCharactersPerSecond = NumberSettings.Find(TunaSweeperDialogue::CharactersPerSecondSettingKey))
	{
		return FMath::Max(0.1f, *RuntimeCharactersPerSecond);
	}

	return FMath::Max(0.1f, GameplaySettings.DialogueCharactersPerSecond);
}

void UTunaSweeperGameInstance::SetCurrentTextLanguage(ETunaSweeperItemTextLanguage Language, bool bSaveImmediately)
{
	const bool bLanguageChanged = CurrentTextLanguage != Language;
	CurrentTextLanguage = Language;
	ApplyCurrentLanguageCulture();

	if (bSaveImmediately)
	{
		SaveGlobalLanguageSetting();
	}

	if (bLanguageChanged)
	{
		OnLanguageChanged.Broadcast();
	}
}

FText UTunaSweeperGameInstance::ResolveLocalizedText(FName StringKey, const FText& FallbackText) const
{
	if (StringKey.IsNone())
	{
		return FallbackText;
	}

	if (const UTunaSweeperTextSubsystem* TextSubsystem = GetSubsystem<UTunaSweeperTextSubsystem>())
	{
		return TextSubsystem->ResolveText(StringKey, CurrentTextLanguage, FallbackText);
	}

	return FallbackText;
}

void UTunaSweeperGameInstance::SetVisionDebugEnabled(bool bEnabled)
{
	SetBoolSetting(TunaSweeperDebug::VisionDebugSettingKey, bEnabled);
}

bool UTunaSweeperGameInstance::IsVisionDebugEnabled() const
{
	if (const bool* RuntimeVisionDebug = BoolSettings.Find(TunaSweeperDebug::VisionDebugSettingKey))
	{
		return *RuntimeVisionDebug;
	}

	return GameplaySettings.bEnableVisionDebug;
}

void UTunaSweeperGameInstance::SetBunkerVisionDebugEnabled(bool bEnabled)
{
	SetBoolSetting(TunaSweeperDebug::BunkerVisionDebugSettingKey, bEnabled);
}

bool UTunaSweeperGameInstance::IsBunkerVisionDebugEnabled() const
{
	if (const bool* RuntimeBunkerVisionDebug = BoolSettings.Find(TunaSweeperDebug::BunkerVisionDebugSettingKey))
	{
		return *RuntimeBunkerVisionDebug;
	}

	return GameplaySettings.bEnableBunkerVisionDebug;
}

bool UTunaSweeperGameInstance::TryGetProjectileHitEffectDefinition(
	FName EffectId,
	FTunaSweeperProjectileHitEffectDefinition& OutDefinition) const
{
	OutDefinition = FTunaSweeperProjectileHitEffectDefinition();
	if (EffectId.IsNone())
	{
		return false;
	}

	const UTunaSweeperProjectileHitEffectDataAsset* DataAsset = ProjectileHitEffectDataAsset.LoadSynchronous();
	if (!DataAsset)
	{
		DataAsset = LoadObject<UTunaSweeperProjectileHitEffectDataAsset>(
			nullptr,
			TunaSweeperProjectileHitEffects::DefaultDataAssetPath);
	}

	return DataAsset && DataAsset->TryGetHitEffect(EffectId, OutDefinition);
}

bool UTunaSweeperGameInstance::TryGetWeaponSpreadRecoilDefinition(
	FName WeaponTypeTag,
	FTunaSweeperWeaponSpreadRecoilDefinition& OutDefinition) const
{
	OutDefinition = FTunaSweeperWeaponSpreadRecoilDefinition();
	if (WeaponTypeTag.IsNone())
	{
		return false;
	}

	const UTunaSweeperWeaponSpreadRecoilDataAsset* DataAsset = WeaponSpreadRecoilDataAsset.LoadSynchronous();
	if (!DataAsset)
	{
		DataAsset = LoadObject<UTunaSweeperWeaponSpreadRecoilDataAsset>(
			nullptr,
			TunaSweeperWeaponSpreadRecoil::DefaultDataAssetPath);
	}

	if (DataAsset && DataAsset->TryGetDefinition(WeaponTypeTag, OutDefinition))
	{
		TunaSweeperWeaponSpreadRecoil::NormalizeDefinition(OutDefinition);
		return true;
	}

	if (TunaSweeperWeaponSpreadRecoil::TryGetFallbackDefinition(WeaponTypeTag, OutDefinition))
	{
		TunaSweeperWeaponSpreadRecoil::NormalizeDefinition(OutDefinition);
		return true;
	}

	return false;
}

void UTunaSweeperGameInstance::ClearRuntimeState()
{
	ResetRuntimeStateForSaveSlotSelection();
	EnsureInventoryStateInitialized();
}

FTunaSweeperSaveSlotSummary UTunaSweeperGameInstance::GetSaveSlotSummary(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	Summary.SaveSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);

	const FString ExistingSlotName = GetExistingSaveGameSlotName(Summary.SaveSlotIndex);
	if (ExistingSlotName.IsEmpty())
	{
		return Summary;
	}

	UTunaSweeperSaveGame* SaveGame = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromSlot(
		ExistingSlotName,
		TunaSweeperSave::SaveUserIndex));
	if (!SaveGame)
	{
		return Summary;
	}

	Summary.bHasData = true;
	Summary.TotalPlaySeconds = FMath::Max(0.0f, SaveGame->TotalPlaySeconds);
	Summary.DifficultyStage = TunaSweeperSave::SanitizeDifficultyStage(SaveGame->DifficultyStage);
	Summary.LastSavedAtTicks = SaveGame->LastSavedAtTicks;
	return Summary;
}

bool UTunaSweeperGameInstance::ActivateSaveSlot(int32 SaveSlotIndex, bool bStartNewGame)
{
	SetActiveSaveSlotIndex(SaveSlotIndex);

	if (bStartNewGame)
	{
		LoadedSlotTotalPlaySeconds = 0.0f;
		ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
		ActiveSaveSlotDifficultyStage = TunaSweeperSave::DefaultDifficultyStage;
		GenerateDefaultInventoryState();
		bInventoryStateInitialized = true;
		RefreshLegacyPlayerInventoryItems();
		if (SaveGameStateInternal(EUsableQuickSlotSaveMode::Clear))
		{
			bPendingBunkerItemStateSave = false;
		}
		return true;
	}

	EnsureInventoryStateInitialized();
	return true;
}

bool UTunaSweeperGameInstance::SetActiveSaveSlotIndex(int32 SaveSlotIndex)
{
	ActiveSaveSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	ResetRuntimeStateForSaveSlotSelection();
	return SaveActiveSaveSlotSelection();
}

bool UTunaSweeperGameInstance::DeleteSaveSlot(int32 SaveSlotIndex)
{
	const int32 SanitizedSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	const FString SlotName = GetSaveGameSlotName(SanitizedSlotIndex);
	const bool bDeleted = UGameplayStatics::DoesSaveGameExist(SlotName, TunaSweeperSave::SaveUserIndex) &&
		UGameplayStatics::DeleteGameInSlot(SlotName, TunaSweeperSave::SaveUserIndex);

	if (ActiveSaveSlotIndex == SanitizedSlotIndex)
	{
		ResetRuntimeStateForSaveSlotSelection();
		ActiveSaveSlotIndex = SanitizedSlotIndex;
	}

	return bDeleted;
}

bool UTunaSweeperGameInstance::DeleteSaveSlotAndStartNewGame(int32 SaveSlotIndex)
{
	const int32 SanitizedSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	const FString SlotName = GetSaveGameSlotName(SanitizedSlotIndex);
	if (UGameplayStatics::DoesSaveGameExist(SlotName, TunaSweeperSave::SaveUserIndex) &&
		!UGameplayStatics::DeleteGameInSlot(SlotName, TunaSweeperSave::SaveUserIndex))
	{
		return false;
	}

	ResetRuntimeStateForSaveSlotSelection();
	return ActivateSaveSlot(SanitizedSlotIndex, true);
}

bool UTunaSweeperGameInstance::IsScenarioProgressFlagSet(FName ScenarioFlag) const
{
	return !ScenarioFlag.IsNone() && CompletedScenarioFlags.Contains(ScenarioFlag);
}

void UTunaSweeperGameInstance::MarkScenarioProgressFlag(FName ScenarioFlag, bool bSaveImmediately)
{
	if (ScenarioFlag.IsNone())
	{
		return;
	}

	EnsureInventoryStateInitialized();
	CompletedScenarioFlags.Add(ScenarioFlag);

	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
}

FName UTunaSweeperGameInstance::ResolveInitialGameplayLevelName()
{
	EnsureInventoryStateInitialized();
	return IsScenarioProgressFlagSet(TunaSweeperScenario::OpeningScenarioFlag)
		? TunaSweeperScenario::BunkerMapName
		: TunaSweeperScenario::OpeningScenarioMapName;
}

void UTunaSweeperGameInstance::BeginScenarioBunkerEntry(FName ScenarioFlag)
{
	PendingScenarioCompletionFlag = ScenarioFlag;
}

bool UTunaSweeperGameInstance::CompletePendingScenarioBunkerEntryIfNeeded()
{
	if (PendingScenarioCompletionFlag.IsNone())
	{
		return false;
	}

	const FName ScenarioFlag = PendingScenarioCompletionFlag;
	PendingScenarioCompletionFlag = NAME_None;
	MarkScenarioProgressFlag(ScenarioFlag, true);
	return true;
}

bool UTunaSweeperGameInstance::IsMemoAcquired(int32 MemoId)
{
	EnsureInventoryStateInitialized();
	return MemoId > 0 && AcquiredMemoIds.Contains(MemoId);
}

bool UTunaSweeperGameInstance::MarkMemoAcquired(int32 MemoId, bool bSaveImmediately)
{
	if (MemoId <= 0)
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	const int32 PreviousCount = AcquiredMemoIds.Num();
	AcquiredMemoIds.Add(MemoId);
	if (AcquiredMemoIds.Num() == PreviousCount)
	{
		return false;
	}

	OnMemoStateChanged.Broadcast();
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return true;
}

void UTunaSweeperGameInstance::GetAcquiredMemoIds(TArray<int32>& OutMemoIds)
{
	EnsureInventoryStateInitialized();
	OutMemoIds = AcquiredMemoIds.Array();
	OutMemoIds.Sort();
}

bool UTunaSweeperGameInstance::HasEverAcquiredItem(int32 ItemId)
{
	EnsureInventoryStateInitialized();
	return ItemId != INDEX_NONE && EverAcquiredItemIds.Contains(ItemId);
}

void UTunaSweeperGameInstance::GetMapMarkers(TArray<FTunaSweeperMapMarkerSaveData>& OutMapMarkers)
{
	EnsureInventoryStateInitialized();
	OutMapMarkers = MapMarkers;
	OutMapMarkers.Sort([](
		const FTunaSweeperMapMarkerSaveData& Left,
		const FTunaSweeperMapMarkerSaveData& Right)
	{
		return Left.MarkerId < Right.MarkerId;
	});
}

int32 UTunaSweeperGameInstance::AddMapMarker(
	const FVector2D& MapPosition,
	int32 MarkerIconIndex,
	int32 MarkerColorIndex,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperMapMarkerSaveData NewMarker;
	NewMarker.MarkerId = FMath::Max(1, NextMapMarkerId++);
	NewMarker.MapPosition = MapPosition;
	NewMarker.MarkerIconIndex = MarkerIconIndex;
	NewMarker.MarkerColorIndex = MarkerColorIndex;
	NewMarker = TunaSweeperMapMarkers::SanitizeMarker(NewMarker);

	MapMarkers.Add(NewMarker);
	OnMapMarkersChanged.Broadcast();
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}

	return NewMarker.MarkerId;
}

bool UTunaSweeperGameInstance::RemoveMapMarker(int32 MarkerId, bool bSaveImmediately)
{
	if (MarkerId <= 0)
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	const int32 RemovedCount = MapMarkers.RemoveAll([MarkerId](const FTunaSweeperMapMarkerSaveData& Marker)
	{
		return Marker.MarkerId == MarkerId;
	});
	if (RemovedCount <= 0)
	{
		return false;
	}

	OnMapMarkersChanged.Broadcast();
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return true;
}

int32 UTunaSweeperGameInstance::GetCurrentExperienceLevel() const
{
	return GetExperienceLevelForTotal(TotalExperiencePoints);
}

int32 UTunaSweeperGameInstance::GetMaxExperienceLevel() const
{
	EnsureExperienceLevelTableLoaded();
	return FMath::Max(1, CachedExperienceForLevels.Num());
}

int32 UTunaSweeperGameInstance::GetExperienceLevelForTotal(int64 ExperiencePoints) const
{
	EnsureExperienceLevelTableLoaded();

	const int64 ClampedExperience = FMath::Max<int64>(0, ExperiencePoints);
	int32 Level = 1;
	for (int32 LevelIndex = 1; LevelIndex < CachedExperienceForLevels.Num(); ++LevelIndex)
	{
		if (ClampedExperience < CachedExperienceForLevels[LevelIndex])
		{
			break;
		}

		Level = LevelIndex + 1;
	}

	return Level;
}

int64 UTunaSweeperGameInstance::GetExperienceForLevel(int32 Level) const
{
	EnsureExperienceLevelTableLoaded();

	if (CachedExperienceForLevels.Num() <= 0)
	{
		return 0;
	}

	const int32 LevelIndex = FMath::Clamp(Level, 1, CachedExperienceForLevels.Num()) - 1;
	return CachedExperienceForLevels[LevelIndex];
}

int64 UTunaSweeperGameInstance::GetExperienceForNextLevel(int32 Level) const
{
	EnsureExperienceLevelTableLoaded();

	const int32 SafeLevel = FMath::Clamp(Level, 1, FMath::Max(1, CachedExperienceForLevels.Num()));
	if (SafeLevel >= CachedExperienceForLevels.Num())
	{
		return 0;
	}

	return FMath::Max<int64>(
		0,
		CachedExperienceForLevels[SafeLevel] - CachedExperienceForLevels[SafeLevel - 1]);
}

float UTunaSweeperGameInstance::GetExperienceProgressForTotal(int64 ExperiencePoints) const
{
	const int64 ClampedExperience = FMath::Max<int64>(0, ExperiencePoints);
	const int32 Level = GetExperienceLevelForTotal(ClampedExperience);
	if (Level >= GetMaxExperienceLevel())
	{
		return 1.0f;
	}

	const int64 LevelStartExperience = GetExperienceForLevel(Level);
	const int64 NextLevelExperience = GetExperienceForLevel(Level + 1);
	const int64 LevelSpan = FMath::Max<int64>(1, NextLevelExperience - LevelStartExperience);
	return static_cast<float>(ClampedExperience - LevelStartExperience) / static_cast<float>(LevelSpan);
}

FTunaSweeperExperienceLevelStatBonuses UTunaSweeperGameInstance::GetExperienceLevelStatBonuses(int32 Level) const
{
	EnsureExperienceLevelRewardsLoaded();

	FTunaSweeperExperienceLevelStatBonuses Bonuses;
	const int32 TargetLevel = FMath::Max(1, Level);
	for (const FTunaSweeperExperienceLevelReward& Reward : CachedExperienceLevelRewards)
	{
		if (Reward.Level > TargetLevel)
		{
			break;
		}

		Bonuses.MaxHealthBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxHealthIncrease);
		Bonuses.MaxFoodBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxFoodIncrease);
		Bonuses.MaxHydrationBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxHydrationIncrease);
		Bonuses.MaxStaminaBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxStaminaIncrease);
		Bonuses.CarryStrengthBonus += FMath::Max(0.0f, Reward.CarryStrengthIncrease);
	}

	Bonuses.ClampNonNegative();
	return Bonuses;
}

FTunaSweeperExperienceLevelStatBonuses UTunaSweeperGameInstance::GetCurrentExperienceLevelStatBonuses() const
{
	return GetExperienceLevelStatBonuses(GetCurrentExperienceLevel());
}

void UTunaSweeperGameInstance::BeginRaidExperienceSession()
{
	EnsureInventoryStateInitialized();
	RaidStartExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
	PendingRaidExperiencePoints = 0;
	bRaidExperienceSessionActive = true;
	bHasPendingRaidExperienceAnimationState = false;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
}

int32 UTunaSweeperGameInstance::AddRaidExperience(int32 ExperienceAmount)
{
	if (ExperienceAmount <= 0)
	{
		return 0;
	}

	EnsureInventoryStateInitialized();
	if (!bRaidExperienceSessionActive)
	{
		const UWorld* World = GetWorld();
		if (!World || !IsMapNameMatch(FName(*World->GetMapName()), TEXT("RaidMap")))
		{
			return 0;
		}

		RaidStartExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
		PendingRaidExperiencePoints = 0;
		bRaidExperienceSessionActive = true;
	}

	PendingRaidExperiencePoints += ExperienceAmount;
	return ExperienceAmount;
}

int32 UTunaSweeperGameInstance::AddRaidExperienceForItem(int32 ItemId, int32 Quantity)
{
	const int32 ExperienceValue = ResolveItemExperienceValue(ItemId);
	const int32 SafeQuantity = FMath::Max(0, Quantity);
	if (ExperienceValue <= 0 || SafeQuantity <= 0)
	{
		return 0;
	}

	return AddRaidExperience(ExperienceValue * SafeQuantity);
}

void UTunaSweeperGameInstance::ClearRaidExperienceGain()
{
	PendingRaidExperiencePoints = 0;
	RaidStartExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
}

bool UTunaSweeperGameInstance::CommitRaidExperienceGain(FTunaSweeperExperienceAnimationState& OutAnimationState)
{
	EnsureInventoryStateInitialized();

	const int64 StartExperience = bRaidExperienceSessionActive
		? FMath::Max<int64>(0, RaidStartExperiencePoints)
		: FMath::Max<int64>(0, TotalExperiencePoints);
	const int64 GainedExperience = FMath::Max<int64>(0, PendingRaidExperiencePoints);
	TotalExperiencePoints = FMath::Max<int64>(StartExperience, TotalExperiencePoints) + GainedExperience;
	OutAnimationState = BuildExperienceAnimationState(
		StartExperience,
		TotalExperiencePoints,
		GainedExperience);
	if (GainedExperience > 0)
	{
		PendingRaidExperienceAnimationState = OutAnimationState;
		bHasPendingRaidExperienceAnimationState = true;
		RefreshCarryWeightState();
		OnExperienceChanged.Broadcast();
	}
	else
	{
		PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
		bHasPendingRaidExperienceAnimationState = false;
	}

	PendingRaidExperiencePoints = 0;
	RaidStartExperiencePoints = TotalExperiencePoints;
	bRaidExperienceSessionActive = false;
	return GainedExperience > 0;
}

bool UTunaSweeperGameInstance::ConsumePendingRaidExperienceAnimationState(
	FTunaSweeperExperienceAnimationState& OutAnimationState)
{
	if (!bHasPendingRaidExperienceAnimationState)
	{
		OutAnimationState = FTunaSweeperExperienceAnimationState();
		return false;
	}

	OutAnimationState = PendingRaidExperienceAnimationState;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bHasPendingRaidExperienceAnimationState = false;
	return true;
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
	PlayerHudState.OverweightCarryWeight = MaxCarryWeight * PlayerHudState.GetOverweightThresholdRatio();
	PlayerHudState.NormalizeWeightLimits();
}

void UTunaSweeperGameInstance::RefreshCarryWeightState()
{
	EnsureInventoryStateInitialized();

	FTunaSweeperCarryWeightDebuffSettings CarrySettings;
	if (UTunaSweeperDebuffDataSubsystem* DebuffDataSubsystem = GetSubsystem<UTunaSweeperDebuffDataSubsystem>())
	{
		CarrySettings = DebuffDataSubsystem->GetCarryWeightSettings();
	}
	CarrySettings.Normalize();

	const float MaxCarryWeight = CalculateMaxCarryWeight();
	const float MovementBlockedWeight = FMath::Max(
		1.0f,
		MaxCarryWeight * CarrySettings.GetMovementBlockedThresholdRatio());

	PlayerHudState.CurrentCarryWeight = CalculatePlayerCarryWeight();
	PlayerHudState.MaxCarryWeight = MaxCarryWeight;
	PlayerHudState.OverweightThreshold = CarrySettings.OverweightThreshold;
	PlayerHudState.OverweightSpeedMultiplier = CarrySettings.OverweightSpeedMultiplier;
	PlayerHudState.OverweightCarryWeight = MaxCarryWeight * CarrySettings.GetOverweightThresholdRatio();
	PlayerHudState.MovementBlockedWeight = MovementBlockedWeight;
	PlayerHudState.NormalizeWeightLimits();
}

float UTunaSweeperGameInstance::GetCarryWeightMovementSpeedMultiplier() const
{
	FTunaSweeperPlayerHudState NormalizedHudState = PlayerHudState;
	NormalizedHudState.NormalizeWeightLimits();
	return NormalizedHudState.GetCarryWeightMovementSpeedMultiplier();
}

bool UTunaSweeperGameInstance::IsCarryWeightOverLimit() const
{
	FTunaSweeperPlayerHudState NormalizedHudState = PlayerHudState;
	NormalizedHudState.NormalizeWeightLimits();
	return NormalizedHudState.IsCarryWeightOverLimit();
}

bool UTunaSweeperGameInstance::IsCarryWeightMovementBlocked() const
{
	FTunaSweeperPlayerHudState NormalizedHudState = PlayerHudState;
	NormalizedHudState.NormalizeWeightLimits();
	return NormalizedHudState.IsCarryWeightMovementBlocked();
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

void UTunaSweeperGameInstance::SetActiveShop(int32 ShopId)
{
	EnsureInventoryStateInitialized();

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopDefinition ShopDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetShopDefinition(ShopId, ShopDefinition))
	{
		ClearActiveShop();
		return;
	}

	ActiveShopId = ShopId;
	bHasActiveShop = true;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::ClearActiveShop()
{
	const bool bHadActiveShop = bHasActiveShop || ActiveShopId != INDEX_NONE;
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;

	if (bHadActiveShop)
	{
		BroadcastInventoryStateChanged();
	}
}

bool UTunaSweeperGameInstance::GetActiveShopItems(TArray<FTunaSweeperShopItemView>& OutShopItems)
{
	EnsureInventoryStateInitialized();
	OutShopItems.Reset();

	if (!bHasActiveShop || ActiveShopId == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopDefinition ShopDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetShopDefinition(ActiveShopId, ShopDefinition))
	{
		return false;
	}

	OutShopItems.Reserve(ShopDefinition.Items.Num());
	for (int32 SlotIndex = 0; SlotIndex < ShopDefinition.Items.Num(); ++SlotIndex)
	{
		const FTunaSweeperShopItemDefinition& ShopItemDefinition = ShopDefinition.Items[SlotIndex];
		FTunaSweeperShopItemView ShopItemView;
		ShopItemView.ShopId = ActiveShopId;
		ShopItemView.SlotIndex = SlotIndex;
		ShopItemView.ItemId = ShopItemDefinition.ItemId;
		ShopItemView.StockQuantity = GetShopStockQuantity(ActiveShopId, SlotIndex, ShopItemDefinition);
		ShopItemView.TotalStockQuantity = FMath::Max(0, ShopItemDefinition.StockQuantity);
		ShopItemView.Price = ItemDataSubsystem->ResolveShopItemBuyPrice(ShopItemDefinition);
		OutShopItems.Add(ShopItemView);
	}

	return OutShopItems.Num() > 0;
}

bool UTunaSweeperGameInstance::TryGetActiveShopItemView(
	int32 ShopSlotIndex,
	FTunaSweeperShopItemView& OutShopItem)
{
	EnsureInventoryStateInitialized();
	OutShopItem = FTunaSweeperShopItemView();

	if (!bHasActiveShop || ActiveShopId == INDEX_NONE || ShopSlotIndex == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopItemDefinition ShopItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetShopItemDefinition(ActiveShopId, ShopSlotIndex, ShopItemDefinition))
	{
		return false;
	}

	OutShopItem.ShopId = ActiveShopId;
	OutShopItem.SlotIndex = ShopSlotIndex;
	OutShopItem.ItemId = ShopItemDefinition.ItemId;
	OutShopItem.StockQuantity = GetShopStockQuantity(ActiveShopId, ShopSlotIndex, ShopItemDefinition);
	OutShopItem.TotalStockQuantity = FMath::Max(0, ShopItemDefinition.StockQuantity);
	OutShopItem.Price = ItemDataSubsystem->ResolveShopItemBuyPrice(ShopItemDefinition);
	return true;
}

bool UTunaSweeperGameInstance::TryBuyActiveShopSlot(int32 ShopSlotIndex)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperShopItemView ShopItemView;
	if (!TryGetActiveShopItemView(ShopSlotIndex, ShopItemView) ||
		ShopItemView.ItemId == INDEX_NONE ||
		ShopItemView.StockQuantity <= 0)
	{
		return false;
	}

	UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>();
	if (!QuestSubsystem || QuestSubsystem->GetCoinBalance() < ShopItemView.Price)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopItemDefinition ShopItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetShopItemDefinition(ShopItemView.ShopId, ShopItemView.SlotIndex, ShopItemDefinition))
	{
		return false;
	}

	if (!AddItemToFirstAvailableInventorySlot(ShopItemView.ItemId, 1))
	{
		return false;
	}

	SetShopStockQuantity(
		ShopItemView.ShopId,
		ShopItemView.SlotIndex,
		ShopItemDefinition,
		ShopItemView.StockQuantity - 1);

	if (ShopItemView.Price > 0)
	{
		QuestSubsystem->TrySpendCoins(ShopItemView.Price, false);
	}
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave(true);
	return true;
}

bool UTunaSweeperGameInstance::DebugRestockActiveShop(bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	if (!bHasActiveShop || ActiveShopId == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopDefinition ShopDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetShopDefinition(ActiveShopId, ShopDefinition))
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < ShopDefinition.Items.Num(); ++SlotIndex)
	{
		const FTunaSweeperShopItemDefinition& ShopItemDefinition = ShopDefinition.Items[SlotIndex];
		SetShopStockQuantity(
			ActiveShopId,
			SlotIndex,
			ShopItemDefinition,
			ShopItemDefinition.StockQuantity);
	}

	if (bSaveImmediately)
	{
		MarkItemStateMutationForSave(true);
	}
	BroadcastInventoryStateChanged();
	return true;
}

bool UTunaSweeperGameInstance::TryGetSlotSellPrice(
	const FTunaSweeperItemSlotReference& SlotReference,
	int32& OutSalePrice)
{
	EnsureInventoryStateInitialized();
	OutSalePrice = 0;

	if (!SlotReference.IsValid() || !IsSellableItemSlotSource(SlotReference.Source))
	{
		return false;
	}

	FTunaSweeperItemInstance ItemInstance;
	FTunaSweeperItemDefinition ItemDefinition;
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem ||
		!TryGetSlotItemInstance(SlotReference, ItemInstance) ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
	{
		return false;
	}

	OutSalePrice = FMath::Max(0, (FMath::Max(0, ItemDefinition.ShopSellPrice) * FMath::Max(1, ItemInstance.Quantity)) / 2);
	return true;
}

bool UTunaSweeperGameInstance::TrySellItemInSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	int32& OutSalePrice)
{
	EnsureInventoryStateInitialized();
	OutSalePrice = 0;

	if (!bHasActiveShop || !TryGetSlotSellPrice(SlotReference, OutSalePrice))
	{
		return false;
	}

	FTunaSweeperItemInstance RemovedItemInstance;
	if (!RemoveItemFromSlot(SlotReference, RemovedItemInstance))
	{
		return false;
	}

	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		if (OutSalePrice > 0)
		{
			QuestSubsystem->AddCoins(OutSalePrice, false);
		}
	}

	MarkItemStateMutationForSave(true);
	ClearSelectedItemSelection();
	ClearHoveredItemSlot(SlotReference);
	return true;
}

void UTunaSweeperGameInstance::SetActiveWorkbench(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	EnsureInventoryStateInitialized();

	if (WorkbenchId <= 0)
	{
		ClearActiveWorkbench();
		return;
	}

	ActiveWorkbenchId = WorkbenchId;
	ActiveWorkbenchMode = WorkbenchMode;
	bHasActiveWorkbench = true;
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::SetActiveWorkbenchMode(ETunaSweeperWorkbenchMode WorkbenchMode)
{
	if (!bHasActiveWorkbench)
	{
		return;
	}

	ActiveWorkbenchMode = WorkbenchMode;
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::ClearActiveWorkbench()
{
	const bool bHadActiveWorkbench = bHasActiveWorkbench || ActiveWorkbenchId != INDEX_NONE;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;

	if (bHadActiveWorkbench)
	{
		BroadcastInventoryStateChanged();
	}
}

bool UTunaSweeperGameInstance::GetActiveWorkbenchRecipes(TArray<FTunaSweeperWorkbenchRecipeView>& OutRecipeViews)
{
	EnsureInventoryStateInitialized();
	OutRecipeViews.Reset();

	if (!bHasActiveWorkbench || ActiveWorkbenchId == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperWorkbenchRecipeDefinition> RecipeDefinitions;
	if (!ItemDataSubsystem || !ItemDataSubsystem->GetWorkbenchRecipeDefinitions(ActiveWorkbenchId, RecipeDefinitions))
	{
		return false;
	}

	OutRecipeViews.Reserve(RecipeDefinitions.Num());
	for (const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition : RecipeDefinitions)
	{
		if (IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition))
		{
			OutRecipeViews.Add(BuildWorkbenchRecipeView(RecipeDefinition, OutRecipeViews.Num()));
		}
	}

	return OutRecipeViews.Num() > 0;
}

bool UTunaSweeperGameInstance::TryGetActiveWorkbenchRecipeView(
	int32 RecipeSlotIndex,
	FTunaSweeperWorkbenchRecipeView& OutRecipeView)
{
	EnsureInventoryStateInitialized();
	OutRecipeView = FTunaSweeperWorkbenchRecipeView();

	if (!bHasActiveWorkbench || ActiveWorkbenchId == INDEX_NONE || RecipeSlotIndex == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperWorkbenchRecipeDefinition> RecipeDefinitions;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->GetWorkbenchRecipeDefinitions(ActiveWorkbenchId, RecipeDefinitions))
	{
		return false;
	}

	TArray<FTunaSweeperWorkbenchRecipeDefinition> UnlockedRecipeDefinitions;
	for (const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition : RecipeDefinitions)
	{
		if (IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition))
		{
			UnlockedRecipeDefinitions.Add(RecipeDefinition);
		}
	}
	if (!UnlockedRecipeDefinitions.IsValidIndex(RecipeSlotIndex))
	{
		return false;
	}

	OutRecipeView = BuildWorkbenchRecipeView(UnlockedRecipeDefinitions[RecipeSlotIndex], RecipeSlotIndex);
	return true;
}

bool UTunaSweeperGameInstance::CanCraftActiveWorkbenchRecipe(int32 RecipeSlotIndex)
{
	FTunaSweeperWorkbenchRecipeView RecipeView;
	return TryGetActiveWorkbenchRecipeView(RecipeSlotIndex, RecipeView) && RecipeView.bCanCraft;
}

bool UTunaSweeperGameInstance::TryCraftActiveWorkbenchRecipe(int32 RecipeSlotIndex, bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperWorkbenchRecipeDefinition> RecipeDefinitions;
	if (!bHasActiveWorkbench ||
		ActiveWorkbenchId == INDEX_NONE ||
		RecipeSlotIndex == INDEX_NONE ||
		!ItemDataSubsystem ||
		!ItemDataSubsystem->GetWorkbenchRecipeDefinitions(ActiveWorkbenchId, RecipeDefinitions))
	{
		return false;
	}

	TArray<FTunaSweeperWorkbenchRecipeDefinition> UnlockedRecipeDefinitions;
	for (const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition : RecipeDefinitions)
	{
		if (IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition))
		{
			UnlockedRecipeDefinitions.Add(RecipeDefinition);
		}
	}
	if (!UnlockedRecipeDefinitions.IsValidIndex(RecipeSlotIndex))
	{
		return false;
	}

	const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition = UnlockedRecipeDefinitions[RecipeSlotIndex];
	const FTunaSweeperWorkbenchRecipeView RecipeView = BuildWorkbenchRecipeView(RecipeDefinition, RecipeSlotIndex);
	if (!RecipeView.bCanCraft || RecipeDefinition.OutputItemId == INDEX_NONE || RecipeDefinition.OutputQuantity <= 0)
	{
		return false;
	}

	const TMap<FGuid, FTunaSweeperItemInstance> PreviousItemInstances = ItemInstancesByUid;
	const TArray<FTunaSweeperInventorySlot> PreviousInventorySlots = PlayerInventorySlots;
	const TArray<FTunaSweeperInventorySlot> PreviousAuxiliaryBagSlots = AuxiliaryBagSlots;
	const TArray<FTunaSweeperInventorySlot> PreviousStorageSlots = StorageSlots;

	for (const FTunaSweeperWorkbenchIngredient& Ingredient : RecipeDefinition.Ingredients)
	{
		if (ConsumeWorkbenchIngredientItemById(Ingredient.ItemId, Ingredient.Quantity) < Ingredient.Quantity)
		{
			ItemInstancesByUid = PreviousItemInstances;
			PlayerInventorySlots = PreviousInventorySlots;
			AuxiliaryBagSlots = PreviousAuxiliaryBagSlots;
			StorageSlots = PreviousStorageSlots;
			return false;
		}
	}

	int32 RemainingOutputQuantity = RecipeDefinition.OutputQuantity;
	TryAddItemQuantityToExistingStacks(RecipeDefinition.OutputItemId, RemainingOutputQuantity, PlayerInventorySlots);
	TryAddItemQuantityToFirstEmptySlots(RecipeDefinition.OutputItemId, RemainingOutputQuantity, PlayerInventorySlots);
	if (RemainingOutputQuantity > 0)
	{
		ItemInstancesByUid = PreviousItemInstances;
		PlayerInventorySlots = PreviousInventorySlots;
		AuxiliaryBagSlots = PreviousAuxiliaryBagSlots;
		StorageSlots = PreviousStorageSlots;
		return false;
	}

	ClearSelectedItemIfInvalid();
	BroadcastInventoryStateChanged();
	MarkItemEverAcquired(RecipeDefinition.OutputItemId);
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->NotifyItemAcquired(
			RecipeDefinition.OutputItemId,
			RecipeDefinition.OutputQuantity,
			!IsCurrentWorldBunkerMap());
	}
	AddRaidExperienceForItem(RecipeDefinition.OutputItemId, RecipeDefinition.OutputQuantity);
	MarkItemStateMutationForSave(bSaveImmediately);
	return true;
}

bool UTunaSweeperGameInstance::GetActiveWorkbenchDismantleCandidates(
	TArray<FTunaSweeperWorkbenchDismantleCandidateView>& OutCandidateViews)
{
	EnsureInventoryStateInitialized();
	OutCandidateViews.Reset();
	if (!bHasActiveWorkbench)
	{
		return false;
	}

	auto AddCandidatesFromSlots = [this, &OutCandidateViews](ETunaSweeperItemSlotSource Source, const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			FTunaSweeperItemSlotReference SlotReference;
			SlotReference.Source = Source;
			SlotReference.SlotIndex = SlotIndex;

			FTunaSweeperWorkbenchDismantleCandidateView CandidateView;
			if (TryGetWorkbenchDismantleCandidateFromSlot(SlotReference, CandidateView))
			{
				CandidateView.ListIndex = OutCandidateViews.Num();
				OutCandidateViews.Add(CandidateView);
			}
		}
	};

	AddCandidatesFromSlots(ETunaSweeperItemSlotSource::Inventory, PlayerInventorySlots);
	AddCandidatesFromSlots(ETunaSweeperItemSlotSource::Storage, StorageSlots);
	return OutCandidateViews.Num() > 0;
}

bool UTunaSweeperGameInstance::TryGetWorkbenchDismantleCandidateFromSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	FTunaSweeperWorkbenchDismantleCandidateView& OutCandidateView)
{
	EnsureInventoryStateInitialized();
	OutCandidateView = FTunaSweeperWorkbenchDismantleCandidateView();
	if (!bHasActiveWorkbench ||
		!SlotReference.IsValid() ||
		!IsWorkbenchItemSlotSourceAllowedForDismantle(SlotReference.Source))
	{
		return false;
	}

	FTunaSweeperItemInstance ItemInstance;
	if (!TryGetSlotItemInstance(SlotReference, ItemInstance))
	{
		return false;
	}

	OutCandidateView.SlotReference = SlotReference;
	OutCandidateView.ItemId = ItemInstance.ItemId;
	OutCandidateView.Quantity = FMath::Max(1, ItemInstance.Quantity);

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperWorkbenchDismantleDefinition DismantleDefinition;
	if (ItemDataSubsystem &&
		ItemDataSubsystem->TryGetWorkbenchDismantleDefinition(ItemInstance.ItemId, DismantleDefinition))
	{
		OutCandidateView.Results = DismantleDefinition.Results;
		OutCandidateView.bCanDismantle = OutCandidateView.Results.Num() > 0;
	}

	return true;
}

bool UTunaSweeperGameInstance::TryDismantleWorkbenchItemInSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	TArray<FTunaSweeperItemStack>& OutOverflowItems,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	OutOverflowItems.Reset();

	FTunaSweeperWorkbenchDismantleCandidateView CandidateView;
	if (!TryGetWorkbenchDismantleCandidateFromSlot(SlotReference, CandidateView) || !CandidateView.bCanDismantle)
	{
		return false;
	}

	if (!TryConsumeSingleItemFromSlot(SlotReference))
	{
		return false;
	}

	for (const FTunaSweeperItemStack& ResultStack : CandidateView.Results)
	{
		AddWorkbenchResultToInventoryOrOverflow(ResultStack.ItemId, ResultStack.Quantity, OutOverflowItems);
	}

	ClearSelectedItemIfInvalid();
	ClearHoveredItemSlot(SlotReference);
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave(bSaveImmediately);
	return true;
}

bool UTunaSweeperGameInstance::GetActiveWorkbenchBlueprintItems(
	TArray<FTunaSweeperWorkbenchBlueprintItemView>& OutBlueprintItemViews)
{
	EnsureInventoryStateInitialized();
	OutBlueprintItemViews.Reset();
	if (!bHasActiveWorkbench)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem)
	{
		return false;
	}

	auto AddBlueprintsFromSlots = [this, ItemDataSubsystem, &OutBlueprintItemViews](ETunaSweeperItemSlotSource Source, const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			FTunaSweeperItemSlotReference SlotReference;
			SlotReference.Source = Source;
			SlotReference.SlotIndex = SlotIndex;

			FTunaSweeperItemInstance ItemInstance;
			FTunaSweeperItemDefinition ItemDefinition;
			if (!TryGetSlotItemInstance(SlotReference, ItemInstance) ||
				!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition) ||
				!IsWorkbenchBlueprintItemDefinition(ItemDefinition))
			{
				continue;
			}

			FTunaSweeperWorkbenchBlueprintItemView BlueprintItemView;
			BlueprintItemView.SlotReference = SlotReference;
			BlueprintItemView.ListIndex = OutBlueprintItemViews.Num();
			BlueprintItemView.ItemId = ItemInstance.ItemId;
			BlueprintItemView.Quantity = FMath::Max(1, ItemInstance.Quantity);
			BlueprintItemView.RecipeId = ItemDefinition.BlueprintRecipeId;
			BlueprintItemView.bAlreadyUnlocked = IsWorkbenchRecipeUnlocked(BlueprintItemView.RecipeId);

			FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
			BlueprintItemView.bRecipeKnown =
				!BlueprintItemView.RecipeId.IsNone() &&
				ItemDataSubsystem->TryGetWorkbenchRecipeDefinition(BlueprintItemView.RecipeId, RecipeDefinition);
			BlueprintItemView.bCanRegister =
				BlueprintItemView.bRecipeKnown &&
				!BlueprintItemView.bAlreadyUnlocked;
			OutBlueprintItemViews.Add(BlueprintItemView);
		}
	};

	AddBlueprintsFromSlots(ETunaSweeperItemSlotSource::Inventory, PlayerInventorySlots);
	AddBlueprintsFromSlots(ETunaSweeperItemSlotSource::Storage, StorageSlots);
	return OutBlueprintItemViews.Num() > 0;
}

bool UTunaSweeperGameInstance::TryRegisterWorkbenchBlueprintFromSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	if (!bHasActiveWorkbench ||
		!SlotReference.IsValid() ||
		!IsWorkbenchItemSlotSourceAllowedForBlueprintRegister(SlotReference.Source))
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem)
	{
		return false;
	}

	FTunaSweeperItemInstance ItemInstance;
	FTunaSweeperItemDefinition ItemDefinition;
	if (!TryGetSlotItemInstance(SlotReference, ItemInstance) ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition) ||
		!IsWorkbenchBlueprintItemDefinition(ItemDefinition) ||
		ItemDefinition.BlueprintRecipeId.IsNone() ||
		IsWorkbenchRecipeUnlocked(ItemDefinition.BlueprintRecipeId))
	{
		return false;
	}

	FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetWorkbenchRecipeDefinition(ItemDefinition.BlueprintRecipeId, RecipeDefinition))
	{
		return false;
	}

	if (!TryConsumeSingleItemFromSlot(SlotReference))
	{
		return false;
	}

	if (!UnlockWorkbenchRecipe(ItemDefinition.BlueprintRecipeId, false))
	{
		return false;
	}

	ClearSelectedItemIfInvalid();
	ClearHoveredItemSlot(SlotReference);
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave(bSaveImmediately);
	return true;
}

bool UTunaSweeperGameInstance::IsWorkbenchRecipeUnlocked(FName RecipeId) const
{
	if (RecipeId.IsNone())
	{
		return false;
	}

	const UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
	if (!ItemDataSubsystem || !const_cast<UTunaSweeperItemDataSubsystem*>(ItemDataSubsystem)->TryGetWorkbenchRecipeDefinition(RecipeId, RecipeDefinition))
	{
		return false;
	}

	return IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition);
}

bool UTunaSweeperGameInstance::UnlockWorkbenchRecipe(FName RecipeId, bool bSaveImmediately)
{
	if (RecipeId.IsNone())
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetWorkbenchRecipeDefinition(RecipeId, RecipeDefinition))
	{
		return false;
	}

	if (RecipeDefinition.bAutoUnlocked)
	{
		return true;
	}

	const int32 PreviousCount = UnlockedWorkbenchRecipeIds.Num();
	UnlockedWorkbenchRecipeIds.Add(RecipeId);
	const bool bChanged = UnlockedWorkbenchRecipeIds.Num() != PreviousCount;
	if (bChanged && bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return true;
}

void UTunaSweeperGameInstance::GetUnlockedWorkbenchRecipeIds(TArray<FName>& OutRecipeIds) const
{
	OutRecipeIds = UnlockedWorkbenchRecipeIds.Array();
	OutRecipeIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
}

FTunaSweeperWorldProgressSaveData UTunaSweeperGameInstance::GetOrCreateWorldProgressState(
	FName ObjectId,
	FName InfoId,
	int32 InitialProgressQuantity,
	int32 RequiredQuantity)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperWorldProgressSaveData EmptyState;
	if (ObjectId.IsNone())
	{
		return EmptyState;
	}

	FTunaSweeperWorldProgressSaveData* ExistingState = WorldProgressStatesById.Find(ObjectId);
	if (!ExistingState)
	{
		FTunaSweeperWorldProgressSaveData NewState;
		NewState.ObjectId = ObjectId;
		NewState.InfoId = InfoId;
		NewState.State = ETunaSweeperWorldProgressState::InProgress;
		NewState.ProgressQuantity = FMath::Clamp(
			InitialProgressQuantity,
			0,
			FMath::Max(0, RequiredQuantity));
		ExistingState = &WorldProgressStatesById.Add(ObjectId, NewState);
	}

	if (!InfoId.IsNone())
	{
		ExistingState->InfoId = InfoId;
	}

	ExistingState->ProgressQuantity = FMath::Clamp(
		ExistingState->ProgressQuantity,
		0,
		FMath::Max(0, RequiredQuantity));
	if (ExistingState->State == ETunaSweeperWorldProgressState::Completed)
	{
		ExistingState->ProgressQuantity = FMath::Max(ExistingState->ProgressQuantity, FMath::Max(0, RequiredQuantity));
	}

	return *ExistingState;
}

bool UTunaSweeperGameInstance::TryGetWorldProgressState(
	FName ObjectId,
	FTunaSweeperWorldProgressSaveData& OutState) const
{
	if (const FTunaSweeperWorldProgressSaveData* FoundState = WorldProgressStatesById.Find(ObjectId))
	{
		OutState = *FoundState;
		return true;
	}

	OutState = FTunaSweeperWorldProgressSaveData();
	return false;
}

bool UTunaSweeperGameInstance::UpdateWorldProgressState(
	FName ObjectId,
	FName InfoId,
	ETunaSweeperWorldProgressState State,
	int32 ProgressQuantity,
	int32 RequiredQuantity,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	if (ObjectId.IsNone())
	{
		return false;
	}

	FTunaSweeperWorldProgressSaveData NewState;
	NewState.ObjectId = ObjectId;
	NewState.InfoId = InfoId;
	NewState.State = State;
	NewState.ProgressQuantity = FMath::Clamp(
		ProgressQuantity,
		0,
		FMath::Max(0, RequiredQuantity));
	if (NewState.State == ETunaSweeperWorldProgressState::Completed)
	{
		NewState.ProgressQuantity = FMath::Max(NewState.ProgressQuantity, FMath::Max(0, RequiredQuantity));
	}

	WorldProgressStatesById.Add(ObjectId, NewState);
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return true;
}

int32 UTunaSweeperGameInstance::GetPiggyBankStoredAncientCoinValue(FName PiggyBankId) const
{
	if (PiggyBankId.IsNone())
	{
		return 0;
	}

	const FTunaSweeperPiggyBankSaveData* FoundState = PiggyBankStatesById.Find(PiggyBankId);
	return FoundState ? FMath::Max(0, FoundState->StoredAncientCoinValue) : 0;
}

bool UTunaSweeperGameInstance::AddPiggyBankStoredAncientCoinValue(
	FName PiggyBankId,
	int32 CoinValueDelta,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	if (PiggyBankId.IsNone() || CoinValueDelta <= 0)
	{
		return false;
	}

	FTunaSweeperPiggyBankSaveData& State = PiggyBankStatesById.FindOrAdd(PiggyBankId);
	State.PiggyBankId = PiggyBankId;
	State.StoredAncientCoinValue = FMath::Max(0, State.StoredAncientCoinValue) + CoinValueDelta;

	if (bSaveImmediately)
	{
		SaveGameState();
	}
	return true;
}

void UTunaSweeperGameInstance::GetHousingFacilities(
	TArray<FTunaSweeperHousingPlacedFacilitySaveData>& OutFacilities)
{
	EnsureInventoryStateInitialized();
	OutFacilities = HousingFacilities;
}

void UTunaSweeperGameInstance::SetHousingFacilities(
	const TArray<FTunaSweeperHousingPlacedFacilitySaveData>& InFacilities,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	HousingFacilities.Reset();
	TSet<FGuid> SeenInstanceIds;
	for (FTunaSweeperHousingPlacedFacilitySaveData HousingFacility : InFacilities)
	{
		if (!HousingFacility.IsValid() || SeenInstanceIds.Contains(HousingFacility.InstanceId))
		{
			continue;
		}

		HousingFacility.RotationQuarterTurns = FMath::Clamp(HousingFacility.RotationQuarterTurns, 0, 3);
		HousingFacilities.Add(HousingFacility);
		SeenInstanceIds.Add(HousingFacility.InstanceId);
	}

	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
}

bool UTunaSweeperGameInstance::IsHousingFacilityUnlocked(FName FacilityId)
{
	EnsureInventoryStateInitialized();
	return !FacilityId.IsNone() && UnlockedHousingFacilityIds.Contains(FacilityId);
}

bool UTunaSweeperGameInstance::UnlockHousingFacility(FName FacilityId, bool bSaveImmediately)
{
	if (FacilityId.IsNone())
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	const int32 PreviousCount = UnlockedHousingFacilityIds.Num();
	UnlockedHousingFacilityIds.Add(FacilityId);
	const bool bChanged = UnlockedHousingFacilityIds.Num() != PreviousCount;
	if (bChanged && bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return bChanged;
}

void UTunaSweeperGameInstance::GetUnlockedHousingFacilityIds(TArray<FName>& OutFacilityIds)
{
	EnsureInventoryStateInitialized();
	OutFacilityIds = UnlockedHousingFacilityIds.Array();
	OutFacilityIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
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

	if (!LoadGameState())
	{
		LoadedSlotTotalPlaySeconds = 0.0f;
		ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
		GenerateDefaultInventoryState();
	}

	bInventoryStateInitialized = true;
	RefreshLegacyPlayerInventoryItems();
}

bool UTunaSweeperGameInstance::LoadGameState()
{
	const FString ExistingSlotName = GetExistingSaveGameSlotName(ActiveSaveSlotIndex);
	if (ExistingSlotName.IsEmpty())
	{
		return false;
	}

	UTunaSweeperSaveGame* SaveGame = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromSlot(
		ExistingSlotName,
		TunaSweeperSave::SaveUserIndex));
	if (!SaveGame)
	{
		return false;
	}

	LoadedSlotTotalPlaySeconds = FMath::Max(0.0f, SaveGame->TotalPlaySeconds);
	ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
	ActiveSaveSlotDifficultyStage = TunaSweeperSave::SanitizeDifficultyStage(SaveGame->DifficultyStage);
	TotalExperiencePoints = FMath::Max<int64>(0, SaveGame->TotalExperiencePoints);
	RaidStartExperiencePoints = TotalExperiencePoints;
	PendingRaidExperiencePoints = 0;
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	CompletedScenarioFlags.Reset();
	for (const FName& ScenarioFlag : SaveGame->CompletedScenarioFlags)
	{
		if (!ScenarioFlag.IsNone())
		{
			CompletedScenarioFlags.Add(ScenarioFlag);
		}
	}
	AcquiredMemoIds.Reset();
	for (int32 MemoId : SaveGame->AcquiredMemoIds)
	{
		if (MemoId > 0)
		{
			AcquiredMemoIds.Add(MemoId);
		}
	}
	EverAcquiredItemIds.Reset();
	for (int32 ItemId : SaveGame->EverAcquiredItemIds)
	{
		if (ItemId != INDEX_NONE)
		{
			EverAcquiredItemIds.Add(ItemId);
		}
	}
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	TSet<int32> LoadedMapMarkerIds;
	for (const FTunaSweeperMapMarkerSaveData& SavedMapMarker : SaveGame->MapMarkers)
	{
		if (SavedMapMarker.MarkerId <= 0 || LoadedMapMarkerIds.Contains(SavedMapMarker.MarkerId))
		{
			continue;
		}

		FTunaSweeperMapMarkerSaveData LoadedMapMarker = TunaSweeperMapMarkers::SanitizeMarker(SavedMapMarker);
		MapMarkers.Add(LoadedMapMarker);
		LoadedMapMarkerIds.Add(LoadedMapMarker.MarkerId);
		NextMapMarkerId = FMath::Max(NextMapMarkerId, LoadedMapMarker.MarkerId + 1);
	}
	MapMarkers.Sort([](
		const FTunaSweeperMapMarkerSaveData& Left,
		const FTunaSweeperMapMarkerSaveData& Right)
	{
		return Left.MarkerId < Right.MarkerId;
	});
	WorldProgressStatesById.Reset();
	for (const FTunaSweeperWorldProgressSaveData& SavedWorldProgressState : SaveGame->WorldProgressStates)
	{
		if (SavedWorldProgressState.ObjectId.IsNone())
		{
			continue;
		}

		FTunaSweeperWorldProgressSaveData LoadedWorldProgressState = SavedWorldProgressState;
		LoadedWorldProgressState.ProgressQuantity = FMath::Max(0, LoadedWorldProgressState.ProgressQuantity);
		WorldProgressStatesById.Add(LoadedWorldProgressState.ObjectId, LoadedWorldProgressState);
	}
	PiggyBankStatesById.Reset();
	for (const FTunaSweeperPiggyBankSaveData& SavedPiggyBankState : SaveGame->PiggyBankStates)
	{
		if (SavedPiggyBankState.PiggyBankId.IsNone())
		{
			continue;
		}

		FTunaSweeperPiggyBankSaveData LoadedPiggyBankState = SavedPiggyBankState;
		LoadedPiggyBankState.StoredAncientCoinValue = FMath::Max(0, LoadedPiggyBankState.StoredAncientCoinValue);
		PiggyBankStatesById.Add(LoadedPiggyBankState.PiggyBankId, LoadedPiggyBankState);
	}
	HousingFacilities.Reset();
	TSet<FGuid> LoadedHousingFacilityIds;
	for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedHousingFacility : SaveGame->HousingFacilities)
	{
		if (!SavedHousingFacility.IsValid() || LoadedHousingFacilityIds.Contains(SavedHousingFacility.InstanceId))
		{
			continue;
		}

		FTunaSweeperHousingPlacedFacilitySaveData LoadedHousingFacility = SavedHousingFacility;
		LoadedHousingFacility.RotationQuarterTurns = FMath::Clamp(LoadedHousingFacility.RotationQuarterTurns, 0, 3);
		HousingFacilities.Add(LoadedHousingFacility);
		LoadedHousingFacilityIds.Add(LoadedHousingFacility.InstanceId);
	}
	UnlockedHousingFacilityIds.Reset();
	for (const FName& FacilityId : SaveGame->UnlockedHousingFacilityIds)
	{
		if (!FacilityId.IsNone())
		{
			UnlockedHousingFacilityIds.Add(FacilityId);
		}
	}
	UnlockedWorkbenchRecipeIds.Reset();
	for (const FName& RecipeId : SaveGame->UnlockedWorkbenchRecipeIds)
	{
		if (!RecipeId.IsNone())
		{
			UnlockedWorkbenchRecipeIds.Add(RecipeId);
		}
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->LoadQuestProgressFromSave(
			SaveGame->QuestProgressStates,
			SaveGame->TrackedQuestId,
			SaveGame->QuestCoinBalance);

		TArray<FTunaSweeperQuestDefinition> QuestDefinitions;
		if (QuestSubsystem->GetAllQuestDefinitions(QuestDefinitions))
		{
			for (const FTunaSweeperQuestDefinition& QuestDefinition : QuestDefinitions)
			{
				if (QuestSubsystem->GetQuestState(QuestDefinition.QuestId) != ETunaSweeperQuestState::RewardCompleted)
				{
					continue;
				}

				for (const FName& FacilityId : QuestDefinition.Rewards.HousingFacilityUnlocks)
				{
					if (!FacilityId.IsNone())
					{
						UnlockedHousingFacilityIds.Add(FacilityId);
					}
				}

				for (const FName& RecipeId : QuestDefinition.Rewards.WorkbenchRecipeUnlocks)
				{
					if (!RecipeId.IsNone())
					{
						UnlockedWorkbenchRecipeIds.Add(RecipeId);
					}
				}
			}
		}
	}
	PendingScenarioCompletionFlag = NAME_None;

	ItemInstancesByUid.Reset();
	for (const FTunaSweeperItemInstance& ItemInstance : SaveGame->ItemInstances)
	{
		FTunaSweeperItemInstance LoadedItemInstance = ItemInstance;
		TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(LoadedItemInstance);
		if (LoadedItemInstance.IsValid())
		{
			ItemInstancesByUid.Add(LoadedItemInstance.Uid, LoadedItemInstance);
		}
	}

	PlayerInventorySlots = SaveGame->InventorySlots;
	EquipmentSlots = SaveGame->EquipmentSlots;
	AuxiliaryBagSlots = SaveGame->AuxiliaryBagSlots;
	UsableQuickSlots = SaveGame->UsableQuickSlots;
	StorageSlotCapacity = NormalizeStorageSlotCapacity(SaveGame->StorageSlotCapacity);
	StorageSlots = SaveGame->StorageSlots;
	ShopStockStatesByKey.Reset();
	for (const FTunaSweeperShopStockSaveData& SavedShopStockState : SaveGame->ShopStockStates)
	{
		if (!TunaSweeperShop::IsValidShopSlotKey(
			SavedShopStockState.ShopId,
			SavedShopStockState.SlotIndex,
			SavedShopStockState.ItemId))
		{
			continue;
		}

		FTunaSweeperShopStockSaveData LoadedShopStockState = SavedShopStockState;
		LoadedShopStockState.StockQuantity = FMath::Max(0, LoadedShopStockState.StockQuantity);
		ShopStockStatesByKey.Add(
			TunaSweeperShop::MakeStockKey(
				LoadedShopStockState.ShopId,
				LoadedShopStockState.SlotIndex,
				LoadedShopStockState.ItemId),
			LoadedShopStockState);
	}
	RemoveInvalidSlotReferences(PlayerInventorySlots);
	RemoveInvalidSlotReferences(EquipmentSlots);
	RemoveInvalidSlotReferences(AuxiliaryBagSlots);
	RemoveInvalidSlotReferences(UsableQuickSlots);
	RemoveInvalidSlotReferences(StorageSlots);

	EnsureSlotArraySize(EquipmentSlots, FMath::Max(TunaSweeperInventory::RequiredEquipmentSlots, GameplaySettings.EquipmentSlotCount));
	EnsureSlotArraySize(AuxiliaryBagSlots, FMath::Max(0, GameplaySettings.AuxiliaryBagSlotCount));
	EnsureSlotArraySize(UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
	for (int32 SlotIndex = StorageSlots.Num() - 1; SlotIndex >= StorageSlotCapacity; --SlotIndex)
	{
		if (StorageSlots[SlotIndex].ItemUid.IsValid())
		{
			StorageSlotCapacity = NormalizeStorageSlotCapacity(SlotIndex + 1);
			break;
		}
	}
	EnsureSlotArraySize(StorageSlots, StorageSlotCapacity);
	for (FTunaSweeperInventorySlot& UsableQuickSlot : UsableQuickSlots)
	{
		if (UsableQuickSlot.ItemUid.IsValid() && !IsItemCompatibleWithUsableQuickSlot(UsableQuickSlot.ItemUid))
		{
			UsableQuickSlot.Clear();
		}
	}
	MigrateLegacyEquipmentSlots();
	BackfillEverAcquiredItemIdsFromCurrentItems();

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
	ActiveLootContainerOwner.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	HoveredItemSlotReference = FTunaSweeperItemSlotReference();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
	return true;
}

bool UTunaSweeperGameInstance::SaveGameStateInternal(
	UTunaSweeperGameInstance::EUsableQuickSlotSaveMode UsableQuickSlotSaveMode) const
{
	const FString ExistingSlotName = GetExistingSaveGameSlotName(ActiveSaveSlotIndex);
	UTunaSweeperSaveGame* ExistingSaveGame = nullptr;
	if (UsableQuickSlotSaveMode == EUsableQuickSlotSaveMode::PreserveExisting && !ExistingSlotName.IsEmpty())
	{
		ExistingSaveGame = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromSlot(
			ExistingSlotName,
			TunaSweeperSave::SaveUserIndex));
	}

	UTunaSweeperSaveGame* SaveGame = Cast<UTunaSweeperSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTunaSweeperSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	TSet<FGuid> PlayerOwnedItemUids;
	CollectPlayerOwnedItemUids(
		PlayerOwnedItemUids,
		UsableQuickSlotSaveMode == EUsableQuickSlotSaveMode::PersistRuntime);
	for (const FGuid& ItemUid : PlayerOwnedItemUids)
	{
		if (const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid))
		{
			SaveGame->ItemInstances.Add(TunaSweeperInventory::MakeItemInstanceForSave(*ItemInstance));
		}
	}

	SaveGame->SaveVersion = TunaSweeperSave::CurrentSaveVersion;
	SaveGame->SaveSlotIndex = ActiveSaveSlotIndex;
	SaveGame->TotalPlaySeconds = GetCurrentActiveSlotTotalPlaySeconds();
	SaveGame->DifficultyStage = TunaSweeperSave::SanitizeDifficultyStage(ActiveSaveSlotDifficultyStage);
	SaveGame->LastSavedAtTicks = FDateTime::Now().GetTicks();
	SaveGame->TotalExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
	SaveGame->CompletedScenarioFlags = CompletedScenarioFlags.Array();
	SaveGame->AcquiredMemoIds = AcquiredMemoIds.Array();
	SaveGame->AcquiredMemoIds.Sort();
	SaveGame->EverAcquiredItemIds = EverAcquiredItemIds.Array();
	SaveGame->EverAcquiredItemIds.Sort();
	SaveGame->MapMarkers = MapMarkers;
	SaveGame->MapMarkers.Sort([](
		const FTunaSweeperMapMarkerSaveData& Left,
		const FTunaSweeperMapMarkerSaveData& Right)
	{
		return Left.MarkerId < Right.MarkerId;
	});
	WorldProgressStatesById.GenerateValueArray(SaveGame->WorldProgressStates);
	SaveGame->WorldProgressStates.Sort([](
		const FTunaSweeperWorldProgressSaveData& Left,
		const FTunaSweeperWorldProgressSaveData& Right)
	{
		return Left.ObjectId.LexicalLess(Right.ObjectId);
	});
	PiggyBankStatesById.GenerateValueArray(SaveGame->PiggyBankStates);
	SaveGame->PiggyBankStates.Sort([](
		const FTunaSweeperPiggyBankSaveData& Left,
		const FTunaSweeperPiggyBankSaveData& Right)
	{
		return Left.PiggyBankId.LexicalLess(Right.PiggyBankId);
	});
	SaveGame->HousingFacilities = HousingFacilities;
	SaveGame->HousingFacilities.Sort([](
		const FTunaSweeperHousingPlacedFacilitySaveData& Left,
		const FTunaSweeperHousingPlacedFacilitySaveData& Right)
	{
		return Left.FacilityId.LexicalLess(Right.FacilityId) ||
			(Left.FacilityId == Right.FacilityId && Left.InstanceId.ToString() < Right.InstanceId.ToString());
	});
	SaveGame->UnlockedHousingFacilityIds = UnlockedHousingFacilityIds.Array();
	SaveGame->UnlockedHousingFacilityIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
	SaveGame->UnlockedWorkbenchRecipeIds = UnlockedWorkbenchRecipeIds.Array();
	SaveGame->UnlockedWorkbenchRecipeIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
	if (const UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ExportQuestProgressForSave(
			SaveGame->QuestProgressStates,
			SaveGame->TrackedQuestId,
			SaveGame->QuestCoinBalance);
	}
	SaveGame->InventorySlots = PlayerInventorySlots;
	SaveGame->EquipmentSlots = EquipmentSlots;
	SaveGame->AuxiliaryBagSlots = AuxiliaryBagSlots;
	SaveGame->StorageSlotCapacity = NormalizeStorageSlotCapacity(StorageSlotCapacity);
	SaveGame->StorageSlots = StorageSlots;
	EnsureSlotArraySize(SaveGame->StorageSlots, SaveGame->StorageSlotCapacity);
	ShopStockStatesByKey.GenerateValueArray(SaveGame->ShopStockStates);
	SaveGame->ShopStockStates.Sort([](
		const FTunaSweeperShopStockSaveData& Left,
		const FTunaSweeperShopStockSaveData& Right)
	{
		if (Left.ShopId != Right.ShopId)
		{
			return Left.ShopId < Right.ShopId;
		}
		if (Left.SlotIndex != Right.SlotIndex)
		{
			return Left.SlotIndex < Right.SlotIndex;
		}
		return Left.ItemId < Right.ItemId;
	});

	switch (UsableQuickSlotSaveMode)
	{
	case EUsableQuickSlotSaveMode::PersistRuntime:
		SaveGame->UsableQuickSlots = UsableQuickSlots;
		break;
	case EUsableQuickSlotSaveMode::PreserveExisting:
		if (ExistingSaveGame)
		{
			TMap<FGuid, FTunaSweeperItemInstance> ExistingItemInstancesByUid;
			for (const FTunaSweeperItemInstance& ExistingItemInstance : ExistingSaveGame->ItemInstances)
			{
				FTunaSweeperItemInstance NormalizedItemInstance = ExistingItemInstance;
				TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(NormalizedItemInstance);
				if (NormalizedItemInstance.IsValid())
				{
					ExistingItemInstancesByUid.Add(NormalizedItemInstance.Uid, NormalizedItemInstance);
				}
			}

			TSet<FGuid> SavedItemUids;
			for (const FTunaSweeperItemInstance& SavedItemInstance : SaveGame->ItemInstances)
			{
				if (SavedItemInstance.Uid.IsValid())
				{
					SavedItemUids.Add(SavedItemInstance.Uid);
				}
			}

			TFunction<void(const FGuid&)> AppendExistingItemUid;
			AppendExistingItemUid = [
				&AppendExistingItemUid,
				&ExistingItemInstancesByUid,
				&SavedItemUids,
				SaveGame](const FGuid& ItemUid)
			{
				if (!ItemUid.IsValid() || SavedItemUids.Contains(ItemUid))
				{
					return;
				}

				const FTunaSweeperItemInstance* ExistingItemInstance = ExistingItemInstancesByUid.Find(ItemUid);
				if (!ExistingItemInstance)
				{
					return;
				}

				SavedItemUids.Add(ItemUid);
				SaveGame->ItemInstances.Add(TunaSweeperInventory::MakeItemInstanceForSave(*ExistingItemInstance));
				for (const TPair<FName, FGuid>& AttachmentSlot : ExistingItemInstance->AttachmentSlots)
				{
					AppendExistingItemUid(AttachmentSlot.Value);
				}
			};

			SaveGame->UsableQuickSlots = ExistingSaveGame->UsableQuickSlots;
			EnsureSlotArraySize(SaveGame->UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
			for (FTunaSweeperInventorySlot& UsableQuickSlot : SaveGame->UsableQuickSlots)
			{
				if (!UsableQuickSlot.ItemUid.IsValid())
				{
					continue;
				}

				if (!ExistingItemInstancesByUid.Contains(UsableQuickSlot.ItemUid))
				{
					UsableQuickSlot.Clear();
					continue;
				}

				AppendExistingItemUid(UsableQuickSlot.ItemUid);
			}
		}
		break;
	case EUsableQuickSlotSaveMode::Clear:
	default:
		SaveGame->UsableQuickSlots.Reset();
		break;
	}
	EnsureSlotArraySize(SaveGame->UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);

	if (!ExistingSlotName.IsEmpty() && !BackupExistingSaveGame(ExistingSlotName))
	{
		return false;
	}

	return UGameplayStatics::SaveGameToSlot(
		SaveGame,
		GetSaveGameSlotName(ActiveSaveSlotIndex),
		TunaSweeperSave::SaveUserIndex);
}

void UTunaSweeperGameInstance::ResetRuntimeStateForSaveSlotSelection()
{
	GameplayInfo.Reset();
	NumberSettings.Reset();
	BoolSettings.Reset();
	PlayerHudState = FTunaSweeperPlayerHudState();
	PlayerInventoryItems.Reset();
	bHasGeneratedPlayerInventoryItems = false;
	ItemInstancesByUid.Reset();
	PlayerInventorySlots.Reset();
	EquipmentSlots.Reset();
	AuxiliaryBagSlots.Reset();
	UsableQuickSlots.Reset();
	StorageSlots.Reset();
	StorageSlotCapacity = GetDefaultStorageSlotCapacity();
	ShopStockStatesByKey.Reset();
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	HoveredItemSlotReference = FTunaSweeperItemSlotReference();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	bInventoryStateInitialized = false;
	bPendingBunkerItemStateSave = false;
	LoadedSlotTotalPlaySeconds = 0.0f;
	ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
	ActiveSaveSlotDifficultyStage = TunaSweeperSave::DefaultDifficultyStage;
	TotalExperiencePoints = 0;
	RaidStartExperiencePoints = 0;
	PendingRaidExperiencePoints = 0;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	bHasPendingBunkerEntryVitals = false;
	PendingBunkerEntryHealthRatio = 1.0f;
	PendingBunkerEntryFoodRatio = 1.0f;
	PendingBunkerEntryHydrationRatio = 1.0f;
	CompletedScenarioFlags.Reset();
	AcquiredMemoIds.Reset();
	EverAcquiredItemIds.Reset();
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	WorldProgressStatesById.Reset();
	PiggyBankStatesById.Reset();
	HousingFacilities.Reset();
	UnlockedHousingFacilityIds.Reset();
	UnlockedWorkbenchRecipeIds.Reset();
	PendingScenarioCompletionFlag = NAME_None;
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ResetQuestProgressForNewGame();
	}
}

void UTunaSweeperGameInstance::GenerateDefaultInventoryState()
{
	ItemInstancesByUid.Reset();
	ActiveSaveSlotDifficultyStage = TunaSweeperSave::DefaultDifficultyStage;
	TotalExperiencePoints = 0;
	RaidStartExperiencePoints = 0;
	PendingRaidExperiencePoints = 0;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	bHasPendingBunkerEntryVitals = false;
	bPendingBunkerItemStateSave = false;
	PendingBunkerEntryHealthRatio = 1.0f;
	PendingBunkerEntryFoodRatio = 1.0f;
	PendingBunkerEntryHydrationRatio = 1.0f;
	CompletedScenarioFlags.Reset();
	AcquiredMemoIds.Reset();
	EverAcquiredItemIds.Reset();
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	WorldProgressStatesById.Reset();
	PiggyBankStatesById.Reset();
	HousingFacilities.Reset();
	UnlockedHousingFacilityIds.Reset();
	UnlockedWorkbenchRecipeIds.Reset();
	PendingScenarioCompletionFlag = NAME_None;
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ResetQuestProgressForNewGame();
	}
	ResetPlayerSlotArrays();
	StorageSlotCapacity = GetDefaultStorageSlotCapacity();
	StorageSlots.Reset();
	EnsureSlotArraySize(StorageSlots, StorageSlotCapacity);
	ShopStockStatesByKey.Reset();
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	HoveredItemSlotReference = FTunaSweeperItemSlotReference();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
}

void UTunaSweeperGameInstance::ResetPlayerSlotArrays()
{
	EquipmentSlots.Reset();
	AuxiliaryBagSlots.Reset();
	UsableQuickSlots.Reset();
	PlayerInventorySlots.Reset();
	EnsureSlotArraySize(EquipmentSlots, FMath::Max(TunaSweeperInventory::RequiredEquipmentSlots, GameplaySettings.EquipmentSlotCount));
	EnsureSlotArraySize(AuxiliaryBagSlots, FMath::Max(0, GameplaySettings.AuxiliaryBagSlotCount));
	EnsureSlotArraySize(UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
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

int32 UTunaSweeperGameInstance::ResolveItemExperienceValue(int32 ItemId)
{
	if (ItemId == INDEX_NONE)
	{
		return 0;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition))
	{
		return 0;
	}

	return FMath::Max(0, ItemDefinition.ExperienceValue);
}

FTunaSweeperExperienceAnimationState UTunaSweeperGameInstance::BuildExperienceAnimationState(
	int64 StartExperiencePoints,
	int64 TargetExperiencePoints,
	int64 GainedExperiencePoints) const
{
	FTunaSweeperExperienceAnimationState AnimationState;
	AnimationState.StartExperiencePoints = FMath::Max<int64>(0, StartExperiencePoints);
	AnimationState.TargetExperiencePoints = FMath::Max<int64>(
		AnimationState.StartExperiencePoints,
		TargetExperiencePoints);
	AnimationState.GainedExperiencePoints = FMath::Max<int64>(
		0,
		FMath::Max<int64>(GainedExperiencePoints, AnimationState.TargetExperiencePoints - AnimationState.StartExperiencePoints));
	AnimationState.StartLevel = GetExperienceLevelForTotal(AnimationState.StartExperiencePoints);
	AnimationState.TargetLevel = GetExperienceLevelForTotal(AnimationState.TargetExperiencePoints);
	AnimationState.AnimationDurationSeconds = TunaSweeperExperience::RaidReturnAnimationDurationSeconds;
	return AnimationState;
}

void UTunaSweeperGameInstance::EnsureExperienceLevelTableLoaded() const
{
	if (bExperienceLevelTableLoaded)
	{
		return;
	}

	bExperienceLevelTableLoaded = true;
	if (!LoadExperienceLevelTableJson(CachedExperienceForLevels))
	{
		BuildDefaultExperienceLevelTable(CachedExperienceForLevels);
	}
}

bool UTunaSweeperGameInstance::LoadExperienceLevelTableJson(TArray<int64>& OutExperienceForLevels) const
{
	OutExperienceForLevels.Reset();

	const FString JsonPath = GetExperienceLevelTableJsonPath();
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *JsonPath))
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Could not load experience level table JSON: %s"), *JsonPath);
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Could not parse experience level table JSON: %s"), *JsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> RootArrayValues;
	const TArray<TSharedPtr<FJsonValue>>* LevelValues = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		RootArrayValues = RootValue->AsArray();
		LevelValues = &RootArrayValues;
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		if (RootObject.IsValid())
		{
			if (!RootObject->TryGetArrayField(TEXT("levels"), LevelValues))
			{
				RootObject->TryGetArrayField(TEXT("level_table"), LevelValues);
			}
		}
	}

	if (!LevelValues)
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Experience level table JSON has no level array: %s"), *JsonPath);
		return false;
	}

	TMap<int32, int64> ExperienceByLevel;
	for (const TSharedPtr<FJsonValue>& LevelValue : *LevelValues)
	{
		const TSharedPtr<FJsonObject> LevelObject = LevelValue.IsValid() ? LevelValue->AsObject() : nullptr;
		int32 ParsedLevel = 1;
		int64 ParsedExperience = 0;
		if (TunaSweeperExperience::ParseLevelTableRow(LevelObject, ParsedLevel, ParsedExperience))
		{
			ExperienceByLevel.Add(ParsedLevel, ParsedExperience);
		}
	}

	if (!ExperienceByLevel.Contains(1))
	{
		ExperienceByLevel.Add(1, 0);
	}

	int32 MaxLevel = 1;
	for (const TPair<int32, int64>& LevelPair : ExperienceByLevel)
	{
		MaxLevel = FMath::Max(MaxLevel, LevelPair.Key);
	}

	if (MaxLevel <= 1)
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Experience level table JSON has no valid progression rows: %s"), *JsonPath);
		return false;
	}

	int64 PreviousExperience = 0;
	for (int32 Level = 1; Level <= MaxLevel; ++Level)
	{
		const int64* FoundExperience = ExperienceByLevel.Find(Level);
		if (!FoundExperience)
		{
			UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Experience level table JSON is missing level %d: %s"), Level, *JsonPath);
			OutExperienceForLevels.Reset();
			return false;
		}

		const int64 NormalizedExperience = Level <= 1
			? 0
			: FMath::Max(*FoundExperience, PreviousExperience + 1);
		OutExperienceForLevels.Add(NormalizedExperience);
		PreviousExperience = NormalizedExperience;
	}

	return OutExperienceForLevels.Num() > 1;
}

void UTunaSweeperGameInstance::BuildDefaultExperienceLevelTable(TArray<int64>& OutExperienceForLevels) const
{
	OutExperienceForLevels.Reset();
	OutExperienceForLevels.Add(0);

	int64 TotalExperience = 0;
	for (int32 Level = 2; Level <= TunaSweeperExperience::DefaultMaxExperienceLevel; ++Level)
	{
		const int64 PreviousLevel = static_cast<int64>(Level - 1);
		TotalExperience += TunaSweeperExperience::BaseExperienceForNextLevel +
			((PreviousLevel - 1) * TunaSweeperExperience::ExperienceIncreasePerLevel);
		OutExperienceForLevels.Add(TotalExperience);
	}
}

FString UTunaSweeperGameInstance::GetExperienceLevelTableJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperExperience::LevelTableJsonRelativePath);
}

void UTunaSweeperGameInstance::EnsureExperienceLevelRewardsLoaded() const
{
	if (bExperienceLevelRewardsLoaded)
	{
		return;
	}

	bExperienceLevelRewardsLoaded = true;

	TArray<FTunaSweeperExperienceLevelReward> LoadedRewards;
	LoadExperienceLevelRewardsJson(LoadedRewards);

	TMap<int32, FTunaSweeperExperienceLevelReward> RewardsByLevel;
	for (FTunaSweeperExperienceLevelReward Reward : LoadedRewards)
	{
		Reward.Normalize();
		RewardsByLevel.Add(Reward.Level, Reward);
	}

	CachedExperienceLevelRewards.Reset();
	RewardsByLevel.GenerateValueArray(CachedExperienceLevelRewards);
	CachedExperienceLevelRewards.Sort([](
		const FTunaSweeperExperienceLevelReward& Left,
		const FTunaSweeperExperienceLevelReward& Right)
	{
		return Left.Level < Right.Level;
	});
}

bool UTunaSweeperGameInstance::LoadExperienceLevelRewardsJson(
	TArray<FTunaSweeperExperienceLevelReward>& OutRewards) const
{
	OutRewards.Reset();

	const FString JsonPath = GetExperienceLevelRewardsJsonPath();
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *JsonPath))
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Could not load experience level rewards JSON: %s"), *JsonPath);
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Could not parse experience level rewards JSON: %s"), *JsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> RootArrayValues;
	const TArray<TSharedPtr<FJsonValue>>* RewardValues = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		RootArrayValues = RootValue->AsArray();
		RewardValues = &RootArrayValues;
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		if (RootObject.IsValid())
		{
			if (!RootObject->TryGetArrayField(TEXT("level_rewards"), RewardValues))
			{
				RootObject->TryGetArrayField(TEXT("rewards"), RewardValues);
			}
		}
	}

	if (!RewardValues)
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Experience level rewards JSON has no reward array: %s"), *JsonPath);
		return false;
	}

	for (const TSharedPtr<FJsonValue>& RewardValue : *RewardValues)
	{
		const TSharedPtr<FJsonObject> RewardObject = RewardValue.IsValid() ? RewardValue->AsObject() : nullptr;
		FTunaSweeperExperienceLevelReward Reward;
		if (TunaSweeperExperience::ParseLevelReward(RewardObject, Reward))
		{
			OutRewards.Add(Reward);
		}
	}

	if (OutRewards.Num() <= 0)
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Experience level rewards JSON has no valid rows: %s"), *JsonPath);
		return false;
	}

	return true;
}

FString UTunaSweeperGameInstance::GetExperienceLevelRewardsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperExperience::LevelRewardsJsonRelativePath);
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

int32 UTunaSweeperGameInstance::GetShopStockQuantity(
	int32 ShopId,
	int32 SlotIndex,
	const FTunaSweeperShopItemDefinition& ShopItemDefinition) const
{
	if (!TunaSweeperShop::IsValidShopSlotKey(ShopId, SlotIndex, ShopItemDefinition.ItemId))
	{
		return 0;
	}

	const FName StockKey = TunaSweeperShop::MakeStockKey(ShopId, SlotIndex, ShopItemDefinition.ItemId);
	if (const FTunaSweeperShopStockSaveData* SavedStockState = ShopStockStatesByKey.Find(StockKey))
	{
		return FMath::Clamp(SavedStockState->StockQuantity, 0, FMath::Max(0, ShopItemDefinition.StockQuantity));
	}

	return FMath::Max(0, ShopItemDefinition.StockQuantity);
}

void UTunaSweeperGameInstance::SetShopStockQuantity(
	int32 ShopId,
	int32 SlotIndex,
	const FTunaSweeperShopItemDefinition& ShopItemDefinition,
	int32 StockQuantity)
{
	if (!TunaSweeperShop::IsValidShopSlotKey(ShopId, SlotIndex, ShopItemDefinition.ItemId))
	{
		return;
	}

	const FName StockKey = TunaSweeperShop::MakeStockKey(ShopId, SlotIndex, ShopItemDefinition.ItemId);
	FTunaSweeperShopStockSaveData StockState;
	StockState.ShopId = ShopId;
	StockState.SlotIndex = SlotIndex;
	StockState.ItemId = ShopItemDefinition.ItemId;
	StockState.StockQuantity = FMath::Clamp(StockQuantity, 0, FMath::Max(0, ShopItemDefinition.StockQuantity));
	ShopStockStatesByKey.Add(StockKey, StockState);
}

FTunaSweeperWorkbenchRecipeView UTunaSweeperGameInstance::BuildWorkbenchRecipeView(
	const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition,
	int32 RecipeSlotIndex) const
{
	FTunaSweeperWorkbenchRecipeView RecipeView;
	RecipeView.RecipeId = RecipeDefinition.RecipeId;
	RecipeView.WorkbenchId = RecipeDefinition.WorkbenchId;
	RecipeView.SlotIndex = RecipeSlotIndex;
	RecipeView.OutputItemId = RecipeDefinition.OutputItemId;
	RecipeView.OutputQuantity = FMath::Max(1, RecipeDefinition.OutputQuantity);

	for (const FTunaSweeperWorkbenchIngredient& Ingredient : RecipeDefinition.Ingredients)
	{
		FTunaSweeperWorkbenchIngredientView IngredientView;
		IngredientView.ItemId = Ingredient.ItemId;
		IngredientView.RequiredQuantity = FMath::Max(1, Ingredient.Quantity);
		IngredientView.AvailableQuantity = CountWorkbenchIngredientItemById(Ingredient.ItemId);
		IngredientView.MissingQuantity = FMath::Max(0, IngredientView.RequiredQuantity - IngredientView.AvailableQuantity);
		if (IngredientView.MissingQuantity > 0)
		{
			++RecipeView.MissingIngredientCount;
		}
		RecipeView.Ingredients.Add(IngredientView);
	}

	RecipeView.bCanCraft =
		RecipeView.OutputItemId != INDEX_NONE &&
		RecipeView.OutputQuantity > 0 &&
		RecipeView.Ingredients.Num() > 0 &&
		RecipeView.MissingIngredientCount <= 0;
	return RecipeView;
}

bool UTunaSweeperGameInstance::IsWorkbenchRecipeDefinitionUnlocked(
	const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition) const
{
	return RecipeDefinition.bAutoUnlocked ||
		(!RecipeDefinition.RecipeId.IsNone() && UnlockedWorkbenchRecipeIds.Contains(RecipeDefinition.RecipeId));
}

bool UTunaSweeperGameInstance::IsWorkbenchBlueprintItemDefinition(
	const FTunaSweeperItemDefinition& ItemDefinition) const
{
	static const FName BlueprintCategoryTag(TEXT("item.category.blueprint"));
	return ItemDefinition.CategoryTag == BlueprintCategoryTag || !ItemDefinition.BlueprintRecipeId.IsNone();
}

bool UTunaSweeperGameInstance::IsWorkbenchItemSlotSourceAllowedForDismantle(ETunaSweeperItemSlotSource Source) const
{
	return Source == ETunaSweeperItemSlotSource::Inventory ||
		Source == ETunaSweeperItemSlotSource::Storage;
}

bool UTunaSweeperGameInstance::IsWorkbenchItemSlotSourceAllowedForBlueprintRegister(ETunaSweeperItemSlotSource Source) const
{
	return Source == ETunaSweeperItemSlotSource::Inventory ||
		Source == ETunaSweeperItemSlotSource::Storage;
}

bool UTunaSweeperGameInstance::TryConsumeSingleItemFromSlot(const FTunaSweeperItemSlotReference& SlotReference)
{
	if (!SlotReference.IsValid())
	{
		return false;
	}

	TArray<FTunaSweeperInventorySlot>* Slots = GetMutableSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		return false;
	}

	FTunaSweeperInventorySlot& Slot = (*Slots)[SlotReference.SlotIndex];
	FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
	if (!ItemInstance || !ItemInstance->IsValid())
	{
		return false;
	}

	if (ItemInstance->Quantity > 1)
	{
		--ItemInstance->Quantity;
		return true;
	}

	TFunction<void(const FGuid&)> RemoveItemUid = [this, &RemoveItemUid](const FGuid& Uid)
	{
		if (!Uid.IsValid())
		{
			return;
		}

		TArray<FGuid> AttachmentUids;
		if (const FTunaSweeperItemInstance* RemovedItemInstance = ItemInstancesByUid.Find(Uid))
		{
			for (const TPair<FName, FGuid>& AttachmentSlot : RemovedItemInstance->AttachmentSlots)
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

	RemoveItemUid(Slot.ItemUid);
	Slot.Clear();
	return true;
}

bool UTunaSweeperGameInstance::AddWorkbenchResultToInventoryOrOverflow(
	int32 ItemId,
	int32 Quantity,
	TArray<FTunaSweeperItemStack>& InOutOverflowItems)
{
	if (ItemId == INDEX_NONE || Quantity <= 0)
	{
		return false;
	}

	int32 RemainingQuantity = Quantity;
	TryAddItemQuantityToExistingStacks(ItemId, RemainingQuantity, PlayerInventorySlots);
	TryAddItemQuantityToFirstEmptySlots(ItemId, RemainingQuantity, PlayerInventorySlots);
	if (RemainingQuantity > 0)
	{
		FTunaSweeperItemStack* ExistingOverflow = InOutOverflowItems.FindByPredicate(
			[ItemId](const FTunaSweeperItemStack& Candidate)
			{
				return Candidate.ItemId == ItemId;
			});
		if (ExistingOverflow)
		{
			ExistingOverflow->Quantity += RemainingQuantity;
		}
		else
		{
			FTunaSweeperItemStack OverflowStack;
			OverflowStack.ItemId = ItemId;
			OverflowStack.Quantity = RemainingQuantity;
			InOutOverflowItems.Add(OverflowStack);
		}
	}

	const int32 AddedQuantity = Quantity - RemainingQuantity;
	if (AddedQuantity > 0)
	{
		MarkItemEverAcquired(ItemId);
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyItemAcquired(ItemId, AddedQuantity, !IsCurrentWorldBunkerMap());
		}
		AddRaidExperienceForItem(ItemId, AddedQuantity);
	}
	return true;
}

int32 UTunaSweeperGameInstance::CountWorkbenchIngredientItemById(int32 ItemId) const
{
	if (ItemId == INDEX_NONE)
	{
		return 0;
	}

	int32 ItemCount = 0;
	auto CountInSlots = [this, ItemId, &ItemCount](const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (const FTunaSweeperInventorySlot& Slot : Slots)
		{
			const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
			if (ItemInstance && ItemInstance->ItemId == ItemId)
			{
				ItemCount += FMath::Max(0, ItemInstance->Quantity);
			}
		}
	};

	CountInSlots(PlayerInventorySlots);
	CountInSlots(AuxiliaryBagSlots);
	CountInSlots(StorageSlots);
	return ItemCount;
}

int32 UTunaSweeperGameInstance::ConsumeWorkbenchIngredientItemById(int32 ItemId, int32 RequestedAmount)
{
	if (ItemId == INDEX_NONE || RequestedAmount <= 0)
	{
		return 0;
	}

	int32 RemainingAmount = RequestedAmount;
	auto ConsumeInSlots = [this, ItemId, &RemainingAmount](TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (FTunaSweeperInventorySlot& Slot : Slots)
		{
			if (RemainingAmount <= 0)
			{
				break;
			}

			FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
			if (!ItemInstance || ItemInstance->ItemId != ItemId)
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

	ConsumeInSlots(PlayerInventorySlots);
	ConsumeInSlots(AuxiliaryBagSlots);
	ConsumeInSlots(StorageSlots);
	return RequestedAmount - RemainingAmount;
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

bool UTunaSweeperGameInstance::BackupExistingSaveGame(const FString& ExistingSlotName) const
{
	if (ExistingSlotName.IsEmpty())
	{
		return true;
	}

	const FString SourceFilePath = GetSaveGameFilePath(ExistingSlotName);
	if (!FPaths::FileExists(SourceFilePath))
	{
		return false;
	}

	const FString BackupDirectory = GetSaveGameBackupDirectory();
	if (!IFileManager::Get().MakeDirectory(*BackupDirectory, true))
	{
		return false;
	}

	const int32 BackupSlotIndex = SanitizeSaveSlotIndex(ActiveSaveSlotIndex);
	const FString BackupFilePath = CreateSaveGameBackupFilePath(BackupSlotIndex, FDateTime::Now());
	bool bBackupWritten = false;

	if (UTunaSweeperSaveGame* ExistingSaveGame = Cast<UTunaSweeperSaveGame>(
		UGameplayStatics::LoadGameFromSlot(ExistingSlotName, TunaSweeperSave::SaveUserIndex)))
	{
		ExistingSaveGame->SaveSlotIndex = BackupSlotIndex;

		TArray<uint8> BackupData;
		bBackupWritten =
			UGameplayStatics::SaveGameToMemory(ExistingSaveGame, BackupData) &&
			FFileHelper::SaveArrayToFile(BackupData, *BackupFilePath);
	}

	if (!bBackupWritten)
	{
		TArray<uint8> RawSaveData;
		bBackupWritten =
			FFileHelper::LoadFileToArray(RawSaveData, *SourceFilePath) &&
			FFileHelper::SaveArrayToFile(RawSaveData, *BackupFilePath);
	}

	if (!bBackupWritten)
	{
		return false;
	}

	TrimSaveGameBackups();
	return true;
}

void UTunaSweeperGameInstance::TrimSaveGameBackups() const
{
	const FString BackupDirectory = GetSaveGameBackupDirectory();
	TArray<FString> BackupFiles;
	IFileManager::Get().FindFilesRecursive(
		BackupFiles,
		*BackupDirectory,
		TEXT("*.sav"),
		true,
		false);

	if (BackupFiles.Num() <= TunaSweeperSave::MaxSaveGameBackupCount)
	{
		return;
	}

	BackupFiles.Sort([](const FString& Left, const FString& Right)
	{
		const FDateTime LeftTime = IFileManager::Get().GetTimeStamp(*Left);
		const FDateTime RightTime = IFileManager::Get().GetTimeStamp(*Right);
		return LeftTime == RightTime ? Left < Right : LeftTime < RightTime;
	});

	const int32 DeleteCount = BackupFiles.Num() - TunaSweeperSave::MaxSaveGameBackupCount;
	for (int32 BackupIndex = 0; BackupIndex < DeleteCount; ++BackupIndex)
	{
		IFileManager::Get().Delete(*BackupFiles[BackupIndex], false, true);
	}
}

bool UTunaSweeperGameInstance::LoadActiveSaveSlotSelection(int32& OutSaveSlotIndex) const
{
	if (!UGameplayStatics::DoesSaveGameExist(GetSaveSettingsSlotName(), TunaSweeperSave::SaveUserIndex))
	{
		OutSaveSlotIndex = 1;
		return false;
	}

	const UTunaSweeperSaveSettings* SaveSettings = Cast<UTunaSweeperSaveSettings>(
		UGameplayStatics::LoadGameFromSlot(GetSaveSettingsSlotName(), TunaSweeperSave::SaveUserIndex));
	if (!SaveSettings)
	{
		OutSaveSlotIndex = 1;
		return false;
	}

	OutSaveSlotIndex = SanitizeSaveSlotIndex(SaveSettings->LastSelectedSaveSlotIndex);
	return true;
}

bool UTunaSweeperGameInstance::SaveActiveSaveSlotSelection() const
{
	UTunaSweeperSaveSettings* SaveSettings = Cast<UTunaSweeperSaveSettings>(
		UGameplayStatics::CreateSaveGameObject(UTunaSweeperSaveSettings::StaticClass()));
	if (!SaveSettings)
	{
		return false;
	}

	SaveSettings->LastSelectedSaveSlotIndex = SanitizeSaveSlotIndex(ActiveSaveSlotIndex);
	return UGameplayStatics::SaveGameToSlot(
		SaveSettings,
		GetSaveSettingsSlotName(),
		TunaSweeperSave::SaveUserIndex);
}

void UTunaSweeperGameInstance::InitializeGlobalLanguageSetting()
{
	ETunaSweeperItemTextLanguage LoadedLanguage = ETunaSweeperItemTextLanguage::English;
	if (LoadGlobalLanguageSetting(LoadedLanguage))
	{
		CurrentTextLanguage = LoadedLanguage;
		ApplyCurrentLanguageCulture();
		return;
	}

	CurrentTextLanguage = DetectDefaultLanguageFromOS();
	ApplyCurrentLanguageCulture();
	SaveGlobalLanguageSetting();
}

bool UTunaSweeperGameInstance::LoadGlobalLanguageSetting(ETunaSweeperItemTextLanguage& OutLanguage) const
{
	if (!GConfig)
	{
		return false;
	}

	FString SavedLanguageCode;
	if (!GConfig->GetString(
		TunaSweeperLanguage::SectionName,
		TunaSweeperLanguage::LanguageKey,
		SavedLanguageCode,
		GGameUserSettingsIni))
	{
		return false;
	}

	return TunaSweeperLanguage::TryParseLanguageCode(SavedLanguageCode, OutLanguage);
}

void UTunaSweeperGameInstance::SaveGlobalLanguageSetting() const
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetString(
		TunaSweeperLanguage::SectionName,
		TunaSweeperLanguage::LanguageKey,
		TunaSweeperLanguage::ToLanguageCode(CurrentTextLanguage),
		GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

ETunaSweeperItemTextLanguage UTunaSweeperGameInstance::DetectDefaultLanguageFromOS() const
{
	ETunaSweeperItemTextLanguage DetectedLanguage = ETunaSweeperItemTextLanguage::English;
	if (TunaSweeperLanguage::TryParseLanguageCode(FPlatformMisc::GetDefaultLanguage(), DetectedLanguage))
	{
		return DetectedLanguage;
	}

	if (TunaSweeperLanguage::TryParseLanguageCode(FPlatformMisc::GetDefaultLocale(), DetectedLanguage))
	{
		return DetectedLanguage;
	}

	return ETunaSweeperItemTextLanguage::English;
}

void UTunaSweeperGameInstance::ApplyCurrentLanguageCulture() const
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		return;
	}
#endif

	FInternationalization::Get().SetCurrentCulture(TunaSweeperLanguage::ToLanguageCode(CurrentTextLanguage));
}

int32 UTunaSweeperGameInstance::FindFirstExistingSaveSlotIndex() const
{
	for (int32 SaveSlotIndex = TunaSweeperSave::MinSaveSlotIndex;
		SaveSlotIndex <= TunaSweeperSave::MaxSaveSlotIndex;
		++SaveSlotIndex)
	{
		if (!GetExistingSaveGameSlotName(SaveSlotIndex).IsEmpty())
		{
			return SaveSlotIndex;
		}
	}

	return TunaSweeperSave::MinSaveSlotIndex;
}

int32 UTunaSweeperGameInstance::SanitizeSaveSlotIndex(int32 SaveSlotIndex) const
{
	return FMath::Clamp(
		SaveSlotIndex,
		TunaSweeperSave::MinSaveSlotIndex,
		TunaSweeperSave::MaxSaveSlotIndex);
}

FString UTunaSweeperGameInstance::GetSaveGameSlotName(int32 SaveSlotIndex) const
{
	return FString::Printf(
		TEXT("%s%02d"),
		TunaSweeperSave::SaveSlotNamePrefix,
		SanitizeSaveSlotIndex(SaveSlotIndex));
}

FString UTunaSweeperGameInstance::GetSaveSettingsSlotName() const
{
	return FString(TunaSweeperSave::SaveSettingsSlotName);
}

FString UTunaSweeperGameInstance::GetExistingSaveGameSlotName(int32 SaveSlotIndex) const
{
	const int32 SanitizedSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	const FString SlotName = GetSaveGameSlotName(SanitizedSlotIndex);
	if (UGameplayStatics::DoesSaveGameExist(SlotName, TunaSweeperSave::SaveUserIndex))
	{
		return SlotName;
	}

	return FString();
}

FString UTunaSweeperGameInstance::GetSaveGameFilePath(const FString& SaveSlotName) const
{
	return FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SaveGames"),
		SaveSlotName + TEXT(".sav"));
}

FString UTunaSweeperGameInstance::GetSaveGameBackupDirectory() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Backups"));
}

FString UTunaSweeperGameInstance::CreateSaveGameBackupFilePath(int32 SaveSlotIndex, FDateTime BackupTime) const
{
	const FString BackupFileName = FString::Printf(
		TEXT("SaveSlot%02d_%s_%lld.sav"),
		SanitizeSaveSlotIndex(SaveSlotIndex),
		*BackupTime.ToString(TEXT("%Y%m%d_%H%M%S")),
		BackupTime.GetTicks());

	return FPaths::Combine(GetSaveGameBackupDirectory(), BackupFileName);
}

float UTunaSweeperGameInstance::GetCurrentActiveSlotTotalPlaySeconds() const
{
	const double SessionSeconds = ActiveSlotStartTimeSeconds > 0.0
		? FPlatformTime::Seconds() - ActiveSlotStartTimeSeconds
		: 0.0;
	return LoadedSlotTotalPlaySeconds + static_cast<float>(FMath::Max(0.0, SessionSeconds));
}

bool UTunaSweeperGameInstance::IsCurrentWorldBunkerMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("BunkerMap"));
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
