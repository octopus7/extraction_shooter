#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "HopperEnemyCharacter.generated.h"

class AHopperArmModule;
class UHopperPresentationComponent;
class UHopperVisualData;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EHopperCombatPhase : uint8
{
    Boarding, Mech, Disembarking, PilotRanged, PilotMelee, Dead
};

/** One AI and one final death/loot event, with a separately destructible mech. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperHopperEnemyCharacter : public ATunaSweeperEnemyCharacter
{
    GENERATED_BODY()
public:
    ATunaSweeperHopperEnemyCharacter();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    virtual ETunaSweeperEnemyFireResult TryFireProjectileAt(AActor* TargetActor) override;
    virtual bool StartEnemyReload() override { return false; }
    virtual FTunaSweeperEnemyWeaponRuntimeStatus GetEnemyWeaponRuntimeStatus() override;
    virtual bool AttackTarget(AActor* TargetActor) override;
    virtual bool UsesMeleeAttack() const override;
    virtual bool IsStandardCombatSuppressed() const override;

    UFUNCTION(BlueprintPure, Category="Hopper")
    EHopperCombatPhase GetCombatPhase() const { return IsDead() ? EHopperCombatPhase::Dead : Phase; }
    UFUNCTION(BlueprintPure, Category="Hopper")
    float GetMechHealth() const { return MechHealth; }
    UFUNCTION(BlueprintPure, Category="Hopper")
    int32 GetPilotAmmo() const { return PilotAmmo; }
    UFUNCTION(BlueprintCallable, Category="Hopper|Arms")
    bool SetArmModule(bool bLeft, TSubclassOf<AHopperArmModule> ModuleClass);
    UFUNCTION(BlueprintPure, Category="Hopper|Arms")
    AHopperArmModule* GetArmModule(bool bLeft) const;
    UHopperPresentationComponent* GetPresentation() const { return Presentation; }
    bool CanAttackHopperTarget(AActor* TargetActor) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Visual")
    TSoftObjectPtr<UHopperVisualData> VisualData;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Visual")
    TSoftObjectPtr<UNiagaraSystem> MechDestructionEffect;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Combat", meta=(ClampMin="1"))
    float MechMaxHealth = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Combat", meta=(ClampMin="0"))
    int32 PilotAmmoCapacity = 24;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Combat", meta=(ClampMin="0"))
    float PilotShotCooldown = .25f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Phases", meta=(ClampMin="0"))
    float BoardingSeconds = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Phases", meta=(ClampMin="0"))
    float DisembarkSeconds = 1.4f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Phases")
    bool bStartMounted = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Movement", meta=(ClampMin="0"))
    float MechMoveSpeed = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Movement", meta=(ClampMin="0"))
    float PilotMoveSpeed = 160.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Arms")
    TSubclassOf<AHopperArmModule> LeftArmClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hopper|Arms")
    TSubclassOf<AHopperArmModule> RightArmClass;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;
    virtual void OnDeathPresentationStarted() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hopper")
    TObjectPtr<UHopperPresentationComponent> Presentation;
private:
    friend class FHopperCombatOwnershipTest;
    friend class FHopperTransitionClearanceTest;
    void InitializePresentation();
    void BeginMechDestruction();
    void UpdateDisembark(float DeltaSeconds);
    bool FindPilotExit(FVector& OutGroundDestination) const;
    bool IsPilotExitClear(const FVector& GroundDestination) const;
    bool IsPilotTransferClear(const FVector& StartFeet, const FVector& GroundDestination, float StartAlpha = 0.f) const;
    void EnterPilotCombat();
    bool FirePilotProjectile(AActor* TargetActor);
    void UpdatePilotAmmoPhase();
    void ApplyPhaseMovementSpeed();
    UPROPERTY(Transient) TObjectPtr<UHopperVisualData> LoadedVisualData;
    UPROPERTY(Transient) TObjectPtr<AHopperArmModule> LeftArm;
    UPROPERTY(Transient) TObjectPtr<AHopperArmModule> RightArm;
    EHopperCombatPhase Phase = EHopperCombatPhase::Boarding;
    float MechHealth = 180.f;
    int32 PilotAmmo = 24;
    float PhaseSeconds = 0.f;
    float ExitRetrySeconds = 0.f;
    bool bExitInProgress = false;
    bool bNextArmLeft = true;
    bool bInitializingBase = true;
    FVector ExitGroundDestination = FVector::ZeroVector;
    FVector ExitStartFeet = FVector::ZeroVector;
    float PreviousGaitYaw = 0.f;
    float TurningFootSpeed = 0.f;
    double NextPilotShotTime = 0;
    double NextPilotMeleeTime = 0;
    static constexpr float PilotRadius = 16.f;
    static constexpr float PilotHalfHeight = 37.f;
};
