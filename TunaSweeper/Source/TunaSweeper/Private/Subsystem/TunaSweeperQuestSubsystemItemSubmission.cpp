#include "Subsystem/TunaSweeperQuestSubsystem.h"

#include "Game/TunaSweeperGameInstance.h"
#include "Templates/UnrealTemplate.h"

bool UTunaSweeperQuestSubsystem::TryGetItemSubmissionQuestForProvider(FName ProviderId, FName& OutQuestId) const
{
	OutQuestId = NAME_None;
	const auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (ProviderId.IsNone() || !Game || Game->IsCombatTestSession() || !EnsureQuestDataLoaded()) return false;
	EnsureSaveStateLoaded();
	TArray<FTunaSweeperQuestDefinition> Definitions;
	GetAllQuestDefinitions(Definitions);
	for (const auto& Definition : Definitions)
	{
		const auto State = GetQuestState(Definition.QuestId);
		if (State != ETunaSweeperQuestState::Accepted && State != ETunaSweeperQuestState::RewardAvailable) continue;
		for (const auto& Objective : Definition.Objectives)
		{
			if (Objective.Type != ETunaSweeperObjectiveType::ItemSubmitted || Objective.TargetProviderId != ProviderId) continue;
			if (State == ETunaSweeperQuestState::RewardAvailable)
			{
				OutQuestId = Definition.QuestId;
				return true;
			}
			if (OutQuestId.IsNone() && GetObjectiveProgressCount(Definition.QuestId, Objective.ObjectiveId) < Objective.RequiredCount)
				OutQuestId = Definition.QuestId;
		}
	}
	return !OutQuestId.IsNone();
}

bool UTunaSweeperQuestSubsystem::TrySubmitItemsToProvider(
	FName ProviderId, FName& OutQuestId, bool bSaveImmediately)
{
	OutQuestId = NAME_None;
	auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (ProviderId.IsNone() || !Game || Game->IsCombatTestSession() || bItemSubmissionInProgress || !EnsureQuestDataLoaded()) return false;
	EnsureSaveStateLoaded();
	TGuardValue<bool> SubmissionGuard(bItemSubmissionInProgress, true);
	TArray<FTunaSweeperQuestDefinition> Definitions;
	GetAllQuestDefinitions(Definitions);
	// Stable authoring order; retry completed submissions before consuming for a new quest.
	Definitions.StableSort([this](const auto& Left, const auto& Right)
	{
		return GetQuestState(Left.QuestId) == ETunaSweeperQuestState::RewardAvailable &&
			GetQuestState(Right.QuestId) != ETunaSweeperQuestState::RewardAvailable;
	});
	for (const auto& Definition : Definitions)
	{
		const auto State = GetQuestState(Definition.QuestId);
		if (State != ETunaSweeperQuestState::Accepted && State != ETunaSweeperQuestState::RewardAvailable) continue;
		TArray<FTunaSweeperItemStack> Requirements;
		TArray<TPair<FName, int32>> Objectives;
		bool bMatchesProvider = false;
		for (const auto& Objective : Definition.Objectives)
		{
			if (Objective.Type != ETunaSweeperObjectiveType::ItemSubmitted || Objective.TargetProviderId != ProviderId) continue;
			bMatchesProvider = true;
			if (Objective.ItemId <= 0 || Objective.RequiredCount <= 0) return false;
			const int32 Remaining = Objective.RequiredCount - GetObjectiveProgressCount(Definition.QuestId, Objective.ObjectiveId);
			if (Remaining <= 0) continue;
			FTunaSweeperItemStack Requirement;
			Requirement.ItemId = Objective.ItemId;
			Requirement.Quantity = Remaining;
			Requirements.Add(Requirement);
			Objectives.Emplace(Objective.ObjectiveId, Remaining);
		}
		if (!bMatchesProvider) continue;
		if (State == ETunaSweeperQuestState::RewardAvailable)
		{
			if (!Requirements.IsEmpty()) continue;
			OutQuestId = Definition.QuestId;
			return true;
		}
		if (Requirements.IsEmpty()) continue;
		if (!Game->TryConsumeInventoryItems(Requirements, [this, &Definition, &Objectives]()
		{
			for (const auto& Objective : Objectives)
			{
				AdvanceObjectiveProgress(Definition.QuestId, Objective.Key, Objective.Value);
			}
			if (AreAllObjectivesComplete(Definition.QuestId))
			{
				SetQuestState(Definition.QuestId, ETunaSweeperQuestState::RewardAvailable);
			}
		})) continue;
		OutQuestId = Definition.QuestId;
		if (GetQuestState(Definition.QuestId) == ETunaSweeperQuestState::RewardAvailable) ShowQuestCompletedToast(Definition.QuestId);
		BroadcastQuestProgressChanged(bSaveImmediately);
		return true;
	}
	return false;
}
