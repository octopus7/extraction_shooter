#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveDefense/WaveDefenseRun.h"
#include "WaveDefenseDirector.generated.h"
class UWaveDefenseDefinition;
class AWaveDefenseCore;
class AWaveDefenseSpawnPoint;
class ATunaSweeperEnemyCharacter;
class AWaveDefenseFacility;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FWaveDefenseFacilitySlot
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName SlotId;
    UPROPERTY(EditAnywhere) EWaveDefenseFacilityKind Kind = EWaveDefenseFacilityKind::Barrier;
    UPROPERTY(EditAnywhere) FTransform LocalTransform;
};

UCLASS(Blueprintable)
class TUNASWEEPER_API AWaveDefenseDirector : public AActor
{
    GENERATED_BODY()
public:
    AWaveDefenseDirector();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    bool TryStart(APawn* Player);
    bool SetReady();
    void NotifyEnemyDied(FGuid EventRunId, FGuid EnemyToken);
    void Abort(EWaveDefenseOutcome Reason);
    FWaveDefenseSnapshot GetSnapshot() const;
    bool IsActiveParticipant(FGuid EventRunId, FGuid EnemyToken, const AActor* ExpectedEnemy = nullptr) const;
    bool IsDamageParticipant(FGuid EventRunId, FGuid EnemyToken, const AActor* ExpectedEnemy, bool bProjectile) const;
    bool IsOwnedProjectile(FGuid EventRunId, FGuid EnemyToken, const AActor* Projectile) const;
    int32 GetDamageMultiplier() const { return DamageMultiplier; }
    bool OwnsCore(const AActor* Actor) const;
    bool OwnsFacility(const AActor* Actor) const;
    AActor* GetPreferredTarget(const AActor* Enemy, FGuid EventRunId, FGuid EnemyToken, EWaveDefenseEnemyRole EnemyRole) const;
    bool TryFacilityAction(FName SlotId, EWaveDefenseAction Action);
    bool GetFacilitySnapshot(FName SlotId, FWaveDefenseFacilitySnapshot& OutSnapshot) const;
    bool TryClaimReward();
    int32 GetDifficultyStage() const;
    int32 GetRewardCoins() const;
    const UWaveDefenseDefinition* GetDefinition() const { return Definition; }
    const AWaveDefenseCore* GetCore() const { return Core; }
    const TArray<TObjectPtr<AWaveDefenseSpawnPoint>>& GetSpawnPoints() const { return SpawnPoints; }
    const TArray<FWaveDefenseFacilitySlot>& GetFacilitySlots() const { return FacilitySlots; }
    void GetLivingEnemies(TArray<ATunaSweeperEnemyCharacter*>& OutEnemies) const;
    void RegisterOwnedActor(AActor* Actor);
    void ConfigurePlacement(UWaveDefenseDefinition* InDefinition, AWaveDefenseCore* InCore,
        const TArray<AWaveDefenseSpawnPoint*>& InSpawnPoints, const TArray<FWaveDefenseFacilitySlot>& InSlots,
        TSubclassOf<AWaveDefenseFacility> InBarrierClass, TSubclassOf<AWaveDefenseFacility> InTurretClass);
protected:
    UPROPERTY(EditAnywhere, Category="Wave Defense") TObjectPtr<UWaveDefenseDefinition> Definition;
    UPROPERTY(EditInstanceOnly, Category="Wave Defense") TObjectPtr<AWaveDefenseCore> Core;
    UPROPERTY(EditInstanceOnly, Category="Wave Defense") TArray<TObjectPtr<AWaveDefenseSpawnPoint>> SpawnPoints;
    UPROPERTY(EditInstanceOnly, Category="Wave Defense") TArray<FWaveDefenseFacilitySlot> FacilitySlots;
    UPROPERTY(EditAnywhere, Category="Wave Defense") TSubclassOf<AWaveDefenseFacility> BarrierClass;
    UPROPERTY(EditAnywhere, Category="Wave Defense") TSubclassOf<AWaveDefenseFacility> TurretClass;
private:
    friend class FWaveDefenseRealCoreDamageTest;
    friend class FWaveDefenseFacilityTest;
    friend class FWaveDefensePersistenceTest;
    friend class FWaveDefenseHUDReuseTest;
    bool TrySpawnOne();
    bool FindSpawnTransform(const AWaveDefenseSpawnPoint* Point, UClass* EnemyClass, FTransform& OutTransform) const;
    void CleanupOwnedActors();
    bool UpdateEnemyProgress(float DeltaSeconds);
    void AnnounceWaveLanes(int32 WaveIndex);
    void PlayWarning() const;
    FWaveDefenseRun Run;
    FGuid RunId;
    TWeakObjectPtr<APawn> PlayerPawn;
    TMap<FGuid, TWeakObjectPtr<ATunaSweeperEnemyCharacter>> Enemies;
    TMap<FGuid, TWeakObjectPtr<ATunaSweeperEnemyCharacter>> Participants;
    struct FEnemyProgress { FVector LastLocation = FVector::ZeroVector; float IdleSeconds = 0; bool bRepathRequested = false; };
    TMap<FGuid, FEnemyProgress> EnemyProgress;
    int32 DamageMultiplier = 10000;
    TArray<TWeakObjectPtr<ATunaSweeperEnemyCharacter>> OwnedEnemies;
    TMap<FName, TWeakObjectPtr<AWaveDefenseFacility>> Facilities;
    TArray<TWeakObjectPtr<AActor>> OwnedActors;
    float SpawnDelay = 0.0f;
    float SpawnBlockedSeconds = 0.0f;
    float OutsideSeconds = 0.0f;
    int32 LastCompletedWave = 0;
    bool bCleaningUp = false;
    bool bTerminalCleanupDone = false;
    uint64 SuccessCandidateFrame = 0;
    TArray<FName> ActiveLanes;
    TArray<FName> AnnouncedLanes;
    float DirectionWarningSeconds = 0.0f;
    int32 LaneCursor = 0;
    int32 LaneRotationIndex = 0;
    int32 BatchesSinceRotation = 0;
    bool bCoreLowWarningSent = false;
    bool bOutsideWarningSent = false;
};
