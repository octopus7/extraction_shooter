#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveDefenseCore.generated.h"
class UTunaSweeperFactionComponent;
class UStaticMeshComponent;
class AWaveDefenseDirector;
struct TUNASWEEPER_API FWaveDefenseDamageRules
{
    static float Resolve(float RawDamage, int32 Multiplier, FGuid TargetRun, FGuid SourceRun, bool bHostile, bool bActiveParticipant);
};
UCLASS(Blueprintable)
class TUNASWEEPER_API AWaveDefenseCore : public AActor
{
    GENERATED_BODY()
public:
    AWaveDefenseCore();
    void ResetForRun(FGuid InRunId, int32 InMaxHealth);
    void SetDirector(AWaveDefenseDirector* InDirector) { Director = InDirector; }
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    int32 GetHealth() const { return Health; }
    bool Repair(int32 Amount) { if (Amount <= 0 || Health <= 0 || Health >= MaxHealth) return false; Health = FMath::Min(MaxHealth, Health + Amount); return true; }
    int32 GetMaxHealth() const { return MaxHealth; }
private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true")) TObjectPtr<UStaticMeshComponent> VisualMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTunaSweeperFactionComponent> FactionComponent;
    UPROPERTY(Transient) TObjectPtr<AWaveDefenseDirector> Director;
    FGuid RunId;
    int32 Health = 0;
    int32 MaxHealth = 0;
};
