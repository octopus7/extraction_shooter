#pragma once

#include "CoreMinimal.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "TunaSweeperDebuffTypes.generated.h"

namespace TunaSweeperDebuff
{
	FORCEINLINE FName BleedingDebuffId()
	{
		static const FName DebuffId(TEXT("debuff.bleeding"));
		return DebuffId;
	}

	FORCEINLINE FName OverweightDebuffId()
	{
		static const FName DebuffId(TEXT("debuff.overweight"));
		return DebuffId;
	}

	FORCEINLINE FName MovementBlockedDebuffId()
	{
		static const FName DebuffId(TEXT("debuff.movement_blocked"));
		return DebuffId;
	}
}

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperCarryWeightDebuffSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff|Carry", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseStrength = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff|Carry", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float KgPerStrength = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff|Carry", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 OverweightThreshold = 7000;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff|Carry", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 MovementBlockedThreshold = 10000;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff|Carry", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 OverweightSpeedMultiplier = 5000;

	void Normalize()
	{
		BaseStrength = FMath::Max(0.0f, BaseStrength);
		KgPerStrength = FMath::Max(0.01f, KgPerStrength);
		OverweightThreshold = TunaSweeperDataValues::ClampRatioValue(OverweightThreshold);
		MovementBlockedThreshold = FMath::Max(
			OverweightThreshold,
			TunaSweeperDataValues::ClampRatioValue(MovementBlockedThreshold));
		OverweightSpeedMultiplier = TunaSweeperDataValues::ClampRatioValue(OverweightSpeedMultiplier);
	}

	float GetOverweightThresholdRatio() const
	{
		return TunaSweeperDataValues::ToRatioFloat(TunaSweeperDataValues::ClampRatioValue(OverweightThreshold));
	}

	float GetMovementBlockedThresholdRatio() const
	{
		return TunaSweeperDataValues::ToRatioFloat(TunaSweeperDataValues::ClampRatioValue(MovementBlockedThreshold));
	}

	float GetOverweightSpeedMultiplier() const
	{
		return TunaSweeperDataValues::ToRatioFloat(TunaSweeperDataValues::ClampRatioValue(OverweightSpeedMultiplier));
	}
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperDebuffCameraReactionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Debuff|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DurationSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Debuff|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LocationAmplitude = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Debuff|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollAmplitudeDegrees = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Debuff|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FOVAmplitudeDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Debuff|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Frequency = 22.0f;

	void Normalize()
	{
		DurationSeconds = FMath::Max(0.0f, DurationSeconds);
		LocationAmplitude = FMath::Max(0.0f, LocationAmplitude);
		RollAmplitudeDegrees = FMath::Max(0.0f, RollAmplitudeDegrees);
		FOVAmplitudeDegrees = FMath::Max(0.0f, FOVAmplitudeDegrees);
		Frequency = FMath::Max(0.0f, Frequency);
	}
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperDebuffDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	FName DebuffId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	FName NameStringKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	FString IconFileName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 BaseApplyChance = 400;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DurationSeconds = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	bool bHasDuration = true;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float TickIntervalSeconds = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamagePerTick = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	FTunaSweeperDebuffCameraReactionSettings CameraReaction;

	void Normalize()
	{
		BaseApplyChance = TunaSweeperDataValues::ClampProbabilityValue(BaseApplyChance);
		DurationSeconds = bHasDuration ? FMath::Max(0.01f, DurationSeconds) : 0.0f;
		TickIntervalSeconds = FMath::Max(0.01f, TickIntervalSeconds);
		DamagePerTick = FMath::Max(0.0f, DamagePerTick);
		CameraReaction.Normalize();
	}
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperActiveDebuffState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	FName DebuffId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	bool bHasDuration = true;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float TickIntervalSeconds = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamagePerTick = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TickAccumulator = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	int32 AppliedOrder = 0;

	void Normalize()
	{
		RemainingSeconds = bHasDuration ? FMath::Max(0.0f, RemainingSeconds) : 0.0f;
		DurationSeconds = bHasDuration ? FMath::Max(0.0f, DurationSeconds) : 0.0f;
		TickIntervalSeconds = FMath::Max(0.01f, TickIntervalSeconds);
		DamagePerTick = FMath::Max(0.0f, DamagePerTick);
		TickAccumulator = FMath::Clamp(TickAccumulator, 0.0f, TickIntervalSeconds);
	}
};
