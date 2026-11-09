#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

bool UTunaSweeperGameInstance::TryBeginWaveDefenseRun(FName EncounterId, int32 RewardCoins, int32 DifficultyStage, FGuid& OutRunId)
{
    OutRunId.Invalidate();
    EnsureInventoryStateInitialized();
    if (IsCombatTestSession() || EncounterId.IsNone() || RewardCoins <= 0 || DifficultyStage != ActiveSaveSlotDifficultyStage) return false;
    if (bWaveDefenseInterruptedNeedsSave)
    {
        if (!SaveGameStateInternal()) return false;
        bWaveDefenseInterruptedNeedsSave = false;
    }
    for (const auto& Pair : WaveDefenseProgress)
        if (Pair.Value.RunId.IsValid() && Pair.Value.Outcome == EWaveDefenseOutcome::None) return false;
    const FWaveDefenseProgress* Existing = WaveDefenseProgress.Find(EncounterId);
    if (Existing && Existing->bRewardPending) return false;
    const bool bHadRecord = Existing != nullptr;
    const FWaveDefenseProgress Previous = Existing ? *Existing : FWaveDefenseProgress();
    FWaveDefenseProgress Next = Previous;
    Next.RunId = FGuid::NewGuid();
    Next.DifficultyStage = DifficultyStage;
    Next.Outcome = EWaveDefenseOutcome::None;
    Next.LastCompletedWave = 0;
    Next.RewardCoins = RewardCoins;
    Next.bRewardPending = false;
    WaveDefenseProgress.Add(EncounterId, Next);
    if (!SaveGameStateInternal())
    {
        if (bHadRecord) WaveDefenseProgress.Add(EncounterId, Previous); else WaveDefenseProgress.Remove(EncounterId);
        return false;
    }
    OutRunId = Next.RunId;
    return true;
}

bool UTunaSweeperGameInstance::TryFinalizeWaveDefenseRun(FName EncounterId, FGuid InRunId, EWaveDefenseOutcome Outcome, int32 CompletedWave)
{
    EnsureInventoryStateInitialized();
    FWaveDefenseProgress* Record = WaveDefenseProgress.Find(EncounterId);
    if (!Record || !InRunId.IsValid() || Record->RunId != InRunId || Outcome == EWaveDefenseOutcome::None || CompletedWave < 0 || CompletedWave > 6) return false;
    if (Record->Outcome != EWaveDefenseOutcome::None) return Record->Outcome == Outcome && Record->LastCompletedWave == CompletedWave;
    if (Outcome == EWaveDefenseOutcome::Succeeded && (CompletedWave != 6 || Record->CompletedRuns == MAX_int32)) return false;
    const FWaveDefenseProgress Previous = *Record;
    Record->Outcome = Outcome;
    Record->LastCompletedWave = CompletedWave;
    Record->BestCompletedWave = FMath::Max(Record->BestCompletedWave, CompletedWave);
    Record->bRewardPending = Outcome == EWaveDefenseOutcome::Succeeded;
    if (Record->bRewardPending) ++Record->CompletedRuns;
    if (!SaveGameStateInternal()) { *Record = Previous; return false; }
    return true;
}

bool UTunaSweeperGameInstance::TryClaimWaveDefenseReward(FName EncounterId, FGuid InRunId)
{
    EnsureInventoryStateInitialized();
    FWaveDefenseProgress* Record = WaveDefenseProgress.Find(EncounterId);
    UTunaSweeperQuestSubsystem* Quests = GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!Record || !Quests || !InRunId.IsValid() || Record->RunId != InRunId || Record->Outcome != EWaveDefenseOutcome::Succeeded || !Record->bRewardPending || Record->RewardCoins <= 0) return false;
    Record->bRewardPending = false;
    const bool bCommitted = Quests->TryCommitCoinGrant(Record->RewardCoins, [this]() { return SaveGameStateInternal(); });
    if (!bCommitted) Record->bRewardPending = true;
    return bCommitted;
}

void UTunaSweeperGameInstance::StageWaveDefenseInterrupted(FName EncounterId, FGuid InRunId)
{
    FWaveDefenseProgress* Record = WaveDefenseProgress.Find(EncounterId);
    if (Record && InRunId.IsValid() && Record->RunId == InRunId && Record->Outcome == EWaveDefenseOutcome::None)
    {
        Record->Outcome = EWaveDefenseOutcome::Interrupted;
        Record->bRewardPending = false;
        bWaveDefenseInterruptedNeedsSave = true;
    }
}
