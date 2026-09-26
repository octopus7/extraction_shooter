#pragma once

#include "CoreMinimal.h"

enum class ETunaSweeperBossTactic : uint8 { Balanced, KeepDistance, Advance };

struct TUNASWEEPER_API FTunaSweeperBossModuleDefinition
{
	FName Id;
	FName NameKey;
	FVector HalfExtent;
	FLinearColor Color;
	float Health = 100.f;
	float Damage = 0.f;
	bool bWeapon = false;
};

struct TUNASWEEPER_API FTunaSweeperBossPart
{
	int32 InstanceId = 0;
	FName ModuleId;
	int32 ParentId = INDEX_NONE;
	int32 SocketIndex = 0;
	int32 YawSteps = 0;
};

struct TUNASWEEPER_API FTunaSweeperBossDefinition
{
	int32 Version = 1;
	int32 CatalogVersion = 1;
	FGuid BossId = FGuid::NewGuid();
	FString Name;
	TArray<FTunaSweeperBossPart> Parts;
	ETunaSweeperBossTactic Tactic = ETunaSweeperBossTactic::Balanced;
	float AttackInterval = 2.5f;
	float PhaseThreshold = 0.5f;
	bool bAlternateWeapons = true;
};

namespace TunaSweeperBossDefinition
{
	inline constexpr int32 MaxParts = 32;
	inline constexpr int32 MaxDepth = 8;
	inline constexpr int32 MaxFileBytes = 256 * 1024;
	inline constexpr int32 SlotCount = 12;
	TUNASWEEPER_API const TArray<FTunaSweeperBossModuleDefinition>& GetCatalog();
	TUNASWEEPER_API const FTunaSweeperBossModuleDefinition* FindModule(FName Id);
	TUNASWEEPER_API FTunaSweeperBossDefinition MakeDefault();
	TUNASWEEPER_API bool Validate(const FTunaSweeperBossDefinition& Definition, FName& OutError);
	TUNASWEEPER_API bool BuildTransforms(const FTunaSweeperBossDefinition& Definition, TMap<int32, FTransform>& OutTransforms);
	TUNASWEEPER_API bool FromJson(const FString& Json, FTunaSweeperBossDefinition& OutDefinition, FName& OutError);
	TUNASWEEPER_API bool ToJson(const FTunaSweeperBossDefinition& Definition, FString& OutJson, FName& OutError);
	TUNASWEEPER_API void RemoveBranch(FTunaSweeperBossDefinition& Definition, int32 InstanceId);
}
