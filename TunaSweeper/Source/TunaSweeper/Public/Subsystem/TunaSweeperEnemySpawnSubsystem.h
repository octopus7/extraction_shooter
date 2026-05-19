#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperEnemySpawnSubsystem.generated.h"

class ATunaSweeperEnemyCharacter;
class ATunaSweeperLootContainerActor;
class UMaterialInterface;
class UWorld;

UCLASS()
class TUNASWEEPER_API UTunaSweeperEnemySpawnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool EnsureEnemiesSpawnedForWorld(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool EnsureRaidRuntimeActorsSpawnedForWorld(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool LoadEnemySpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadLootContainerSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Enemy Spawn")
	bool IsEnemySpawnDataLoaded() const { return bEnemySpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsLootContainerSpawnDataLoaded() const { return bLootContainerSpawnDataLoaded; }

private:
	struct FEnemySpawnDefinition
	{
		FName LevelName;
		TSoftClassPtr<ATunaSweeperEnemyCharacter> EnemyClass;
		TSoftObjectPtr<UMaterialInterface> BodyMaterial;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		int32 DropContainerDefinitionId = INDEX_NONE;
		int32 DropContentsId = INDEX_NONE;
		float MaxHealth = 30.0f;
	};

	struct FLootContainerSpawnDefinition
	{
		FName LevelName;
		TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		int32 ContainerDefinitionId = INDEX_NONE;
		int32 ContentsId = INDEX_NONE;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedEnemySpawnData();
	void ResetLoadedLootContainerSpawnData();
	FString GetEnemySpawnJsonPath() const;
	FString GetLootContainerSpawnJsonPath() const;
	bool DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const;

	TArray<FEnemySpawnDefinition> EnemySpawnDefinitions;
	TArray<FLootContainerSpawnDefinition> LootContainerSpawnDefinitions;

	TWeakObjectPtr<UWorld> LastSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bEnemySpawnDataLoaded = false;
	bool bLootContainerSpawnDataLoaded = false;
};
