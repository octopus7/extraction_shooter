#include "Game/TunaSweeperGameInstance.h"
#include "TunaSweeperGameInstanceShared.h"

DEFINE_LOG_CATEGORY(LogTunaSweeperGameInstance);

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

