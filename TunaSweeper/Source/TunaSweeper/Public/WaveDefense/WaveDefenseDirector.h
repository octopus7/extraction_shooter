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
    int32 GetDamageMultiplier() const { return DamageMultiplier; }
    bool OwnsCore(const AActor* Actor) const;
    bool OwnsFacility(const AActor* Actor) const;
    AActor* GetPreferredTarget(const AActor* Enemy, FGuid EventRunId, FGuid EnemyToken, EWaveDefenseEnemyRole EnemyRole) const;
    bool TryFacilityAction(FName SlotId, EWaveDefenseAction Action);
    bool TryClaimReward();
    int32 GetDifficultyStage() const;
    int32 GetRewardCoins() const;
    void GetLivingEnemies(TArray<ATunaSweeperEnemyCharacter*>& OutEnemies) const;
    void RegisterOwnedActor(AActor* Actor);
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
    bool TrySpawnOne();
    bool FindSpawnTransform(const AWaveDefenseSpawnPoint* Point, UClass* EnemyClass, FTransform& OutTransform) const;
    void CleanupOwnedActors();
    bool UpdateEnemyProgress(float DeltaSeconds);
    FWaveDefenseRun Run;
    FGuid RunId;
    TWeakObjectPtr<APawn> PlayerPawn;
    TMap<FGuid, TWeakObjectPtr<ATunaSweeperEnemyCharacter>> Enemies;
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
};
