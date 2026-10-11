#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Game/TunaSweeperGameInstance.h"

bool UTunaSweeperQuestSubsystem::HasPossessionObjectives(int32 QuestId) const
{
    const auto* Definition = FindQuestDefinition(QuestId);
    return Definition && Definition->Objectives.ContainsByPredicate([](const auto& Objective)
    { return Objective.Type == ETunaSweeperObjectiveType::ItemPossessed; });
}

void UTunaSweeperQuestSubsystem::HandleQuestInventoryChanged()
{
    if (ReconcilePossessionObjectives(true)) BroadcastQuestProgressChanged(false);
}

bool UTunaSweeperQuestSubsystem::ReconcilePossessionObjectives(bool bShowCompletionToast)
{
    auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
    if (!Game || Game->IsCombatTestSession() || bReconcilingPossession || bQuestRewardClaimInProgress || !EnsureQuestDataLoaded()) return false;
    TGuardValue<bool> Guard(bReconcilingPossession, true);
    bool bChanged = false;
    TArray<int32> NewlyReady;
    for (const auto& Pair : QuestDefinitions)
    {
        const auto OldState = GetQuestState(Pair.Key);
        if ((OldState != ETunaSweeperQuestState::Accepted && OldState != ETunaSweeperQuestState::RewardAvailable) || !HasPossessionObjectives(Pair.Key)) continue;
        auto& Progress = GetOrCreateQuestProgress(Pair.Key);
        TMap<int32, int32> RemainingByItem;
        for (const auto& Objective : Pair.Value.Objectives)
        {
            if (Objective.Type != ETunaSweeperObjectiveType::ItemPossessed) continue;
            if (!RemainingByItem.Contains(Objective.ItemId)) RemainingByItem.Add(Objective.ItemId, Game->CountQuestSubmissionItemById(Objective.ItemId));
            int32& Available = RemainingByItem.FindChecked(Objective.ItemId);
            const int32 Count = FMath::Min(Available, Objective.RequiredCount);
            Available -= Count;
            auto* Saved = Progress.ObjectiveProgress.FindByPredicate([&](const auto& Value) { return Value.ObjectiveId == Objective.ObjectiveId; });
            if (!Saved)
            {
                FTunaSweeperObjectiveProgressSaveData New;
                New.ObjectiveId = Objective.ObjectiveId;
                Progress.ObjectiveProgress.Add(New);
                Saved = &Progress.ObjectiveProgress.Last();
            }
            bChanged |= Saved->CurrentCount != Count;
            Saved->CurrentCount = Count;
        }
        const auto NewState = AreAllObjectivesComplete(Pair.Key) ? ETunaSweeperQuestState::RewardAvailable : ETunaSweeperQuestState::Accepted;
        if (OldState != NewState)
        {
            SetQuestState(Pair.Key, NewState);
            bChanged = true;
            if (NewState == ETunaSweeperQuestState::RewardAvailable) NewlyReady.Add(Pair.Key);
        }
    }
    if (bShowCompletionToast) for (int32 Id : NewlyReady) ShowQuestCompletedToast(Id);
    return bChanged;
}
