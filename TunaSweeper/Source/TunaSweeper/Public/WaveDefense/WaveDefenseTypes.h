#pragma once
#include "CoreMinimal.h"
#include "WaveDefenseTypes.generated.h"

UENUM(BlueprintType)
enum class EWaveDefensePhase : uint8 { Idle, Preparing, Active, Intermission, Resolving, Finished };
UENUM(BlueprintType)
enum class EWaveDefenseOutcome : uint8 { None, Succeeded, CoreDestroyed, PlayerDied, Abandoned, Interrupted, Faulted };
UENUM(BlueprintType)
enum class EWaveDefenseEnemyRole : uint8 { CoreAssault, PlayerPressure, EliteAssault };
UENUM(BlueprintType)
enum class EWaveDefenseFacilityKind : uint8 { Barrier, Turret };
UENUM(BlueprintType)
enum class EWaveDefenseAction : uint8 { InstallBarrier, InstallTurret, RepairCore, RepairFacility, ReloadTurret };

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FWaveDefenseProgress
{
    GENERATED_BODY()
    UPROPERTY() FGuid RunId;
    UPROPERTY() int32 DifficultyStage = 0;
    UPROPERTY() EWaveDefenseOutcome Outcome = EWaveDefenseOutcome::None;
    UPROPERTY() int32 LastCompletedWave = 0;
    UPROPERTY() int32 RewardCoins = 0;
    UPROPERTY() bool bRewardPending = false;
    UPROPERTY() int32 BestCompletedWave = 0;
    UPROPERTY() int32 CompletedRuns = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FWaveDefenseSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EWaveDefensePhase Phase = EWaveDefensePhase::Idle;
    UPROPERTY(BlueprintReadOnly) EWaveDefenseOutcome Outcome = EWaveDefenseOutcome::None;
    UPROPERTY(BlueprintReadOnly) int32 Wave = 0;
    UPROPERTY(BlueprintReadOnly) int32 TotalWaves = 0;
    UPROPERTY(BlueprintReadOnly) int32 Alive = 0;
    UPROPERTY(BlueprintReadOnly) int32 Queued = 0;
    UPROPERTY(BlueprintReadOnly) int32 CoreHealth = 0;
    UPROPERTY(BlueprintReadOnly) int32 CoreMaxHealth = 0;
    UPROPERTY(BlueprintReadOnly) int32 Supplies = 0;
    UPROPERTY(BlueprintReadOnly) float SecondsRemaining = 0.0f;
    UPROPERTY(BlueprintReadOnly) float LeaveSecondsRemaining = 0.0f;
    UPROPERTY(BlueprintReadOnly) TArray<FName> AnnouncedLanes;
    UPROPERTY(BlueprintReadOnly) bool bResultSavePending = false;
    UPROPERTY(BlueprintReadOnly) bool bRewardPending = false;
    UPROPERTY(BlueprintReadOnly) float DirectionWarningSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FWaveDefenseFacilitySnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bOccupied = false;
    UPROPERTY(BlueprintReadOnly) int32 Health = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxHealth = 0;
    UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxAmmo = 0;
};
