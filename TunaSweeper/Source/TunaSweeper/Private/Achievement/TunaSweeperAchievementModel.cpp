#include "Achievement/TunaSweeperAchievementModel.h"
#include "Quest/TunaSweeperQuestId.h"

namespace TunaSweeperAchievementModel
{
	namespace
	{
		bool IsDefinitionSatisfied(
			const FTunaSweeperAchievementDefinition& Definition,
			const FTunaSweeperAchievementProgressState& State,
			const FTunaSweeperAchievementSnapshot& Snapshot)
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
				return State.ClaimedQuestIds.Contains(Definition.TargetQuestId);
			case ETunaSweeperAchievementConditionType::GameplayEvent:
				return State.GameplayFacts.Contains(MakeGameplayFactKey(Definition.EventType, Definition.TargetId));
			case ETunaSweeperAchievementConditionType::AllFacilitiesRestored:
				if (Definition.RequiredFacilityIds.IsEmpty()) return false;
				for (FName Id : Definition.RequiredFacilityIds)
					if (Id.IsNone() || !Snapshot.RestoredFacilityIds.Contains(Id)) return false;
				return true;
			case ETunaSweeperAchievementConditionType::CurrentItemQuantity:
				return Definition.TargetItemId > 0 && Definition.RequiredCount > 0 &&
					Snapshot.OwnedItemCounts.FindRef(Definition.TargetItemId) >= Definition.RequiredCount;
			case ETunaSweeperAchievementConditionType::ExtractionHealth:
				return Snapshot.bSuccessfulExtraction && FMath::IsFinite(Snapshot.Health) &&
					FMath::IsFinite(Snapshot.MaxHealth) && Snapshot.Health > 0 && Snapshot.MaxHealth > 0 &&
					static_cast<double>(Snapshot.Health) / Snapshot.MaxHealth <= 0.10;
			case ETunaSweeperAchievementConditionType::TimedExtraction:
				return Snapshot.bSuccessfulExtraction && Snapshot.RaidKills >= Definition.RequiredCount &&
					FMath::IsFinite(Snapshot.RaidSeconds) && Snapshot.RaidSeconds >= 295.0 && Snapshot.RaidSeconds <= 305.0;
			default:
				return false;
			}
		}
	}

	bool CanUnlock(const FTunaSweeperAchievementDefinition& Definition)
	{
		return Definition.bEnabled && Definition.AchievementId != FName(TEXT("14")) &&
			Definition.ConditionType != ETunaSweeperAchievementConditionType::Reserved;
	}

	bool IsSupportedSave(const USaveGame& Save, FName BuildFlavor, FName DistributionNamespace)
	{
		const auto* AchievementSave = Cast<UTunaSweeperAchievementSaveGame>(&Save);
		return AchievementSave && AchievementSave->SaveVersion >= 1 && AchievementSave->SaveVersion <= 2 &&
			AchievementSave->BuildFlavor == BuildFlavor && AchievementSave->DistributionNamespace == DistributionNamespace;
	}

	FString MakeGameplayFactKey(ETunaSweeperAchievementEvent Event, FName TargetId)
	{
		return FString::Printf(TEXT("%d|%s"), static_cast<uint8>(Event), *TargetId.ToString());
	}

	bool RecordGameplayEvent(FTunaSweeperAchievementProgressState& State, ETunaSweeperAchievementEvent Event, FName TargetId)
	{
		if (static_cast<uint8>(Event) > static_cast<uint8>(ETunaSweeperAchievementEvent::EndingReached) ||
			(TargetId.IsNone() && Event != ETunaSweeperAchievementEvent::VehicleCollisionKill)) return false;
		const FString Key = MakeGameplayFactKey(Event, TargetId);
		if (State.GameplayFacts.Contains(Key)) return false;
		State.GameplayFacts.Add(Key);
		return true;
	}

	void RestoreProgress(const UTunaSweeperAchievementSaveGame& Save, FTunaSweeperAchievementProgressState& State)
	{
		State = {};
		State.TotalEnemyKills = FMath::Max<int64>(0, Save.TotalEnemyKills);
		for (FName Id : Save.KilledEnemyIds) if (!Id.IsNone()) State.KilledEnemyIds.Add(Id);
		for (FName Id : Save.ReachedLocationIds) if (!Id.IsNone()) State.ReachedLocationIds.Add(Id);
		for (int32 Id : Save.NumericClaimedQuestIds) if (Id > 0) State.ClaimedQuestIds.Add(Id);
		for (FName Id : Save.ClaimedQuestIds)
		{
			const int32 Numeric = TunaSweeperQuestId::FromLegacyString(Id.ToString());
			if (Numeric > 0) State.ClaimedQuestIds.Add(Numeric);
		}
		for (FName Id : Save.UnlockedAchievementIds)
			if (!Id.IsNone() && Id != FName(TEXT("14"))) State.UnlockedAchievementIds.Add(Id);
		for (const FString& Key : Save.ConfirmedPlatformUnlockKeys) if (!Key.IsEmpty()) State.ConfirmedPlatformUnlockKeys.Add(Key);
		for (const FString& Key : Save.GameplayFacts) if (!Key.IsEmpty()) State.GameplayFacts.Add(Key);
	}

	void CaptureProgress(const FTunaSweeperAchievementProgressState& State, UTunaSweeperAchievementSaveGame& Save)
	{
		Save.TotalEnemyKills = FMath::Max<int64>(0, State.TotalEnemyKills);
		Save.KilledEnemyIds = State.KilledEnemyIds.Array();
		Save.ReachedLocationIds = State.ReachedLocationIds.Array();
		Save.NumericClaimedQuestIds = State.ClaimedQuestIds.Array();
		Save.UnlockedAchievementIds = State.UnlockedAchievementIds.Array();
		Save.UnlockedAchievementIds.Remove(FName(TEXT("14")));
		Save.ConfirmedPlatformUnlockKeys = State.ConfirmedPlatformUnlockKeys.Array();
		Save.GameplayFacts = State.GameplayFacts.Array();
		Save.GameplayFacts.Sort();
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
			if (!CanUnlock(Definition)) continue;

			if (Definition.ConditionType == ETunaSweeperAchievementConditionType::CurrentItemQuantity)
			{
				if (Definition.TargetItemId <= 0 || Definition.RequiredCount <= 0) { OutError = TEXT("item quantity requires a positive item id and count"); return false; }
			}
			else if (Definition.ConditionType == ETunaSweeperAchievementConditionType::AllFacilitiesRestored)
			{
				TSet<FName> Ids;
				for (FName Id : Definition.RequiredFacilityIds)
				{
					if (Id.IsNone() || Ids.Contains(Id)) { OutError = TEXT("invalid facility list"); return false; }
					Ids.Add(Id);
				}
				if (Ids.IsEmpty()) { OutError = TEXT("enabled facility achievement requires a complete target list"); return false; }
			}
			else if (Definition.ConditionType == ETunaSweeperAchievementConditionType::ExtractionHealth ||
				Definition.ConditionType == ETunaSweeperAchievementConditionType::TimedExtraction)
			{
				if (Definition.RequiredCount <= 0) { OutError = TEXT("extraction count must be positive"); return false; }
			}
			else if (Definition.ConditionType == ETunaSweeperAchievementConditionType::GameplayEvent &&
				Definition.EventType == ETunaSweeperAchievementEvent::VehicleCollisionKill)
			{
				if (!Definition.TargetId.IsNone() || Definition.RequiredCount != 1) { OutError = TEXT("roadkill must accept any enemy"); return false; }
			}
			else if (Definition.ConditionType == ETunaSweeperAchievementConditionType::TotalEnemyKills)
			{
				if (!Definition.TargetId.IsNone() || Definition.RequiredCount <= 0)
				{
					OutError = FString::Printf(
						TEXT("total_enemy_kills requires an empty target_id and required_count greater than zero: %s"),
						*Definition.AchievementId.ToString());
					return false;
				}
			}
			else if ((Definition.ConditionType == ETunaSweeperAchievementConditionType::QuestRewardClaimed
				? Definition.TargetQuestId <= 0 : Definition.TargetId.IsNone()) || Definition.RequiredCount != 1)
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
			if (!CanUnlock(Definition)) continue;
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

	bool RecordQuestRewardClaimed(FTunaSweeperAchievementProgressState& State, int32 QuestId)
	{
		if ((QuestId <= 0) || State.ClaimedQuestIds.Contains(QuestId))
		{
			return false;
		}
		State.ClaimedQuestIds.Add(QuestId);
		return true;
	}

	void EvaluateDefinitions(
		const TArray<FTunaSweeperAchievementDefinition>& Definitions,
		FTunaSweeperAchievementProgressState& State,
		TArray<FName>& OutNewlyUnlockedIds,
		const FTunaSweeperAchievementSnapshot& Snapshot)
	{
		OutNewlyUnlockedIds.Reset();
		for (const FTunaSweeperAchievementDefinition& Definition : Definitions)
		{
			if (CanUnlock(Definition) && !State.UnlockedAchievementIds.Contains(Definition.AchievementId) &&
				IsDefinitionSatisfied(Definition, State, Snapshot))
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
			if (!CanUnlock(Definition)) continue;
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
			if (!CanUnlock(Definition) || !State.UnlockedAchievementIds.Contains(Definition.AchievementId) ||
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
