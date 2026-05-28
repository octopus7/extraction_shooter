#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"

bool UTunaSweeperWeaponSpreadRecoilDataAsset::TryGetDefinition(
	FName WeaponTypeTag,
	FTunaSweeperWeaponSpreadRecoilDefinition& OutDefinition) const
{
	OutDefinition = FTunaSweeperWeaponSpreadRecoilDefinition();
	if (WeaponTypeTag.IsNone())
	{
		return false;
	}

	for (const FTunaSweeperWeaponSpreadRecoilDefinition& Definition : WeaponTypeDefinitions)
	{
		if (Definition.WeaponTypeTag == WeaponTypeTag)
		{
			OutDefinition = Definition;
			return true;
		}
	}

	return false;
}
