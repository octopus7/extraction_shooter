#pragma once

#include "CoreMinimal.h"
#include "Research/TunaSweeperResearchTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperResearchSubsystem.generated.h"

enum class ETunaSweeperResearchNotificationMode : uint8
{
	Immediate,
	Deferred
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperResearchSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	FSimpleMulticastDelegate OnResearchStateChanged;
	FSimpleMulticastDelegate OnResearchEffectsChanged;
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Research") bool LoadResearchData(bool bForceReload = false);
	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Research") bool GetAllNodeViews(TArray<FTunaSweeperResearchNodeView>& OutViews) const;
	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Research") bool GetNodeView(FName NodeId, FTunaSweeperResearchNodeView& OutView) const;
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Research") bool TryStartResearch(FName NodeId);
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Research") bool TryClaimResearch(FName NodeId);
	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Research") int32 GetAppliedNodeCount() const { return AppliedNodeIds.Num(); }
	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Research") FTunaSweeperResearchStatBonuses GetAppliedStatBonuses() const;
	void ExportResearchProgressForSave(TArray<FName>& OutAppliedNodeIds, TArray<FTunaSweeperActiveResearchSaveData>& OutActiveResearch, int64& OutLastObservedUtcTicks) const;
	void LoadResearchProgressFromSave(
		const TArray<FName>& SavedAppliedNodeIds,
		const TArray<FTunaSweeperActiveResearchSaveData>& SavedActiveResearch,
		int64 SavedLastObservedUtcTicks,
		ETunaSweeperResearchNotificationMode NotificationMode = ETunaSweeperResearchNotificationMode::Immediate);
	void ResetResearchProgressForNewGame(
		ETunaSweeperResearchNotificationMode NotificationMode = ETunaSweeperResearchNotificationMode::Immediate);
	void FlushDeferredResearchNotifications();
private:
	void NotifyResearchProgressChanged(ETunaSweeperResearchNotificationMode NotificationMode);
	bool TickResearch(float DeltaSeconds);
	void RefreshTemporalState(bool bSaveIfChanged);
	bool EnsureResearchDataLoaded() const;
	ETunaSweeperResearchNodeState EvaluateNodeState(const FTunaSweeperResearchNodeDefinition& Definition, const FTunaSweeperActiveResearchSaveData** OutActive = nullptr) const;
	int64 GetEffectiveUtcTicks() const;
	void EnsureSaveStateLoaded() const;
	void RequestSaveGameState() const;
	FText ResolveLocalizedText(const FString& Korean, const FString& English) const;
	TMap<FName, FTunaSweeperResearchNodeDefinition> Definitions;
	TSet<FName> AppliedNodeIds;
	TArray<FTunaSweeperActiveResearchSaveData> ActiveResearch;
	int64 LastObservedUtcTicks = 0;
	double SessionStartPlatformSeconds = 0.0;
	int64 SessionStartUtcTicks = 0;
	bool bResearchDataLoaded = false;
	bool bProgressLoaded = false;
	bool bResearchEffectsNotificationPending = false;
	bool bResearchStateNotificationPending = false;
	FTSTicker::FDelegateHandle TickerHandle;
};
