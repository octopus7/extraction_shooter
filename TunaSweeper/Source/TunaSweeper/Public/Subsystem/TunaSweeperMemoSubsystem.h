#pragma once

#include "CoreMinimal.h"
#include "Memo/TunaSweeperMemoTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperMemoSubsystem.generated.h"

class ATunaSweeperMemoActor;
class UMaterialInterface;
class UStaticMesh;
class UWorld;
class UTunaSweeperInteractionMarkerWidget;

UCLASS()
class TUNASWEEPER_API UTunaSweeperMemoSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool LoadMemoDefinitions(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool LoadMemoSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool EnsureMemosSpawnedForWorld(UWorld* World);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Memo")
	bool IsMemoDefinitionDataLoaded() const { return bMemoDefinitionDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Memo")
	bool IsMemoSpawnDataLoaded() const { return bMemoSpawnDataLoaded; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool TryGetMemoDefinition(int32 MemoId, FTunaSweeperMemoDefinition& OutDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	void GetMemoListEntries(TArray<FTunaSweeperMemoListEntry>& OutEntries);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	int32 GetFirstAcquiredMemoId();

private:
	struct FMemoSpawnDefinition
	{
		// Future extension: map-placed marker actors can be matched by MemoId and used as the source
		// for the runtime spawn transform and visual mesh/material fields below.
		FName LevelName;
		FName SpawnId;
		int32 MemoId = INDEX_NONE;
		TSoftClassPtr<ATunaSweeperMemoActor> MemoActorClass;
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass;
		TSoftObjectPtr<UStaticMesh> VisualMesh;
		TSoftObjectPtr<UMaterialInterface> VisualMaterial;
		FText InteractionDisplayName;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector ActorScale = FVector::OneVector;
		FVector VisualScale = FVector(0.85f, 0.55f, 0.08f);
		FVector VisualRelativeLocation = FVector::ZeroVector;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedMemoDefinitions();
	void ResetLoadedMemoSpawnData();
	FString GetMemoDefinitionsJsonPath() const;
	FString GetMemoSpawnsJsonPath() const;
	bool DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const;

	TMap<int32, FTunaSweeperMemoDefinition> MemoDefinitionsById;
	TArray<FMemoSpawnDefinition> MemoSpawnDefinitions;
	TWeakObjectPtr<UWorld> LastSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bMemoDefinitionDataLoaded = false;
	bool bMemoSpawnDataLoaded = false;
};
