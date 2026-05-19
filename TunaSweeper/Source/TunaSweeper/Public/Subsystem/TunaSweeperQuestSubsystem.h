#pragma once

#include "CoreMinimal.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperQuestSubsystem.generated.h"

UCLASS()
class TUNASWEEPER_API UTunaSweeperQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	FSimpleMulticastDelegate OnQuestProgressChanged;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	static FName GetFirstOutingQuestId();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool LoadQuestData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool IsQuestDataLoaded() const { return bQuestDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool TryGetQuestDefinition(FName QuestId, FTunaSweeperQuestDefinition& OutDefinition) const;

	const FTunaSweeperQuestDefinition* FindQuestDefinition(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	ETunaSweeperQuestState GetQuestState(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool CanAcceptQuest(FName QuestId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool AcceptQuest(FName QuestId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool CanClaimQuestReward(FName QuestId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool ClaimQuestReward(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool SetTrackedQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void ClearTrackedQuest();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FName GetTrackedQuestId() const { return TrackedQuestId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool GetQuestObjectiveProgress(FName QuestId, TArray<FTunaSweeperObjectiveProgressView>& OutProgress) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyLevelTravelRequested(FName SourceLevelName, FName TargetLevelName);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyItemAcquired(int32 ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyEnemyKilled(FName EnemyId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyInteractionCompleted(FName InteractionEventId, FName InteractionTypeName);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	int32 GetCoinBalance() const { return CoinBalance; }

	void ExportQuestProgressForSave(
		TArray<FTunaSweeperQuestProgressSaveData>& OutQuestProgress,
		FName& OutTrackedQuestId,
		int32& OutQuestCoinBalance) const;
	void LoadQuestProgressFromSave(
		const TArray<FTunaSweeperQuestProgressSaveData>& SavedQuestProgress,
		FName SavedTrackedQuestId,
		int32 SavedQuestCoinBalance);
	void ResetQuestProgressForNewGame();

private:
	bool EnsureQuestDataLoaded() const;
	bool LoadQuestDefinitionsJson();
	void RegisterFallbackQuest();
	void ResetLoadedQuestData();
	FString GetQuestDefinitionsJsonPath() const;
	bool IsMapNameMatch(FName ActualMapName, const TCHAR* ExpectedMapName) const;
	bool DoesObjectiveMatchLevelTravel(const FTunaSweeperObjectiveDefinition& Objective, FName SourceLevelName, FName TargetLevelName) const;
	bool DoesObjectiveMatchItemAcquired(const FTunaSweeperObjectiveDefinition& Objective, int32 ItemId) const;
	bool DoesObjectiveMatchEnemyKilled(const FTunaSweeperObjectiveDefinition& Objective, FName EnemyId) const;
	bool DoesObjectiveMatchInteractionCompleted(const FTunaSweeperObjectiveDefinition& Objective, FName InteractionEventId, FName InteractionTypeName) const;
	bool AdvanceObjectiveProgress(FName QuestId, FName ObjectiveId, int32 Amount);
	void AdvanceMatchingObjectives(TFunctionRef<bool(const FTunaSweeperObjectiveDefinition&)> Predicate, int32 Amount);
	void SetQuestState(FName QuestId, ETunaSweeperQuestState NewState);
	bool AreAllObjectivesComplete(FName QuestId) const;
	int32 GetObjectiveProgressCount(FName QuestId, FName ObjectiveId) const;
	FTunaSweeperQuestProgressSaveData& GetOrCreateQuestProgress(FName QuestId);
	void BroadcastQuestProgressChanged(bool bSaveImmediately);
	void EnsureSaveStateLoaded() const;
	void RequestSaveGameState() const;
	bool IsQuestTrackable(FName QuestId) const;

	TMap<FName, FTunaSweeperQuestDefinition> QuestDefinitions;
	TMap<FName, FTunaSweeperQuestProgressSaveData> QuestProgressById;
	FName TrackedQuestId = NAME_None;
	int32 CoinBalance = 0;
	bool bQuestDataLoaded = false;
};
