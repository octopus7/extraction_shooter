#pragma once

#include "Achievement/TunaSweeperAchievementTypes.h"

namespace TunaSweeperAchievementModel
{
	bool ValidateDefinitions(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FString& OutError);

	bool ValidateConfiguredPlatformIds(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FName PlatformName,
		const TArray<FString>& ConfiguredIds,
		FString& OutError);

	bool RecordEnemyKilled(FTunaSweeperAchievementProgressState& State, FName EnemyId);
	bool RecordLocationReached(FTunaSweeperAchievementProgressState& State, FName LocationId);
	bool RecordQuestRewardClaimed(FTunaSweeperAchievementProgressState& State, FName QuestId);

	void EvaluateDefinitions(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FTunaSweeperAchievementProgressState& State,
		TArray<FName>& OutNewlyUnlockedIds);

	bool MergePlatformState(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FName PlatformName,
		const TSet<FString>& UnlockedPlatformIds,
		FTunaSweeperAchievementProgressState& State,
		TArray<FName>& OutRemotelyUnlockedInternalIds);

	bool FindNextPendingUnlock(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		const FTunaSweeperAchievementProgressState& State,
		FName PlatformName,
		const TSet<FName>& AttemptedInternalIds,
		FName& OutInternalId,
		FString& OutPlatformId);

	FString MakePlatformUnlockKey(FName PlatformName, const FString& PlatformAchievementId);
}
