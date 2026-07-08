#include "TunaSweeperGameInstanceShared.h"

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

