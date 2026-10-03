#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HopperArmModule.generated.h"

class ATunaSweeperProjectile;
class UHopperVisualData;
class USceneComponent;
class UStaticMeshComponent;

/** Replaceable arm. Ownership, faction and cooldown checks surround the extensible firing event. */
UCLASS(Blueprintable)
class TUNASWEEPER_API AHopperArmModule : public AActor
{
    GENERATED_BODY()
public:
    AHopperArmModule();
    virtual void Tick(float DeltaSeconds) override;
    void ConfigureVisual(UHopperVisualData* Data, bool bLeft);
    bool TryFireAt(AActor* TargetActor);
    bool IsReady() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hopper|Weapon")
    TSubclassOf<ATunaSweeperProjectile> ProjectileClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hopper|Weapon", meta=(ClampMin="0"))
    float Damage = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hopper|Weapon", meta=(ClampMin="0"))
    float CooldownSeconds = .75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hopper|Weapon")
    bool bShowCannonNozzle = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hopper|Visual")
    bool bUseHopperArmVisuals = true;

protected:
    /** Return true only when the module actually delivered its attack. */
    UFUNCTION(BlueprintNativeEvent, Category="Hopper|Weapon")
    bool FireWeapon(AActor* TargetActor, const FVector& AimPoint);
    virtual bool FireWeapon_Implementation(AActor* TargetActor, const FVector& AimPoint);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hopper|Weapon")
    TObjectPtr<USceneComponent> Muzzle;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> VisualParts;
    UPROPERTY(Transient) TObjectPtr<UHopperVisualData> VisualData;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> CannonNozzle;
    double NextFireTime = 0;
    double RestorePoseTime = 0;
};
