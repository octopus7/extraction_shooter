#pragma once

#include "CoreMinimal.h"
#include "Combat/TunaSweeperCombatValue.h"

class AActor;
class UTunaSweeperItemDataSubsystem;
struct FDamageEvent;

namespace TunaSweeperArmor
{
	// Tier 0 preserves legacy / non-ballistic defense. Tiers 1..4 are gameplay tiers.
	inline float EffectiveDefense(int32 Defense, int32 ArmorTier, int32 PenetrationTier)
	{
		const float RetainedFraction = ArmorTier <= 0 || PenetrationTier <= 0 ? 1.0f :
			FMath::Clamp(0.5f + 0.25f * (FMath::Clamp(ArmorTier, 1, 4) - FMath::Clamp(PenetrationTier, 1, 4)), 0.0f, 1.0f);
		return static_cast<float>(FMath::Max(0, Defense)) * RetainedFraction;
	}

	inline float ApplyDefense(float Damage, float Defense)
	{
		return TunaSweeperCombatValue::Round(Damage - FMath::Max(0.0f, Defense));
	}

	TUNASWEEPER_API int32 ResolvePenetrationTier(const FDamageEvent& Event, const AActor* Causer);
	TUNASWEEPER_API float ItemDefense(UTunaSweeperItemDataSubsystem* Items, int32 ItemId,
		FName RequiredSlot, int32 PenetrationTier);
}
