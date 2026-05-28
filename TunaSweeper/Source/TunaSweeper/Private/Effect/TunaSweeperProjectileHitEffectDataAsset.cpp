#include "Effect/TunaSweeperProjectileHitEffectDataAsset.h"

bool UTunaSweeperProjectileHitEffectDataAsset::TryGetHitEffect(
	FName EffectId,
	FTunaSweeperProjectileHitEffectDefinition& OutDefinition) const
{
	OutDefinition = FTunaSweeperProjectileHitEffectDefinition();
	if (EffectId.IsNone())
	{
		return false;
	}

	for (const FTunaSweeperProjectileHitEffectDefinition& Definition : HitEffects)
	{
		if (Definition.EffectId == EffectId)
		{
			OutDefinition = Definition;
			return true;
		}
	}

	return false;
}
