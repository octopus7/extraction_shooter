#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperEnemySpawnSubsystem.generated.h"

class AActor;
class ATunaSweeperEnemyCharacter;
class ATunaSweeperLootContainerActor;
class ATunaSweeperTransparentObstacleActor;
class ATunaSweeperWorldProgressActor;
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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadTransparentObstacleSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadWorldProgressObjectSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Enemy Spawn")
	bool IsEnemySpawnDataLoaded() const { return bEnemySpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsLootContainerSpawnDataLoaded() const { return bLootContainerSpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsTransparentObstacleSpawnDataLoaded() const { return bTransparentObstacleSpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsWorldProgressObjectSpawnDataLoaded() const { return bWorldProgressObjectSpawnDataLoaded; }

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
		bool bEditorOnly = false;
	};

	struct FTransparentObstacleSpawnDefinition
	{
		FName LevelName;
		FName ObstacleId;
		TSoftClassPtr<ATunaSweeperTransparentObstacleActor> ObstacleClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent = FVector(260.0f, 45.0f, 140.0f);
	};

	struct FWorldProgressObjectSpawnDefinition
	{
		FName LevelName;
		FName ObjectId;
		FName InfoId;
		TSoftClassPtr<ATunaSweeperWorldProgressActor> ProgressActorClass;
		TSoftClassPtr<AActor> CompletedActorClass;
		FText DisplayName;
		FText InteractionDisplayName;
		FText RequiredItemDisplayName;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent = FVector(260.0f, 55.0f, 140.0f);
		int32 RequiredItemId = 6002;
		int32 RequiredQuantity = 2;
		int32 InitialProgressQuantity = 0;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedEnemySpawnData();
	void ResetLoadedLootContainerSpawnData();
	void ResetLoadedTransparentObstacleSpawnData();
	void ResetLoadedWorldProgressObjectSpawnData();
	FString GetEnemySpawnJsonPath() const;
	FString GetLootContainerSpawnJsonPath() const;
	FString GetTransparentObstacleSpawnJsonPath() const;
	FString GetWorldProgressObjectSpawnJsonPath() const;
	bool DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const;

	TArray<FEnemySpawnDefinition> EnemySpawnDefinitions;
	TArray<FLootContainerSpawnDefinition> LootContainerSpawnDefinitions;
	TArray<FTransparentObstacleSpawnDefinition> TransparentObstacleSpawnDefinitions;
	TArray<FWorldProgressObjectSpawnDefinition> WorldProgressObjectSpawnDefinitions;

	TWeakObjectPtr<UWorld> LastSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bEnemySpawnDataLoaded = false;
	bool bLootContainerSpawnDataLoaded = false;
	bool bTransparentObstacleSpawnDataLoaded = false;
	bool bWorldProgressObjectSpawnDataLoaded = false;
};
