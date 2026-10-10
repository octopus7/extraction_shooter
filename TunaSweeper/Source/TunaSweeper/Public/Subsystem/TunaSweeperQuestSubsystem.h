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
	static int32 GetFirstOutingQuestId();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	static FName GetMoleProviderId();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool LoadQuestData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool IsQuestDataLoaded() const { return bQuestDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool TryGetQuestDefinition(int32 QuestId, FTunaSweeperQuestDefinition& OutDefinition) const;

	const FTunaSweeperQuestDefinition* FindQuestDefinition(int32 QuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool GetAllQuestDefinitions(TArray<FTunaSweeperQuestDefinition>& OutDefinitions) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool TryGetQuestTextByKey(FName StringKey, ETunaSweeperItemTextLanguage Language, FText& OutText) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool GetQuestPresentationLines(
		int32 QuestId,
		ETunaSweeperQuestPresentationTrigger Trigger,
		TArray<FTunaSweeperQuestPresentationLineView>& OutLines) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	ETunaSweeperQuestState GetQuestState(int32 QuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool AreQuestPrerequisitesMet(int32 QuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool CanAcceptQuest(int32 QuestId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool AcceptQuest(int32 QuestId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool CanClaimQuestReward(int32 QuestId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool ClaimQuestReward(int32 QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool SetTrackedQuest(int32 QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void ClearTrackedQuest();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	int32 GetTrackedQuestId() const { return TrackedQuestId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool GetQuestObjectiveProgress(int32 QuestId, TArray<FTunaSweeperObjectiveProgressView>& OutProgress) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool TryResolveQuestForProvider(FName ProviderId, int32 FallbackQuestId, int32& OutQuestId) const;

	bool TryGetLatestQuestInProviderChain(FName ProviderId, int32& OutQuestId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyLevelTravelRequested(FName SourceLevelName, FName TargetLevelName);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyBunkerRescueReturn(FName SourceLevelName, FName TargetLevelName);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyWarpPointUsed(FName LevelName, FName WarpPointId, FName TargetWarpPointId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyItemAcquired(int32 ItemId, int32 Quantity, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyEnemyKilled(FName EnemyId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyInteractionCompleted(FName InteractionEventId, FName InteractionTypeName);

	// One eligible quest per interaction; an already submitted quest can retry its reward without consuming again.
	bool TrySubmitItemsToProvider(FName ProviderId, int32& OutQuestId, bool bSaveImmediately = true);
	bool TryGetItemSubmissionQuestForProvider(FName ProviderId, int32& OutQuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	int32 GetCoinBalance() const { return CoinBalance; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void AddCoins(int32 Amount, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool TrySpendCoins(int32 Amount, bool bSaveImmediately = true);

	void ExportQuestProgressForSave(
		TArray<FTunaSweeperNumericQuestProgressSaveData>& OutQuestProgress,
		int32& OutTrackedQuestId,
		int32& OutQuestCoinBalance) const;
	void LoadQuestProgressFromSave(
		const TArray<FTunaSweeperNumericQuestProgressSaveData>& SavedQuestProgress,
		int32 SavedTrackedQuestId,
		int32 SavedQuestCoinBalance);
	void ResetQuestProgressForNewGame();

private:
	friend class FTunaSweeperQuestSubmissionTest;
	bool bItemSubmissionInProgress = false;
	bool EnsureQuestDataLoaded() const;
	bool LoadQuestDefinitionsJson();
	bool LoadQuestTextStringsCsv();
	void ResetLoadedQuestData();
	FString GetQuestDefinitionsJsonPath() const;
	void ResolveDefinitionText(FTunaSweeperQuestDefinition& Definition) const;
	FText ResolveQuestText(FName StringKey, const FText& FallbackText = FText::GetEmpty()) const;
	bool IsMapNameMatch(FName ActualMapName, const TCHAR* ExpectedMapName) const;
	bool DoesObjectiveMatchLevelTravel(const FTunaSweeperObjectiveDefinition& Objective, FName SourceLevelName, FName TargetLevelName) const;
	bool DoesObjectiveMatchBunkerRescueReturn(const FTunaSweeperObjectiveDefinition& Objective, FName SourceLevelName, FName TargetLevelName) const;
	bool DoesObjectiveMatchWarpPointUsed(const FTunaSweeperObjectiveDefinition& Objective, FName LevelName, FName WarpPointId, FName TargetWarpPointId) const;
	bool DoesObjectiveMatchItemAcquired(const FTunaSweeperObjectiveDefinition& Objective, int32 ItemId) const;
	bool DoesObjectiveMatchEnemyKilled(const FTunaSweeperObjectiveDefinition& Objective, FName EnemyId) const;
	bool DoesObjectiveMatchInteractionCompleted(const FTunaSweeperObjectiveDefinition& Objective, FName InteractionEventId, FName InteractionTypeName) const;
	bool AdvanceObjectiveProgress(int32 QuestId, FName ObjectiveId, int32 Amount);
	void AdvanceMatchingObjectives(
		TFunctionRef<bool(const FTunaSweeperObjectiveDefinition&)> Predicate,
		int32 Amount,
		bool bSaveImmediately = true);
	void SetQuestState(int32 QuestId, ETunaSweeperQuestState NewState);
	void ShowQuestCompletedToast(int32 QuestId) const;
	bool AreDefinitionPrerequisitesMet(const FTunaSweeperQuestDefinition& Definition) const;
	bool IsQuestForProvider(const FTunaSweeperQuestDefinition& Definition, FName ProviderId) const;
	bool AreAllObjectivesComplete(int32 QuestId) const;
	int32 GetObjectiveProgressCount(int32 QuestId, FName ObjectiveId) const;
	FTunaSweeperNumericQuestProgressSaveData& GetOrCreateQuestProgress(int32 QuestId);
	void BroadcastQuestProgressChanged(bool bSaveImmediately);
	void EnsureSaveStateLoaded() const;
	void RequestSaveGameState() const;
	bool IsQuestTrackable(int32 QuestId) const;

	TMap<int32, FTunaSweeperQuestDefinition> QuestDefinitions;
	TMap<FName, FTunaSweeperQuestTextString> QuestTextStringsByKey;
	TMap<int32, FTunaSweeperNumericQuestProgressSaveData> QuestProgressById;
	int32 TrackedQuestId = 0;
	int32 CoinBalance = 0;
	bool bQuestDataLoaded = false;
};
