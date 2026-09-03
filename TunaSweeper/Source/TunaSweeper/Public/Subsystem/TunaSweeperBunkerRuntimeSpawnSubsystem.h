#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperBunkerRuntimeSpawnSubsystem.generated.h"

class ATunaSweeperMoleCompanionActor;
class UMaterialInterface;
class UStaticMesh;
class UWorld;

UCLASS()
class TUNASWEEPER_API UTunaSweeperBunkerRuntimeSpawnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Bunker Runtime Spawn")
	bool EnsureBunkerRuntimeActorsSpawnedForWorld(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Bunker Runtime Spawn")
	bool LoadBunkerCharacterSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Bunker Runtime Spawn")
	bool IsBunkerCharacterSpawnDataLoaded() const { return bBunkerCharacterSpawnDataLoaded; }

private:
	struct FBunkerCharacterSpawnDefinition
	{
		FName LevelName = NAME_None;
		FName SpawnId = NAME_None;
		TSoftClassPtr<ATunaSweeperMoleCompanionActor> ActorClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale = FVector::OneVector;
		TSoftObjectPtr<UStaticMesh> DummyMesh;
		TSoftObjectPtr<UMaterialInterface> VisualMaterial;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedBunkerCharacterSpawnData();
	FString GetBunkerCharacterSpawnJsonPath() const;
	bool DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const;

	TArray<FBunkerCharacterSpawnDefinition> BunkerCharacterSpawnDefinitions;
	TWeakObjectPtr<UWorld> LastSpawnedBunkerWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bBunkerCharacterSpawnDataLoaded = false;
};
