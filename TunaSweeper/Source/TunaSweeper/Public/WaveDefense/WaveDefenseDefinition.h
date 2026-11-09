#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WaveDefense/WaveDefenseTypes.h"
#include "WaveDefenseDefinition.generated.h"
class ATunaSweeperEnemyCharacter;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FWaveDefenseEnemyEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<ATunaSweeperEnemyCharacter> EnemyClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CombatProfile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EWaveDefenseEnemyRole Role = EWaveDefenseEnemyRole::CoreAssault;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float BaseHealth = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WeaponId = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AmmoId = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ReserveAmmo = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Count = 0;
};
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FWaveDefenseWave
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AliveCap = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FWaveDefenseEnemyEntry> Enemies;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Lanes;
    int32 GetCount() const;
};
UCLASS(BlueprintType)
class TUNASWEEPER_API UWaveDefenseDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName EncounterId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FWaveDefenseWave> Waves;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RewardCoins = 300;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CoreMaxHealth = 1000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 InitialSupplies = 200;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WaveSupplies = 100;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float PreparationSeconds = 45.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float IntermissionSeconds = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float LeaveSeconds = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpawnIntervalSeconds = 3.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpawnRetrySeconds = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpawnFailureSeconds = 15.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float StuckSeconds = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float RepathSeconds = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BarrierCost = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TurretCost = 150;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CoreRepairCost = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FacilityRepairCost = 25;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ReloadCost = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BarrierHealth = 300;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TurretHealth = 250;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TurretAmmo = 60;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TurretLimit = 2;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CoreRepairAmount = 200;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TurretRange = 1800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TurretInterval = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TurretDamage = 10.0f;
    bool ValidateDefinition(TArray<FName>& OutErrors) const;
};
