#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveDefense/WaveDefenseTypes.h"
#include "WaveDefenseFacility.generated.h"
class AWaveDefenseDirector;
class ATunaSweeperProjectile;
class UWaveDefenseDefinition;
class UTunaSweeperFactionComponent;
class UStaticMeshComponent;
UCLASS(Blueprintable)
class TUNASWEEPER_API AWaveDefenseFacility : public AActor
{
    GENERATED_BODY()
public:
    AWaveDefenseFacility();
    void InitializeForRun(AWaveDefenseDirector* InDirector, FGuid InRunId, FName InSlotId, EWaveDefenseFacilityKind InKind);
    void Configure(const UWaveDefenseDefinition& Definition);
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    bool CanDamageTarget(const AActor* Target) const;
    bool Repair();
    bool Reload();
    bool CanRepair() const { return Health > 0 && Health < MaxHealth; }
    bool CanReload() const { return Kind == EWaveDefenseFacilityKind::Turret && Health > 0 && Ammo < MaxAmmo; }
    bool IsOperational() const { return Health > 0; }
    EWaveDefenseFacilityKind GetKind() const { return Kind; }
    FWaveDefenseFacilitySnapshot GetSnapshot() const;
protected:
    UPROPERTY(EditDefaultsOnly, Category="Wave Defense") TSoftClassPtr<ATunaSweeperProjectile> ProjectileClass;
private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true")) TObjectPtr<UStaticMeshComponent> VisualMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTunaSweeperFactionComponent> FactionComponent;
    UPROPERTY(Transient) TObjectPtr<AWaveDefenseDirector> Director;
    FGuid RunId;
    FName SlotId;
    EWaveDefenseFacilityKind Kind = EWaveDefenseFacilityKind::Barrier;
    int32 Health = 0, MaxHealth = 0, Ammo = 0, MaxAmmo = 0;
    float Range = 0.0f, Interval = 1.0f, Damage = 0.0f, Cooldown = 0.0f;
};
