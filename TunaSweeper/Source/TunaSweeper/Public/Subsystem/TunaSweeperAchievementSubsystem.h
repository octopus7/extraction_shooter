#pragma once

#include "Achievement/TunaSweeperAchievementTypes.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperAchievementSubsystem.generated.h"

class ITunaSweeperAchievementPublisher;
class APawn;

DECLARE_MULTICAST_DELEGATE_OneParam(FTunaSweeperAchievementUnlockedDelegate, FName);

UCLASS()
class TUNASWEEPER_API UTunaSweeperAchievementSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FTunaSweeperAchievementUnlockedDelegate OnAchievementUnlocked;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Achievement")
	bool LoadAchievementDefinitions(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Achievement")
	void ReportEnemyKilled(FName EnemyId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Achievement")
	void ReportLocationReached(FName LocationId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Achievement")
	void ReportQuestRewardClaimed(int32 QuestId);

	// Only call after the owning gameplay system commits the actual action/state.
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Achievement")
	void ReportGameplayEvent(ETunaSweeperAchievementEvent Event, FName TargetId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Achievement")
	void ReportWeaponFired(int32 WeaponItemId);

	// Called by the local controller once its gameplay world is ready (also supports PIE).
	void HandleGameplayWorldReady(UWorld* World);
	void CaptureCompletedExtraction(APawn* Player, FName Destination);
	void CancelRaid();
	// Non-extraction travel must discard any previous completed-hold evidence.
	void DiscardPendingExtraction(UWorld* World);
	void PrepareRegionTravel(UWorld* World, FName Destination);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	TArray<FTunaSweeperAchievementDefinition> GetAchievementDefinitions() const { return Definitions; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	FText GetAchievementName(FName AchievementId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	int64 GetTotalEnemyKills() const { return ProgressState.TotalEnemyKills; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool HasKilledEnemy(FName EnemyId) const { return ProgressState.KilledEnemyIds.Contains(EnemyId); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool HasReachedLocation(FName LocationId) const { return ProgressState.ReachedLocationIds.Contains(LocationId); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool HasClaimedQuestReward(int32 QuestId) const { return ProgressState.ClaimedQuestIds.Contains(QuestId); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool IsAchievementUnlocked(FName AchievementId) const
	{
		return AchievementId != FName(TEXT("14")) && ProgressState.UnlockedAchievementIds.Contains(AchievementId);
	}

private:
	friend class FTunaAchievementLifecycleTest;
	bool IsGameplayReportingAllowed() const;
	void RefreshGameplaySnapshot();
	void FlushPendingProgress();
	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	bool LoadProgressState();
	bool SaveProgressState() const;
	void ProcessProgressChanged(bool bStateChanged);
	void BroadcastUnlocked(const TArray<FName>& AchievementIds);
	void TryStartPlatformSync();
	void HandlePlatformQueryComplete(bool bSuccess, TSet<FString> UnlockedPlatformIds);
	void PublishNextPendingAchievement();
	bool HasPlatformMappings(FName PlatformName) const;
	bool ValidatePlatformConfiguration(FName PlatformName) const;
	bool ReadSteamConfiguredAchievementIds(TArray<FString>& OutIds, FString& OutError) const;
	const FTunaSweeperAchievementDefinition* FindDefinition(FName AchievementId) const;
	FString GetDefinitionsPath() const;
	FName GetDistributionNamespace() const;
	FString GetProgressSavePath() const;

	TArray<FTunaSweeperAchievementDefinition> Definitions;
	FTunaSweeperAchievementProgressState ProgressState;
	FTunaSweeperAchievementSnapshot GameplaySnapshot;
	FTunaSweeperAchievementSnapshot PendingExtraction;
	FTunaSweeperAchievementRaidState RaidState;
	double PendingRegionRaidSeconds = 0;
	FName PendingRegionDestination;
	FName PendingExtractionDestination;
	TWeakObjectPtr<UWorld> GameplayWorld;
	TArray<FName> PendingNotifications;
	bool bRefreshingSnapshot = false;
	bool bProgressDirty = false;
	TSharedPtr<ITunaSweeperAchievementPublisher> Publisher;
	TSet<FName> AttemptedPlatformWrites;
	FName CurrentPlatformName = NAME_None;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle TravelFailureHandle;
	bool bDefinitionsLoaded = false;
	bool bProgressStateLoaded = false;
	bool bPlatformQueryInFlight = false;
	bool bPlatformQueryComplete = false;
	bool bPlatformWriteInFlight = false;
};
