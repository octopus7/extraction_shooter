#include "Game/TunaSweeperGameInstance.h"
#include "TunaSweeperGameInstanceShared.h"
#include "Effect/TunaSweeperOcclusionRevealSettingsDataAsset.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Title/TunaSweeperDisplaySettings.h"
#include "TunaWarpTransitionProfile.h"

DEFINE_LOG_CATEGORY(LogTunaSweeperGameInstance);

UTunaSweeperGameInstance::UTunaSweeperGameInstance()
{
	WarpTransitionProfile = TSoftObjectPtr<UTunaWarpTransitionProfile>(
		FSoftObjectPath(TEXT("/TunaWarpTransition/Profiles/DA_WarpTransition_Default.DA_WarpTransition_Default")));
	LevelTravelPresentationDataAsset = TSoftObjectPtr<UTunaSweeperLevelTravelPresentationDataAsset>(
		FSoftObjectPath(TEXT("/Game/Movies/DA_LevelTravelPresentation.DA_LevelTravelPresentation")));
}

UTunaWarpTransitionProfile* UTunaSweeperGameInstance::GetWarpTransitionProfile_Implementation() const
{
	return WarpTransitionProfile.LoadSynchronous();
}

UTunaSweeperOcclusionRevealSettingsDataAsset* UTunaSweeperGameInstance::GetOcclusionRevealSettingsDataAsset() const
{
	return OcclusionRevealSettingsDataAsset.LoadSynchronous();
}

bool UTunaSweeperGameInstance::TryResolveLevelTravel(
	ETunaSweeperLevelTravelDestination Destination,
	FName& OutTargetLevelName,
	FTunaSweeperLevelTravelPresentationDefinition& OutPresentation) const
{
	OutTargetLevelName = NAME_None;
	OutPresentation = FTunaSweeperLevelTravelPresentationDefinition();
	OutPresentation.Destination = Destination;

	switch (Destination)
	{
	case ETunaSweeperLevelTravelDestination::Bunker:
		OutTargetLevelName = TunaSweeperBuildFlavor::GetBunkerLevelName();
		break;
	case ETunaSweeperLevelTravelDestination::Raid:
		OutTargetLevelName = TunaSweeperBuildFlavor::GetRaidGameplayLevelName();
		break;
	default:
		return false;
	}

	if (const UTunaSweeperLevelTravelPresentationDataAsset* PresentationData =
		LevelTravelPresentationDataAsset.LoadSynchronous())
	{
		PresentationData->TryGetPresentation(Destination, OutPresentation);
	}

	return !OutTargetLevelName.IsNone();
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

	if (GEngine)
	{
		if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
		{
			GameUserSettings->LoadSettings(false);
			if (TunaSweeperDisplaySettings::ClampUnsupportedFullscreenResolution(*GameUserSettings))
			{
				GameUserSettings->ApplyResolutionSettings(false);
				GameUserSettings->SaveSettings();
			}
		}
	}

	InitializeGlobalLanguageSetting();
	TunaSweeperSave::PurgeLegacyFlatSaveFiles();
	TunaSweeperSave::PurgeOutdatedSaveFiles(TunaSweeperBuildFlavor::GetSaveGameDirectory());

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

