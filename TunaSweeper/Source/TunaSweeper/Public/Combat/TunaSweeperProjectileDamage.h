#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperProjectileDamage
{
	// Global headshot damage: 2x total damage means a 100 percent increase.
	inline constexpr float HeadshotDamageMultiplier = 2.0f;

	// One projectile, before target defense, difficulty or damage-over-time.
	// Multiply the unrounded base value, then round once before the flat bonus.
	inline int32 Calculate(float BaseDamage, float Multiplier, int32 Bonus)
	{
		return FMath::Max(0, FMath::RoundToInt(BaseDamage * FMath::Max(0.0f, Multiplier)) + Bonus);
	}
}
