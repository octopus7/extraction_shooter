#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperEnemySpawnSubsystem.generated.h"

class ATunaSweeperEnemyCharacter;
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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool LoadEnemySpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Enemy Spawn")
	bool IsEnemySpawnDataLoaded() const { return bEnemySpawnDataLoaded; }

private:
	struct FEnemySpawnDefinition
	{
		FName LevelName;
		TSoftClassPtr<ATunaSweeperEnemyCharacter> EnemyClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedEnemySpawnData();
	FString GetEnemySpawnJsonPath() const;
	bool DoesSpawnMatchWorld(const FEnemySpawnDefinition& SpawnDefinition, const UWorld* World) const;

	TArray<FEnemySpawnDefinition> EnemySpawnDefinitions;

	TWeakObjectPtr<UWorld> LastSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bEnemySpawnDataLoaded = false;
};
