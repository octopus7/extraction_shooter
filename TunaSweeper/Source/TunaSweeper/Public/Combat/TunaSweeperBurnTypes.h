#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperBurnTypes.generated.h"

/** A shot snapshots its burn strength so later research or ammunition changes cannot alter it. */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperBurnSpec
{
	GENERATED_BODY()

	static constexpr float TickIntervalSeconds = 1.0f;
	static constexpr int32 MaxTickCount = 300;
	static constexpr float MaxDamagePerTick = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burn")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burn", meta = (ClampMin = "0", ClampMax = "100000"))
	float BaseDamagePerTick = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burn", meta = (ClampMin = "1", ClampMax = "300"))
	int32 TickCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Burn", meta = (ClampMin = "0", ClampMax = "10000"))
	float DamageMultiplier = 1.0f;

	void Normalize()
	{
		BaseDamagePerTick = FMath::IsFinite(BaseDamagePerTick)
			? FMath::Clamp(BaseDamagePerTick, 0.0f, MaxDamagePerTick) : 0.0f;
		DamageMultiplier = FMath::IsFinite(DamageMultiplier)
			? FMath::Clamp(DamageMultiplier, 0.0f, 10000.0f) : 0.0f;
		TickCount = FMath::Clamp(TickCount, 1, MaxTickCount);
		bEnabled = bEnabled && BaseDamagePerTick > 0.0f && DamageMultiplier > 0.0f;
	}

	float GetDamagePerTick() const
	{
		FTunaSweeperBurnSpec SafeSpec = *this;
		SafeSpec.Normalize();
		return SafeSpec.bEnabled
			? FMath::Min(SafeSpec.BaseDamagePerTick * SafeSpec.DamageMultiplier, MaxDamagePerTick) : 0.0f;
	}

	float GetDurationSeconds() const
	{
		return FMath::Clamp(TickCount, 1, MaxTickCount) * TickIntervalSeconds;
	}
};
