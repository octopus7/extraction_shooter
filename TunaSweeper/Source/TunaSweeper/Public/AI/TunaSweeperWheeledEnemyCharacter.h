#pragma once

#include "AI/TunaSweeperEnemyCharacter.h"
#include "TunaSweeperWheeledEnemyCharacter.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/** Delivery robot and rabbit trike visuals driven by the existing character AI/movement. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWheeledEnemyCharacter : public ATunaSweeperEnemyCharacter
{
    GENERATED_BODY()
public:
    ATunaSweeperWheeledEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    virtual ETunaSweeperEnemyFireResult TryFireProjectileAt(AActor* TargetActor) override;
    bool IsDriving() const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Visual")
    bool bTricycle = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Visual")
    TSoftObjectPtr<UStaticMesh> RobotBodyMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Visual")
    TSoftObjectPtr<UStaticMesh> WheelMesh;
    /** Only slot 1 (Eyes) is overridden; body atlas remains nonemissive. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Visual")
    TSoftObjectPtr<UMaterialInterface> EyeMaterial;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> RobotBody;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> LeftWheel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> RightWheel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> FrontWheel;
protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnDeathPresentationStarted() override;
private:
    void ApplyRobotPresentation();
    void ConfigureDefaultCombat();
    void AlignBuiltInMuzzle();
    FVector PreviousRobotLocation = FVector::ZeroVector;
    FRotator PreviousRobotRotation = FRotator::ZeroRotator;
    double LeftRollRadians = 0;
    double RightRollRadians = 0;
    double FrontRollRadians = 0;
    float FrontSteeringDegrees = 0;
};
