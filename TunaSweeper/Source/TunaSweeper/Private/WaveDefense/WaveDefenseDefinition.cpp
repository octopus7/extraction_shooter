#include "WaveDefense/WaveDefenseDefinition.h"
#include "AI/TunaSweeperEnemyCharacter.h"
int32 FWaveDefenseWave::GetCount() const
{
    int64 Count = 0;
    for (const auto& Enemy : Enemies) Count += Enemy.Count;
    return Count > MAX_int32 || Count < 0 ? 0 : static_cast<int32>(Count);
}
bool UWaveDefenseDefinition::ValidateDefinition(TArray<FName>& OutErrors) const
{
    OutErrors.Reset();
    bool bValid = !EncounterId.IsNone() && Waves.Num() == 6 && RewardCoins > 0 && CoreMaxHealth > 0
        && InitialSupplies >= 0 && WaveSupplies >= 0 && int64(InitialSupplies) + int64(WaveSupplies) * 5 <= MAX_int32
        && FMath::IsFinite(PreparationSeconds) && PreparationSeconds > 0 && FMath::IsFinite(IntermissionSeconds) && IntermissionSeconds > 0
        && FMath::IsFinite(LeaveSeconds) && LeaveSeconds > 0 && FMath::IsFinite(SpawnIntervalSeconds) && SpawnIntervalSeconds > 0
        && FMath::IsFinite(SpawnRetrySeconds) && SpawnRetrySeconds > 0 && FMath::IsFinite(SpawnFailureSeconds) && SpawnFailureSeconds > 0
        && FMath::IsFinite(StuckSeconds) && StuckSeconds > 0 && FMath::IsFinite(RepathSeconds) && RepathSeconds > 0
        && BarrierCost > 0 && TurretCost > 0 && CoreRepairCost > 0 && FacilityRepairCost > 0 && ReloadCost > 0
        && BarrierHealth > 0 && TurretHealth > 0 && TurretAmmo > 0 && TurretLimit > 0 && CoreRepairAmount > 0
        && FMath::IsFinite(TurretRange) && TurretRange > 0 && FMath::IsFinite(TurretInterval) && TurretInterval > 0
        && FMath::IsFinite(TurretDamage) && TurretDamage > 0;
    for (const FWaveDefenseWave& Wave : Waves)
    {
        bValid &= Wave.AliveCap > 0 && Wave.GetCount() > 0;
        for (const FWaveDefenseEnemyEntry& Enemy : Wave.Enemies)
            bValid &= Enemy.Count > 0 && FMath::IsFinite(Enemy.BaseHealth) && Enemy.BaseHealth > 0 && !Enemy.CombatProfile.IsNone()
                && !Enemy.EnemyClass.IsNull() && Enemy.EnemyClass.LoadSynchronous() != nullptr
                && Enemy.ReserveAmmo >= 0 && (Enemy.Role != EWaveDefenseEnemyRole::PlayerPressure || (Enemy.WeaponId > 0 && Enemy.AmmoId > 0));
    }
    if (!bValid) OutErrors.Add(TEXT("ui.wave_defense.invalid_definition"));
    return bValid;
}
