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
        && InitialSupplies >= 0 && WaveSupplies >= 0 && PreparationSeconds > 0 && IntermissionSeconds > 0;
    for (const FWaveDefenseWave& Wave : Waves)
    {
        bValid &= Wave.AliveCap > 0 && Wave.GetCount() > 0;
        for (const FWaveDefenseEnemyEntry& Enemy : Wave.Enemies)
            bValid &= Enemy.Count > 0 && Enemy.BaseHealth > 0 && !Enemy.CombatProfile.IsNone()
                && !Enemy.EnemyClass.IsNull() && Enemy.EnemyClass.LoadSynchronous() != nullptr
                && Enemy.ReserveAmmo >= 0 && (Enemy.Role != EWaveDefenseEnemyRole::PlayerPressure || (Enemy.WeaponId > 0 && Enemy.AmmoId > 0));
    }
    if (!bValid) OutErrors.Add(TEXT("ui.wave_defense.invalid_definition"));
    return bValid;
}
