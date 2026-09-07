#pragma once

#include "Combat/TunaSweeperBurnTypes.h"
#include "Research/TunaSweeperResearchTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

namespace TunaSweeperBurn
{
	// Evaluate from the loaded round at fire time; research never ignites ordinary ammunition by itself.
	inline FTunaSweeperBurnSpec ResolveWeaponBurnSpec(
		const FTunaSweeperItemDefinition& Weapon,
		const FTunaSweeperItemDefinition& Ammo,
		const FTunaSweeperResearchBurnBonuses& Research)
	{
		FTunaSweeperBurnSpec Spec;
		const bool bWeaponBurns = FMath::IsFinite(Weapon.BurnDamagePerTick) && Weapon.BurnDamagePerTick > 0.0f;
		const bool bAmmoBurns = FMath::IsFinite(Ammo.BurnDamagePerTick) && Ammo.BurnDamagePerTick > 0.0f;
		Spec.bEnabled = bWeaponBurns || bAmmoBurns;
		if (!Spec.bEnabled)
		{
			return Spec;
		}

		Spec.BaseDamagePerTick = FMath::Max(
			bWeaponBurns ? Weapon.BurnDamagePerTick : 0.0f,
			bAmmoBurns ? Ammo.BurnDamagePerTick : 0.0f);
		const int32 BaseTicks = FMath::Max(
			bWeaponBurns ? Weapon.BurnTickCount : 0,
			bAmmoBurns ? Ammo.BurnTickCount : 0);
		Spec.TickCount = static_cast<int32>(FMath::Clamp<int64>(
			static_cast<int64>(BaseTicks) + FMath::Max(0, Research.AdditionalTickCount), 1, MAX_int32));
		Spec.DamageMultiplier = Research.DamageMultiplier;
		Spec.Normalize();
		return Spec;
	}
}
