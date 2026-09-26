#pragma once

#include "CoreMinimal.h"

/** Item-specific mesh overrides. Gun actor classes remain in WeaponActorClassMappings.json. */
struct FTunaSweeperWeaponVisualDefinition
{
	FSoftObjectPath Mesh;
	FSoftObjectPath Material;
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FVector Scale = FVector::OneVector;
	bool bAlignLongestAxisToX = false;
	bool bCenterOnBounds = false;
	float FitLengthCm = 0.f;
};

struct FTunaSweeperEnemyDefaultLoadout
{
	int32 WeaponItemId = INDEX_NONE;
	int32 ReserveMagazineCount = 0;
	TMap<FName, int32> AmmoItemIdsByWeaponType;
};

struct FTunaSweeperEnemyWeaponLoadout
{
	int32 WeaponItemId = INDEX_NONE;
	int32 AmmoItemId = INDEX_NONE;
	int32 ReserveAmmoCount = 0;
};
