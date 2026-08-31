#include "Achievement/TunaSweeperAchievementModel.h"

namespace TunaSweeperAchievementModel
{
	namespace
	{
		bool IsDefinitionSatisfied(
			const FTunaSweeperAchievementDefinition& Definition,
			const FTunaSweeperAchievementProgressState& State)
		{
			switch (Definition.ConditionType)
			{
			case ETunaSweeperAchievementConditionType::SpecificEnemyFirstKill:
				return State.KilledEnemyIds.Contains(Definition.TargetId);
			case ETunaSweeperAchievementConditionType::TotalEnemyKills:
				return State.TotalEnemyKills >= Definition.RequiredCount;
			case ETunaSweeperAchievementConditionType::LocationReached:
				return State.ReachedLocationIds.Contains(Definition.TargetId);
			case ETunaSweeperAchievementConditionType::QuestRewardClaimed:
				return State.ClaimedQuestIds.Contains(Definition.TargetId);
			default:
				return false;
			}
		}
	}

	bool ValidateDefinitions(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FString& OutError)
	{
		OutError.Reset();
		TSet<FName> AchievementIds;
		TMap<FName, TSet<FString>> PlatformIds;

		for (const FTunaSweeperAchievementDefinition& Definition : Definitions)
		{
			if (Definition.AchievementId.IsNone())
			{
				OutError = TEXT("achievement_id must not be empty");
				return false;
			}
			if (AchievementIds.Contains(Definition.AchievementId))
			{
				OutError = FString::Printf(TEXT("duplicate achievement_id: %s"), *Definition.AchievementId.ToString());
				return false;
			}
			AchievementIds.Add(Definition.AchievementId);

			if (Definition.ConditionType == ETunaSweeperAchievementConditionType::TotalEnemyKills)
			{
				if (!Definition.TargetId.IsNone() || Definition.RequiredCount <= 0)
				{
					OutError = FString::Printf(
						TEXT("total_enemy_kills requires an empty target_id and required_count greater than zero: %s"),
						*Definition.AchievementId.ToString());
					return false;
				}
			}
			else if (Definition.TargetId.IsNone() || Definition.RequiredCount != 1)
			{
				OutError = FString::Printf(
					TEXT("target-based achievement requires target_id and required_count 1: %s"),
					*Definition.AchievementId.ToString());
				return false;
			}

			for (const TPair<FName, FString>& PlatformPair : Definition.PlatformIds)
			{
				const FString PlatformId = PlatformPair.Value.TrimStartAndEnd();
				if (PlatformPair.Key.IsNone() || PlatformId.IsEmpty())
				{
					OutError = FString::Printf(
						TEXT("platform name and id must not be empty: %s"),
						*Definition.AchievementId.ToString());
					return false;
				}

				TSet<FString>& IdsForPlatform = PlatformIds.FindOrAdd(PlatformPair.Key);
				if (IdsForPlatform.Contains(PlatformId))
				{
					OutError = FString::Printf(
						TEXT("duplicate platform achievement id for %s: %s"),
						*PlatformPair.Key.ToString(),
						*PlatformId);
					return false;
				}
				IdsForPlatform.Add(PlatformId);
			}
		}

		return true;
	}

	bool ValidateConfiguredPlatformIds(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FName PlatformName,
		const TArray<FString>& ConfiguredIds,
		FString& OutError)
	{
		OutError.Reset();
		TSet<FString> ExpectedIds;
		for (const FTunaSweeperAchievementDefinition& Definition : Definitions)
		{
			if (const FString* PlatformId = Definition.PlatformIds.Find(PlatformName))
			{
				ExpectedIds.Add(PlatformId->TrimStartAndEnd());
			}
		}

		TSet<FString> ActualIds;
		for (const FString& ConfiguredId : ConfiguredIds)
		{
			const FString TrimmedId = ConfiguredId.TrimStartAndEnd();
			if (TrimmedId.IsEmpty() || ActualIds.Contains(TrimmedId))
			{
				OutError = FString::Printf(TEXT("empty or duplicate configured %s achievement id"), *PlatformName.ToString());
				return false;
			}
			ActualIds.Add(TrimmedId);
		}

		if (ExpectedIds.Num() != ActualIds.Num())
		{
			OutError = FString::Printf(
				TEXT("%s achievement id count differs between JSON (%d) and config (%d)"),
				*PlatformName.ToString(),
				ExpectedIds.Num(),
				ActualIds.Num());
			return false;
		}

		for (const FString& ExpectedId : ExpectedIds)
		{
			if (!ActualIds.Contains(ExpectedId))
			{
				OutError = FString::Printf(
					TEXT("%s achievement id is missing from config: %s"),
					*PlatformName.ToString(),
					*ExpectedId);
				return false;
			}
		}

		return true;
	}

	bool RecordEnemyKilled(FTunaSweeperAchievementProgressState& State, FName EnemyId)
	{
		if (State.TotalEnemyKills < MAX_int64)
		{
			++State.TotalEnemyKills;
		}
		if (!EnemyId.IsNone())
		{
			State.KilledEnemyIds.Add(EnemyId);
		}
		return true;
	}

	bool RecordLocationReached(FTunaSweeperAchievementProgressState& State, FName LocationId)
	{
		if (LocationId.IsNone() || State.ReachedLocationIds.Contains(LocationId))
		{
			return false;
		}
		State.ReachedLocationIds.Add(LocationId);
		return true;
	}

	bool RecordQuestRewardClaimed(FTunaSweeperAchievementProgressState& State, FName QuestId)
	{
		if (QuestId.IsNone() || State.ClaimedQuestIds.Contains(QuestId))
		{
			return false;
		}
		State.ClaimedQuestIds.Add(QuestId);
		return true;
	}

	void EvaluateDefinitions(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FTunaSweeperAchievementProgressState& State,
		TArray<FName>& OutNewlyUnlockedIds)
	{
		OutNewlyUnlockedIds.Reset();
		for (const FTunaSweeperAchievementDefinition& Definition : Definitions)
		{
			if (!State.UnlockedAchievementIds.Contains(Definition.AchievementId) &&
				IsDefinitionSatisfied(Definition, State))
			{
				State.UnlockedAchievementIds.Add(Definition.AchievementId);
				OutNewlyUnlockedIds.Add(Definition.AchievementId);
			}
		}
	}

	bool MergePlatformState(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FName PlatformName,
		const TSet<FString>& UnlockedPlatformIds,
		FTunaSweeperAchievementProgressState& State,
		TArray<FName>& OutRemotelyUnlockedInternalIds)
	{
		OutRemotelyUnlockedInternalIds.Reset();
		bool bStateChanged = false;
		for (const FTunaSweeperAchievementDefinition& Definition : Definitions)
		{
			const FString* PlatformId = Definition.PlatformIds.Find(PlatformName);
			if (!PlatformId)
			{
				continue;
			}

			const FString UnlockKey = MakePlatformUnlockKey(PlatformName, *PlatformId);
			if (UnlockedPlatformIds.Contains(*PlatformId))
			{
				if (!State.UnlockedAchievementIds.Contains(Definition.AchievementId))
				{
					State.UnlockedAchievementIds.Add(Definition.AchievementId);
					OutRemotelyUnlockedInternalIds.Add(Definition.AchievementId);
					bStateChanged = true;
				}
				if (!State.ConfirmedPlatformUnlockKeys.Contains(UnlockKey))
				{
					State.ConfirmedPlatformUnlockKeys.Add(UnlockKey);
					bStateChanged = true;
				}
			}
			else if (State.ConfirmedPlatformUnlockKeys.Remove(UnlockKey) > 0)
			{
				bStateChanged = true;
			}
		}
		return bStateChanged;
	}

	bool FindNextPendingUnlock(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		const FTunaSweeperAchievementProgressState& State,
		FName PlatformName,
		const TSet<FName>& AttemptedInternalIds,
		FName& OutInternalId,
		FString& OutPlatformId)
	{
		OutInternalId = NAME_None;
		OutPlatformId.Reset();
		for (const FTunaSweeperAchievementDefinition& Definition : Definitions)
		{
			if (!State.UnlockedAchievementIds.Contains(Definition.AchievementId) ||
				AttemptedInternalIds.Contains(Definition.AchievementId))
			{
				continue;
			}

			const FString* PlatformId = Definition.PlatformIds.Find(PlatformName);
			if (!PlatformId || State.ConfirmedPlatformUnlockKeys.Contains(
				MakePlatformUnlockKey(PlatformName, *PlatformId)))
			{
				continue;
			}

			OutInternalId = Definition.AchievementId;
			OutPlatformId = *PlatformId;
			return true;
		}
		return false;
	}

	FString MakePlatformUnlockKey(FName PlatformName, const FString& PlatformAchievementId)
	{
		return PlatformName.ToString() + TEXT("|") + PlatformAchievementId.TrimStartAndEnd();
	}
}
