#include "Game/TunaSweeperGameInstance.h"

#include "Component/TunaSweeperVitalsComponent.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

namespace TunaSweeperSave
{
	const TCHAR* SaveSlotNamePrefix = TEXT("TunaSweeperSave_Slot");
	const TCHAR* SaveSettingsSlotName = TEXT("TunaSweeperSaveSettings");
	constexpr int32 CurrentSaveVersion = 11;
	constexpr int32 SaveUserIndex = 0;
	constexpr int32 MinSaveSlotIndex = 1;
	constexpr int32 MaxSaveSlotIndex = 3;
	constexpr int32 MaxSaveGameBackupCount = 30;
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
	constexpr int64 BaseExperienceForNextLevel = 100;
	constexpr int64 ExperienceIncreasePerLevel = 50;
	constexpr float RaidReturnAnimationDurationSeconds = 3.2f;
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
	const FName PistolAmmoTypeTag(TEXT("ammo.type.pistol"));
	const FName RifleAmmoTypeTag(TEXT("ammo.type.rifle"));
	const FName ShotgunAmmoTypeTag(TEXT("ammo.type.shotgun"));
	const FName MagazineAttachmentSlotTag(TEXT("attachment.slot.magazine"));
	const FName OpticAttachmentSlotTag(TEXT("attachment.slot.optic"));
	constexpr int32 DefaultWeaponMagazineCapacity = 12;
	constexpr float DefaultWeaponReloadSeconds = 1.8f;

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
			OutDefinition.DecreasePerSecond = 5.0f;
			return true;
		}

		if (WeaponTypeTag == TunaSweeperInventory::RifleWeaponTypeTag)
		{
			OutDefinition.IncreasePerShot = 0.8f;
			OutDefinition.MinimumSpreadHalfAngleDegrees = 1.0f;
			OutDefinition.MaximumSpreadHalfAngleDegrees = 6.0f;
			OutDefinition.DecreasePerSecond = 6.5f;
			return true;
		}

		if (WeaponTypeTag == TunaSweeperInventory::ShotgunWeaponTypeTag)
		{
			OutDefinition.IncreasePerShot = 2.0f;
			OutDefinition.MinimumSpreadHalfAngleDegrees = 4.5f;
			OutDefinition.MaximumSpreadHalfAngleDegrees = 12.0f;
			OutDefinition.DecreasePerSecond = 4.5f;
			return true;
		}

		OutDefinition.IncreasePerShot = 1.0f;
		OutDefinition.MinimumSpreadHalfAngleDegrees = 1.0f;
		OutDefinition.MaximumSpreadHalfAngleDegrees = 8.0f;
		OutDefinition.DecreasePerSecond = 5.0f;
		return true;
	}
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
		GenerateDefaultInventoryState();
		bInventoryStateInitialized = true;
		RefreshLegacyPlayerInventoryItems();
		SaveGameStateInternal(EUsableQuickSlotSaveMode::Clear);
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

int32 UTunaSweeperGameInstance::GetExperienceLevelForTotal(int64 ExperiencePoints) const
{
	const int64 ClampedExperience = FMath::Max<int64>(0, ExperiencePoints);
	int32 Level = 1;
	while (ClampedExperience >= GetExperienceForLevel(Level + 1))
	{
		++Level;
	}

	return Level;
}

int64 UTunaSweeperGameInstance::GetExperienceForLevel(int32 Level) const
{
	if (Level <= 1)
	{
		return 0;
	}

	const int64 CompletedLevels = static_cast<int64>(Level - 1);
	return CompletedLevels *
		((2 * TunaSweeperExperience::BaseExperienceForNextLevel) +
			((CompletedLevels - 1) * TunaSweeperExperience::ExperienceIncreasePerLevel)) /
		2;
}

int64 UTunaSweeperGameInstance::GetExperienceForNextLevel(int32 Level) const
{
	const int64 SafeLevel = FMath::Max<int64>(1, Level);
	return TunaSweeperExperience::BaseExperienceForNextLevel +
		((SafeLevel - 1) * TunaSweeperExperience::ExperienceIncreasePerLevel);
}

float UTunaSweeperGameInstance::GetExperienceProgressForTotal(int64 ExperiencePoints) const
{
	const int64 ClampedExperience = FMath::Max<int64>(0, ExperiencePoints);
	const int32 Level = GetExperienceLevelForTotal(ClampedExperience);
	const int64 LevelStartExperience = GetExperienceForLevel(Level);
	const int64 NextLevelExperience = GetExperienceForLevel(Level + 1);
	const int64 LevelSpan = FMath::Max<int64>(1, NextLevelExperience - LevelStartExperience);
	return static_cast<float>(ClampedExperience - LevelStartExperience) / static_cast<float>(LevelSpan);
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
	PlayerHudState.NormalizeWeightLimits();
}

float UTunaSweeperGameInstance::GetCarryWeightMovementSpeedMultiplier() const
{
	FTunaSweeperPlayerHudState NormalizedHudState = PlayerHudState;
	NormalizedHudState.NormalizeWeightLimits();
	return NormalizedHudState.GetCarryWeightMovementSpeedMultiplier();
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

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetUsableQuickSlots()
{
	EnsureInventoryStateInitialized();
	return UsableQuickSlots;
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

	UTunaSweeperVitalsComponent* VitalsComponent = InstigatorPawn->FindComponentByClass<UTunaSweeperVitalsComponent>();
	if (!VitalsComponent)
	{
		return false;
	}

	FTunaSweeperVitalsDelta Effect;
	Effect.Health = ItemDefinition.UseHealthDelta;
	Effect.Food = ItemDefinition.UseFoodDelta;
	Effect.Hydration = ItemDefinition.UseHydrationDelta;
	VitalsComponent->ApplyConsumableVitalsEffect(Effect);

	ItemInstance->Quantity -= 1;
	if (ItemInstance->Quantity <= 0)
	{
		ItemInstancesByUid.Remove(ItemUid);
		(*Slots)[SlotReference.SlotIndex].Clear();
		RemoveInvalidSlotReferences(PlayerInventorySlots);
		RemoveInvalidSlotReferences(EquipmentSlots);
		RemoveInvalidSlotReferences(AuxiliaryBagSlots);
		RemoveInvalidSlotReferences(UsableQuickSlots);
		RemoveInvalidSlotReferences(ActiveLootContainerSlots);
	}

	ClearSelectedItemIfInvalid();
	BroadcastInventoryStateChanged();
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
	TArray<FTunaSweeperInventorySlot> SimUsableQuickSlots = UsableQuickSlots;
	TArray<FTunaSweeperInventorySlot> SimLootContainerSlots = ActiveLootContainerSlots;
	TArray<FTunaSweeperInventorySlot> SimSelectedWeaponAttachmentSlots = SelectedWeaponAttachmentSlots;

	auto GetSimSlots = [
		&SimInventorySlots,
		&SimEquipmentSlots,
		&SimAuxiliaryBagSlots,
		&SimUsableQuickSlots,
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
	if (bAcquiredFromLootContainer && AcquiredItemId != INDEX_NONE && AcquiredQuantity > 0)
	{
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyItemAcquired(AcquiredItemId, AcquiredQuantity);
		}
		AddRaidExperienceForItem(AcquiredItemId, AcquiredQuantity);
	}
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
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyItemAcquired(SplitItemId, SplitQuantity);
		}
		AddRaidExperienceForItem(SplitItemId, SplitQuantity);
	}
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
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->NotifyItemAcquired(ItemId, Quantity);
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

	const FGuid ItemUid = CreateItemInstance(ItemId, Quantity);
	if (!AddItemUidToFirstEmptyCompatibleEquipmentSlot(ItemUid) &&
		!AddItemUidToFirstEmptySlot(ItemUid, PlayerInventorySlots))
	{
		ItemInstancesByUid.Remove(ItemUid);
		return false;
	}

	BroadcastInventoryStateChanged();
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->NotifyItemAcquired(ItemId, Quantity);
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

	TArray<FGuid> CreatedItemUids;
	for (const FTunaSweeperItemStack& ItemReward : ItemRewards)
	{
		if (ItemReward.ItemId == INDEX_NONE || ItemReward.Quantity <= 0)
		{
			continue;
		}

		const FGuid ItemUid = CreateItemInstance(ItemReward.ItemId, ItemReward.Quantity);
		if (!AddItemUidToFirstEmptySlot(ItemUid, PlayerInventorySlots))
		{
			ItemInstancesByUid.Remove(ItemUid);
			for (const FGuid& CreatedItemUid : CreatedItemUids)
			{
				ItemInstancesByUid.Remove(CreatedItemUid);
				for (FTunaSweeperInventorySlot& InventorySlot : PlayerInventorySlots)
				{
					if (InventorySlot.ItemUid == CreatedItemUid)
					{
						InventorySlot.Clear();
					}
				}
			}
			return false;
		}

		CreatedItemUids.Add(ItemUid);
	}

	if (CreatedItemUids.Num() > 0)
	{
		BroadcastInventoryStateChanged();
	}
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
	SaveGameStateInternal();
}

void UTunaSweeperGameInstance::ClearInventoryAndSave()
{
	EnsureInventoryStateInitialized();
	ClearSelectedItemSelection();
	ClearHoveredItemSlot();
	ItemInstancesByUid.Reset();
	ResetPlayerSlotArrays();
	UsableQuickSlots.Reset();
	EnsureSlotArraySize(UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	ClearRaidExperienceGain();
	SaveGameStateInternal(EUsableQuickSlotSaveMode::Clear);
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::HandleLevelTravelPersistence(FName SourceLevelName, FName TargetLevelName)
{
	if (IsRaidToBunkerTravel(SourceLevelName, TargetLevelName))
	{
		EnsureInventoryStateInitialized();
		FTunaSweeperExperienceAnimationState ExperienceAnimationState;
		CommitRaidExperienceGain(ExperienceAnimationState);
		SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime);
		return;
	}

	if (IsBunkerToRaidTravel(SourceLevelName, TargetLevelName))
	{
		SaveGameState();
		BeginRaidExperienceSession();
	}
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
	RemoveInvalidSlotReferences(PlayerInventorySlots);
	RemoveInvalidSlotReferences(EquipmentSlots);
	RemoveInvalidSlotReferences(AuxiliaryBagSlots);
	RemoveInvalidSlotReferences(UsableQuickSlots);

	EnsureSlotArraySize(EquipmentSlots, FMath::Max(TunaSweeperInventory::RequiredEquipmentSlots, GameplaySettings.EquipmentSlotCount));
	EnsureSlotArraySize(AuxiliaryBagSlots, FMath::Max(0, GameplaySettings.AuxiliaryBagSlotCount));
	EnsureSlotArraySize(UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
	for (FTunaSweeperInventorySlot& UsableQuickSlot : UsableQuickSlots)
	{
		if (UsableQuickSlot.ItemUid.IsValid() && !IsItemCompatibleWithUsableQuickSlot(UsableQuickSlot.ItemUid))
		{
			UsableQuickSlot.Clear();
		}
	}
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
	ActiveLootContainerOwner.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
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
	SaveGame->LastSavedAtTicks = FDateTime::Now().GetTicks();
	SaveGame->TotalExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
	SaveGame->CompletedScenarioFlags = CompletedScenarioFlags.Array();
	SaveGame->AcquiredMemoIds = AcquiredMemoIds.Array();
	SaveGame->AcquiredMemoIds.Sort();
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
	LoadedSlotTotalPlaySeconds = 0.0f;
	ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
	TotalExperiencePoints = 0;
	RaidStartExperiencePoints = 0;
	PendingRaidExperiencePoints = 0;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	CompletedScenarioFlags.Reset();
	AcquiredMemoIds.Reset();
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	WorldProgressStatesById.Reset();
	HousingFacilities.Reset();
	UnlockedHousingFacilityIds.Reset();
	PendingScenarioCompletionFlag = NAME_None;
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ResetQuestProgressForNewGame();
	}
}

void UTunaSweeperGameInstance::GenerateDefaultInventoryState()
{
	ItemInstancesByUid.Reset();
	TotalExperiencePoints = 0;
	RaidStartExperiencePoints = 0;
	PendingRaidExperiencePoints = 0;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	CompletedScenarioFlags.Reset();
	AcquiredMemoIds.Reset();
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	WorldProgressStatesById.Reset();
	HousingFacilities.Reset();
	UnlockedHousingFacilityIds.Reset();
	PendingScenarioCompletionFlag = NAME_None;
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ResetQuestProgressForNewGame();
	}
	ResetPlayerSlotArrays();
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
		!FMath::IsNearlyZero(ItemDefinition.UseHydrationDelta);
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

bool UTunaSweeperGameInstance::CanGrantQuestItemRewards(const TArray<FTunaSweeperItemStack>& ItemRewards) const
{
	int32 RequiredInventorySlots = 0;
	for (const FTunaSweeperItemStack& ItemReward : ItemRewards)
	{
		if (ItemReward.ItemId != INDEX_NONE && ItemReward.Quantity > 0)
		{
			++RequiredInventorySlots;
		}
	}

	if (RequiredInventorySlots <= 0)
	{
		return true;
	}

	int32 EmptyInventorySlots = 0;
	for (const FTunaSweeperInventorySlot& InventorySlot : PlayerInventorySlots)
	{
		if (InventorySlot.IsEmpty())
		{
			++EmptyInventorySlots;
		}
	}

	return EmptyInventorySlots >= RequiredInventorySlots;
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

void UTunaSweeperGameInstance::CollectPlayerOwnedItemUids(
	TSet<FGuid>& OutItemUids,
	bool bIncludeUsableQuickSlots) const
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
	if (bIncludeUsableQuickSlots)
	{
		CollectSlots(UsableQuickSlots);
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
