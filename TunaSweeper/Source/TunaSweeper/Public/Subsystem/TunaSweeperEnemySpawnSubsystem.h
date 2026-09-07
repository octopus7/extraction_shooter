#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCombatProfile.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperEnemySpawnSubsystem.generated.h"

class ATunaSweeperLootContainerActor;
class UWorld;

/** Owns enemy combat-profile lookup and the retained loot-container spawn data. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperEnemySpawnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool LoadEnemyCombatProfileData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool TryGetEnemyCombatProfile(FName ProfileId, FTunaSweeperEnemyCombatProfile& OutProfile);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Spawn")
	bool EnsureLootContainersSpawnedForWorld(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Spawn")
	bool LoadLootContainerSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Enemy Spawn")
	bool IsEnemyCombatProfileDataLoaded() const { return bEnemyCombatProfileDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Spawn")
	bool IsLootContainerSpawnDataLoaded() const { return bLootContainerSpawnDataLoaded; }

private:
	struct FLootContainerSpawnDefinition
	{
		FName LevelName;
		TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		int32 ContainerDefinitionId = INDEX_NONE;
		int32 ContentsId = INDEX_NONE;
		bool bEditorOnly = false;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedEnemyCombatProfileData();
	void ResetLoadedLootContainerSpawnData();
	FString GetEnemyCombatProfileJsonPath() const;
	FString GetLootContainerSpawnJsonPath() const;
	bool DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const;

	TMap<FName, FTunaSweeperEnemyCombatProfile> EnemyCombatProfilesById;
	TArray<FLootContainerSpawnDefinition> LootContainerSpawnDefinitions;
	TWeakObjectPtr<UWorld> LastLootSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bEnemyCombatProfileDataLoaded = false;
	bool bLootContainerSpawnDataLoaded = false;
};
