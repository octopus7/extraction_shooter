#pragma once

#include "CoreMinimal.h"
#include "Housing/TunaSweeperHousingTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperHousingSubsystem.generated.h"

class APlayerController;
class AActor;
class ATunaSweeperHousingAreaActor;
class ATunaSweeperHousingFacilityActor;
class ATunaSweeperHousingGridVisualActor;
class ATunaSweeperHousingManagementActor;
class UWorld;

UCLASS()
class TUNASWEEPER_API UTunaSweeperHousingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool EnsureHousingForWorld(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void RegisterHousingArea(ATunaSweeperHousingAreaActor* HousingArea);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void UnregisterHousingArea(ATunaSweeperHousingAreaActor* HousingArea);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool LoadHousingFacilityDefinitions(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void GetFacilityViews(TArray<FTunaSweeperHousingFacilityView>& OutViews);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	bool TryGetFacilityDefinition(FName FacilityId, FTunaSweeperHousingFacilityDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool OpenHousingMode(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void CloseHousingMode();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool UpdateHousingMode(APlayerController* PlayerController);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	bool IsHousingModeOpen() const { return bHousingModeOpen; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool StartPlacement(FName FacilityId, FGuid ExistingInstanceId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void CancelPlacement();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool RotateActivePlacement(int32 QuarterTurnDelta);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool UpdatePlacementPreview(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool TryCommitPlacement(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool StoreFacility(FGuid InstanceId, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool TryGetPlacedFacilityAtMouse(
		APlayerController* PlayerController,
		FTunaSweeperHousingPlacedFacilitySaveData& OutFacility);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	bool HasActivePlacement() const { return ActivePlacement.FacilityId != NAME_None; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	ETunaSweeperHousingPlacementStatus GetActivePlacementStatus() const { return ActivePlacementStatus; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	ATunaSweeperHousingAreaActor* GetActiveHousingArea() const { return ActiveHousingArea.Get(); }

	FSimpleMulticastDelegate OnHousingStateChanged;

private:
	struct FActiveHousingPlacement
	{
		FName FacilityId = NAME_None;
		FGuid ExistingInstanceId;
		FIntPoint AnchorCell = FIntPoint::ZeroValue;
		int32 RotationQuarterTurns = 0;
		bool bHasPreviewCell = false;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedFacilityDefinitions();
	void ResetRuntimeWorldState();
	void LoadSavedFacilitiesFromGameInstance();
	void PersistSavedFacilitiesToGameInstance(bool bSaveImmediately);
	void RefreshSpawnedFacilities();
	void DestroySpawnedFacilities();
	void DestroyPreviewActor();
	void BroadcastHousingChanged();
	bool IsBunkerWorld(const UWorld* World) const;
	bool DoesAreaBelongToWorld(const ATunaSweeperHousingAreaActor* HousingArea, const UWorld* World) const;
	ATunaSweeperHousingAreaActor* ResolveActiveAreaForWorld(UWorld* World);
	ATunaSweeperHousingAreaActor* SpawnDefaultHousingArea(UWorld* World);
	ATunaSweeperHousingManagementActor* EnsureHousingManagementActorForWorld(UWorld* World);
	bool TryGetMouseWorldPoint(APlayerController* PlayerController, FVector& OutWorldPoint) const;
	bool TryResolvePlacementAtMouse(
		APlayerController* PlayerController,
		FTunaSweeperHousingPlacedFacilitySaveData& OutPlacement,
		ETunaSweeperHousingPlacementStatus& OutStatus) const;
	bool ValidatePlacement(
		const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
		const FGuid& IgnoredInstanceId,
		ETunaSweeperHousingPlacementStatus& OutStatus) const;
	bool DoFootprintsOverlap(
		const FTunaSweeperHousingPlacedFacilitySaveData& LeftPlacement,
		const FTunaSweeperHousingFacilityDefinition& LeftDefinition,
		const FTunaSweeperHousingPlacedFacilitySaveData& RightPlacement,
		const FTunaSweeperHousingFacilityDefinition& RightDefinition) const;
	FIntPoint GetRotatedFootprintSize(
		const FTunaSweeperHousingFacilityDefinition& Definition,
		int32 RotationQuarterTurns) const;
	FTransform BuildFacilityWorldTransform(
		const FTunaSweeperHousingFacilityDefinition& Definition,
		const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const;
	FTransform BuildPlacedActorWorldTransform(
		const FTunaSweeperHousingFacilityDefinition& Definition,
		const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const;
	TSubclassOf<AActor> ResolveFacilityActorClass(const FTunaSweeperHousingFacilityDefinition& Definition) const;
	FName BuildPlacedFacilityActorId(const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const;
	void ConfigurePlacedFacilityActor(
		AActor* Actor,
		const FTunaSweeperHousingFacilityDefinition& Definition,
		const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const;
	void ConfigureFacilityActor(
		AActor* Actor,
		const FTunaSweeperHousingFacilityDefinition& Definition,
		const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
		bool bPreview,
		bool bPlacementValid) const;
	bool TryGetDefinition(FName FacilityId, FTunaSweeperHousingFacilityDefinition& OutDefinition) const;
	bool IsFacilityFunctionUnlocked(const FTunaSweeperHousingFacilityDefinition& Definition) const;
	bool HasEnoughMaterials(const FTunaSweeperHousingFacilityDefinition& Definition) const;
	bool ConsumeRequiredMaterials(const FTunaSweeperHousingFacilityDefinition& Definition);
	FText ResolveFacilityDisplayName(const FTunaSweeperHousingFacilityDefinition& Definition) const;
	FText ResolveFacilityDescription(const FTunaSweeperHousingFacilityDefinition& Definition) const;
	FText BuildMaterialsText(const FTunaSweeperHousingFacilityDefinition& Definition) const;
	FText BuildStateText(ETunaSweeperHousingFacilityBuildState BuildState) const;
	FString GetHousingFacilityDefinitionsJsonPath() const;
	void RefreshGridVisual(
		const FTunaSweeperHousingPlacedFacilitySaveData* HighlightPlacement,
		bool bHighlightValid = true);
	ATunaSweeperHousingGridVisualActor* EnsureGridVisualActor();
	void DestroyGridVisualActor();

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperHousingFacilityDefinition> FacilityDefinitionsById;

	UPROPERTY(Transient)
	TArray<FTunaSweeperHousingFacilityDefinition> FacilityDefinitions;

	UPROPERTY(Transient)
	TArray<FTunaSweeperHousingPlacedFacilitySaveData> SavedFacilities;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATunaSweeperHousingAreaActor>> RegisteredHousingAreas;

	UPROPERTY(Transient)
	TWeakObjectPtr<ATunaSweeperHousingAreaActor> ActiveHousingArea;

	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AActor>> SpawnedFacilityActors;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviewActor;

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperHousingGridVisualActor> GridVisualActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<ATunaSweeperHousingManagementActor> RuntimeHousingManagementActor;

	FActiveHousingPlacement ActivePlacement;
	ETunaSweeperHousingPlacementStatus ActivePlacementStatus = ETunaSweeperHousingPlacementStatus::None;
	FDelegateHandle PostLoadMapHandle;
	bool bFacilityDefinitionsLoaded = false;
	bool bSavedFacilitiesLoaded = false;
	bool bHousingModeOpen = false;
};
