#include "WaveDefense/WaveDefenseRun.h"
#include "WaveDefense/WaveDefenseDefinition.h"
bool FWaveDefenseRun::Start(const UWaveDefenseDefinition& Definition, FGuid InRunId)
{
    TArray<FName> Errors;
    if (Snapshot.Phase != EWaveDefensePhase::Idle || !InRunId.IsValid() || !Definition.ValidateDefinition(Errors)) return false;
    RunId = InRunId;
    Counts.Reset(); Caps.Reset(); AliveTokens.Reset(); UsedTokens.Reset();
    for (const FWaveDefenseWave& Wave : Definition.Waves) { Counts.Add(Wave.GetCount()); Caps.Add(Wave.AliveCap); }
    Snapshot.Phase = EWaveDefensePhase::Preparing;
    Snapshot.Wave = 1;
    Snapshot.TotalWaves = Counts.Num();
    Snapshot.Queued = Counts[0];
    Snapshot.CoreHealth = Snapshot.CoreMaxHealth = Definition.CoreMaxHealth;
    Snapshot.Supplies = Definition.InitialSupplies;
    Snapshot.SecondsRemaining = Definition.PreparationSeconds;
    IntermissionSeconds = Definition.IntermissionSeconds;
    WaveSupplies = Definition.WaveSupplies;
    return true;
}
void FWaveDefenseRun::StartWave()
{
    Snapshot.Phase = EWaveDefensePhase::Active;
    Snapshot.SecondsRemaining = 0;
    Snapshot.Queued = Counts[Snapshot.Wave - 1];
}
void FWaveDefenseRun::AdvanceTime(float GameSeconds)
{
    if (!FMath::IsFinite(GameSeconds) || GameSeconds <= 0 || (Snapshot.Phase != EWaveDefensePhase::Preparing && Snapshot.Phase != EWaveDefensePhase::Intermission)) return;
    Snapshot.SecondsRemaining = FMath::Max(0.0f, Snapshot.SecondsRemaining - GameSeconds);
    if (Snapshot.SecondsRemaining <= 0.0f) StartWave();
}
bool FWaveDefenseRun::NotifySpawned(FGuid EnemyToken)
{
    if (Snapshot.Phase != EWaveDefensePhase::Active || !EnemyToken.IsValid() || UsedTokens.Contains(EnemyToken) || Snapshot.Queued <= 0 || Snapshot.Alive >= Caps[Snapshot.Wave - 1]) return false;
    UsedTokens.Add(EnemyToken); AliveTokens.Add(EnemyToken); --Snapshot.Queued; ++Snapshot.Alive;
    return true;
}
bool FWaveDefenseRun::NotifyEnemyDied(FGuid EnemyToken)
{
    if (Snapshot.Phase != EWaveDefensePhase::Active || !AliveTokens.Remove(EnemyToken)) return false;
    --Snapshot.Alive;
    return true;
}
void FWaveDefenseRun::ResolveFrame(bool bPlayerDead, bool bCoreDestroyed)
{
    if (Snapshot.Phase != EWaveDefensePhase::Preparing && Snapshot.Phase != EWaveDefensePhase::Active && Snapshot.Phase != EWaveDefensePhase::Intermission) return;
    if (bPlayerDead || bCoreDestroyed)
    {
        Snapshot.Outcome = bPlayerDead ? EWaveDefenseOutcome::PlayerDied : EWaveDefenseOutcome::CoreDestroyed;
        Snapshot.Phase = EWaveDefensePhase::Resolving;
        return;
    }
    if (Snapshot.Phase != EWaveDefensePhase::Active || Snapshot.Queued != 0 || Snapshot.Alive != 0) return;
    if (Snapshot.Wave == Snapshot.TotalWaves) { Snapshot.Outcome = EWaveDefenseOutcome::Succeeded; Snapshot.Phase = EWaveDefensePhase::Resolving; return; }
    Snapshot.Supplies += WaveSupplies;
    ++Snapshot.Wave;
    Snapshot.Queued = Counts[Snapshot.Wave - 1];
    Snapshot.Phase = EWaveDefensePhase::Intermission;
    Snapshot.SecondsRemaining = IntermissionSeconds;
}
void FWaveDefenseRun::Abort(EWaveDefenseOutcome Reason)
{
    if (Reason == EWaveDefenseOutcome::None || Reason == EWaveDefenseOutcome::Succeeded || Snapshot.Phase == EWaveDefensePhase::Idle || Snapshot.Phase == EWaveDefensePhase::Finished) return;
    Snapshot.Outcome = Reason;
    Snapshot.Phase = EWaveDefensePhase::Resolving;
}
bool FWaveDefenseRun::TrySpendSupplies(int32 Amount)
{
    if (Amount <= 0 || Snapshot.Supplies < Amount || (Snapshot.Phase != EWaveDefensePhase::Preparing && Snapshot.Phase != EWaveDefensePhase::Intermission)) return false;
    Snapshot.Supplies -= Amount;
    return true;
}
