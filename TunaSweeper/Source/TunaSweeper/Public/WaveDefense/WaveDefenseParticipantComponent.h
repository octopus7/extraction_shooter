#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WaveDefense/WaveDefenseTypes.h"
#include "WaveDefenseParticipantComponent.generated.h"
class AWaveDefenseDirector;

UCLASS(ClassGroup=(WaveDefense), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UWaveDefenseParticipantComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    void Initialize(AWaveDefenseDirector* InDirector, FGuid InRunId, FGuid InToken, EWaveDefenseEnemyRole InRole);
    AActor* GetPreferredTarget() const;
    bool ShouldGrantDefaultKillRewards() const { return !RunId.IsValid(); }
    void NotifyDeath();
    void NotifyDamageFrom(AActor* DamageSource);
    bool WasRecentlyHitBy(const AActor* DamageSource) const;
    FGuid GetRunId() const { return RunId; }
    FGuid GetToken() const { return Token; }
    EWaveDefenseEnemyRole GetRole() const { return Role; }
    AWaveDefenseDirector* GetDirector() const { return Director.Get(); }
private:
    TWeakObjectPtr<AWaveDefenseDirector> Director;
    FGuid RunId;
    FGuid Token;
    EWaveDefenseEnemyRole Role = EWaveDefenseEnemyRole::CoreAssault;
    bool bDeathReported = false;
    TWeakObjectPtr<AActor> LastDamageSource;
    double LastDamageTime = -1000.0;
};
