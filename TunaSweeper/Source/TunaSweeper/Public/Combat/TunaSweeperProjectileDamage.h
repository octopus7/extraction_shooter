#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperProjectileDamage
{
	// Global headshot damage: 2x total damage means a 100 percent increase.
	inline constexpr float HeadshotDamageMultiplier = 2.0f;

	// One projectile, before target defense, difficulty or damage-over-time.
	// Preserve precision through headshots, difficulty and armor; the receiver rounds final damage.
	inline float Calculate(float BaseDamage, float Multiplier, int32 Bonus)
	{
		return FMath::Max(0.0f, BaseDamage * FMath::Max(0.0f, Multiplier) + Bonus);
	}
}
