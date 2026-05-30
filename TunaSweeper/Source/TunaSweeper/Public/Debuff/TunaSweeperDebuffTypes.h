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
}

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

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float TickIntervalSeconds = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamagePerTick = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Debuff")
	FTunaSweeperDebuffCameraReactionSettings CameraReaction;

	void Normalize()
	{
		BaseApplyChance = TunaSweeperDataValues::ClampProbabilityValue(BaseApplyChance);
		DurationSeconds = FMath::Max(0.01f, DurationSeconds);
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
		RemainingSeconds = FMath::Max(0.0f, RemainingSeconds);
		DurationSeconds = FMath::Max(0.0f, DurationSeconds);
		TickIntervalSeconds = FMath::Max(0.01f, TickIntervalSeconds);
		DamagePerTick = FMath::Max(0.0f, DamagePerTick);
		TickAccumulator = FMath::Clamp(TickAccumulator, 0.0f, TickIntervalSeconds);
	}
};
