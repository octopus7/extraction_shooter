#pragma once
#include "CoreMinimal.h"
#include "WaveDefense/WaveDefenseTypes.h"
class UWaveDefenseDefinition;
class TUNASWEEPER_API FWaveDefenseRun
{
public:
    bool Start(const UWaveDefenseDefinition& Definition, FGuid InRunId);
    void AdvanceTime(float GameSeconds);
    bool NotifySpawned(FGuid EnemyToken);
    bool NotifyEnemyDied(FGuid EnemyToken);
    void ResolveFrame(bool bPlayerDead, bool bCoreDestroyed);
    void Abort(EWaveDefenseOutcome Reason);
    bool TrySpendSupplies(int32 Amount);
    FWaveDefenseSnapshot GetSnapshot() const { return Snapshot; }
private:
    void StartWave();
    FGuid RunId;
    FWaveDefenseSnapshot Snapshot;
    TArray<int32> Counts;
    TArray<int32> Caps;
    TSet<FGuid> AliveTokens;
    TSet<FGuid> UsedTokens;
    float IntermissionSeconds = 0.0f;
    int32 WaveSupplies = 0;
};
