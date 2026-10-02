#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaRocketConfig.h"
#include "TunaGuidedRocket.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class ATunaRocketEffect;

UENUM(BlueprintType)
enum class ETunaRocketDetonation : uint8 { Fuse, Impact, Manual };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTunaRocketDetonated, ETunaRocketDetonation, Reason, FVector, Location);

UCLASS(Blueprintable)
class TUNAGUIDEDROCKET_API ATunaGuidedRocket : public AActor
{
    GENERATED_BODY()
public:
    ATunaGuidedRocket();
    virtual void Tick(float DeltaSeconds) override;
    virtual FVector GetVelocity() const override;
    UFUNCTION(BlueprintCallable, Category="Rocket") void Detonate(ETunaRocketDetonation Reason = ETunaRocketDetonation::Manual);
    UFUNCTION(BlueprintPure, Category="Rocket") bool HasDetonated() const { return bDetonated; }
    UFUNCTION(BlueprintPure, Category="Rocket") bool HasLostGuidance() const { return bGuidanceLost; }
    UFUNCTION(BlueprintPure, Category="Rocket") float GetFlightAge() const { return Age; }
    // The configuration is snapshotted at launch; editing the DA affects subsequent rockets.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rocket", meta=(ExposeOnSpawn="true")) TObjectPtr<UTunaRocketConfig> Configuration;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rocket", meta=(ExposeOnSpawn="true")) TObjectPtr<AActor> TargetActor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rocket", meta=(ExposeOnSpawn="true")) FVector TargetOffset = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rocket") TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UStaticMeshComponent> RocketMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UStaticMeshComponent> ExhaustMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UNiagaraComponent> TrailNiagara;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") TSubclassOf<ATunaRocketEffect> SimpleEffectClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") bool bSimpleTrail = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") bool bSimpleExplosion = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") TObjectPtr<UNiagaraSystem> ExplosionNiagara;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Presentation") TObjectPtr<USoundBase> ExplosionSound;
    UPROPERTY(BlueprintAssignable, Category="Rocket") FTunaRocketDetonated OnDetonated;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Damage") TEnumAsByte<ECollisionChannel> DamageCoverChannel = ECC_Visibility;
    // Override in the host BP for faction/team rules. Zero DA damage is visual-only.
    UFUNCTION(BlueprintNativeEvent, Category="Damage") bool CanDamageActor(AActor* Candidate) const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void ApplyExplosionDamage();
    void SpawnSimpleEffect(bool bExplosion);
    FTunaRocketSettings Active;
    TWeakObjectPtr<AActor> ActiveTarget;
    FVector AimOffset = FVector::ZeroVector;
    FVector Direction = FVector::ForwardVector;
    float Age = 0, TrailCountdown = 0;
    bool bDetonated = false, bGuidanceLost = false;
};
