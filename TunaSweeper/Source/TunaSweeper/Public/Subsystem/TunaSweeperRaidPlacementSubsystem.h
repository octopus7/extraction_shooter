#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCombatProfile.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "Raid/TunaSweeperRaidPlacementAnchor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperRaidPlacementSubsystem.generated.h"

class AActor;
class ATunaSweeperEnemyCharacter;
class ATunaSweeperLootContainerActor;
class UMaterialInterface;
class UWorld;

/** Resolves level-owned raid placement anchors with external enemy/loot data at runtime. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperRaidPlacementSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Placement")
	bool EnsureRaidPlacementActorsSpawnedForWorld(UWorld* World);

	/** Sets the seed for the current raid before its map is loaded. It is intentionally runtime-only. */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Placement")
	void SetRaidSeed(int32 InRaidSeed);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Placement")
	int32 GetRaidSeed() const { return RaidSeed; }

	/** Order-independent deterministic roll in [0, 1) for a placement. */
	static float GetDeterministicPlacementRoll(int32 InRaidSeed, int32 PlacementId);

private:
	struct FEnemySpawnProfile
	{
		FName ProfileId;
		TSoftClassPtr<ATunaSweeperEnemyCharacter> EnemyClass;
		TSoftObjectPtr<UMaterialInterface> BodyMaterial;
		FName CombatProfileId;
		int32 DropContainerDefinitionId = INDEX_NONE;
		int32 DropContentsId = INDEX_NONE;
		int32 WeaponItemId = INDEX_NONE;
		int32 AmmoItemId = INDEX_NONE;
		int32 ReserveAmmoCount = INDEX_NONE;
		float LootLoadedAmmoDeductionRatio = 0.35f;
		int32 LootLoadedAmmoFlatDeduction = 0;
		int32 ExperienceValue = 30;
		float MaxHealth = 30.0f;
		int32 BleedingChanceBonus = 0;
		float BleedingDurationBonusSeconds = 0.0f;
		uint8 FactionId = TunaSweeperFactionIds::NoFaction;
		FName SquadId;
		int32 SquadSlot = INDEX_NONE;
	};

	struct FEnemyPlacementDefinition
	{
		FName LevelId;
		int32 PlacementId = INDEX_NONE;
		FName ProfileId;
		float SpawnChance = 1.0f;
		FName ConditionId;
	};

	struct FLootPlacementDefinition
	{
		FName LevelId;
		int32 PlacementId = INDEX_NONE;
		TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerClass;
		int32 ContainerDefinitionId = INDEX_NONE;
		int32 ContentsId = INDEX_NONE;
		float SpawnChance = 1.0f;
		FName ConditionId;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	bool LoadData(bool bForceReload = false);
	bool LoadEnemyProfiles(const FString& JsonPath);
	bool LoadEnemyPlacements(const FString& JsonPath);
	bool LoadLootPlacements(const FString& JsonPath);
	void ResetLoadedData();
	FString GetEnemyProfilesJsonPath() const;
	FString GetEnemyPlacementsJsonPath() const;
	FString GetLootPlacementsJsonPath() const;
	bool DoesLevelIdMatchWorld(FName LevelId, const UWorld* World) const;
	bool DoesSpawnConditionPass(FName ConditionId, const FString& DebugId) const;
	bool ShouldSpawnAtPlacement(int32 PlacementId, float SpawnChance) const;
	FName MakeRuntimeInstanceId(FName LevelId, int32 PlacementId) const;

	TMap<FName, FEnemySpawnProfile> EnemyProfilesById;
	TArray<FEnemyPlacementDefinition> EnemyPlacementDefinitions;
	TArray<FLootPlacementDefinition> LootPlacementDefinitions;
	TWeakObjectPtr<UWorld> LastSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	int32 RaidSeed = 0;
	bool bRaidSeedExplicitlySet = false;
	bool bDataLoaded = false;
};
