#pragma once

#include "Achievement/TunaSweeperAchievementTypes.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperAchievementSubsystem.generated.h"

class ITunaSweeperAchievementPublisher;

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
	void ReportQuestRewardClaimed(FName QuestId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	int64 GetTotalEnemyKills() const { return ProgressState.TotalEnemyKills; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool HasKilledEnemy(FName EnemyId) const { return ProgressState.KilledEnemyIds.Contains(EnemyId); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool HasReachedLocation(FName LocationId) const { return ProgressState.ReachedLocationIds.Contains(LocationId); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool HasClaimedQuestReward(FName QuestId) const { return ProgressState.ClaimedQuestIds.Contains(QuestId); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Achievement")
	bool IsAchievementUnlocked(FName AchievementId) const
	{
		return ProgressState.UnlockedAchievementIds.Contains(AchievementId);
	}

private:
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
	TSharedPtr<ITunaSweeperAchievementPublisher> Publisher;
	TSet<FName> AttemptedPlatformWrites;
	FName CurrentPlatformName = NAME_None;
	FDelegateHandle PostLoadMapHandle;
	bool bDefinitionsLoaded = false;
	bool bProgressStateLoaded = false;
	bool bPlatformQueryInFlight = false;
	bool bPlatformQueryComplete = false;
	bool bPlatformWriteInFlight = false;
};
