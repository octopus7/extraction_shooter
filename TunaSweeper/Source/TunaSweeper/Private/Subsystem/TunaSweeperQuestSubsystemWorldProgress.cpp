#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Game/TunaSweeperGameInstance.h"

void UTunaSweeperQuestSubsystem::HandleWorldProgressChanged()
{
    if (ReconcileWorldProgressObjectives(true))
    {
        // The world-state producer owns persistence, including inventory transactions.
        // Saving from this delegate would bypass its bSaveImmediately policy.
        BroadcastQuestProgressChanged(false);
    }
}

bool UTunaSweeperQuestSubsystem::ReconcileWorldProgressObjectives(bool bShowCompletionToast)
{
    auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
    if (!Game || Game->IsCombatTestSession() || !EnsureQuestDataLoaded() || bReconcilingWorldProgress) return false;
    TGuardValue<bool> Guard(bReconcilingWorldProgress, true);
    bool bChanged = false;
    TArray<int32> CompletedQuests;
    for (const auto& Pair : QuestDefinitions)
    {
        if (GetQuestState(Pair.Key) != ETunaSweeperQuestState::Accepted) continue;
        bool bHasCompletedWorldObjective = false;
        for (const auto& Objective : Pair.Value.Objectives)
        {
            if (Objective.Type != ETunaSweeperObjectiveType::WorldProgressCompleted ||
                Objective.WorldProgressId.IsNone() || Objective.RequiredCount != 1) continue;
            FTunaSweeperWorldProgressSaveData WorldState;
            if (Game->TryGetWorldProgressState(Objective.WorldProgressId, WorldState) &&
                WorldState.State == ETunaSweeperWorldProgressState::Completed)
            {
                bHasCompletedWorldObjective = true;
                bChanged |= AdvanceObjectiveProgress(Pair.Key, Objective.ObjectiveId, 1);
            }
        }
        if (bHasCompletedWorldObjective && AreAllObjectivesComplete(Pair.Key))
        {
            SetQuestState(Pair.Key, ETunaSweeperQuestState::RewardAvailable);
            CompletedQuests.Add(Pair.Key);
            bChanged = true;
        }
    }
    if (bShowCompletionToast)
        for (const int32 QuestId : CompletedQuests) ShowQuestCompletedToast(QuestId);
    return bChanged;
}
