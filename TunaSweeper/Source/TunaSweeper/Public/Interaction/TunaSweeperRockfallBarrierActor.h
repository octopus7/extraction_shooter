#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperRockfallBarrierActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UTextRenderComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTunaSweeperInteractableComponent;

/** Item-armed, persistent rockfall. Cliff components remain after the central obstruction clears. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRockfallBarrierActor : public AActor
{
    GENERATED_BODY()
public:
    ATunaSweeperRockfallBarrierActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable, Category="Rockfall") bool ArmExplosive(bool bSaveImmediately = true);
    UFUNCTION(BlueprintCallable, Category="Rockfall") void RefreshSavedState();
    UFUNCTION(BlueprintPure, Category="Rockfall") bool IsCleared() const { return bCleared; }
    UFUNCTION(BlueprintPure, Category="Rockfall") bool IsArmed() const { return bArmed; }
    UFUNCTION(BlueprintPure, Category="Rockfall") float GetCountdownRemaining() const;
    UFUNCTION(BlueprintPure, Category="Rockfall") float GetExplosionDamageAtLocation(FVector Location) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall") FName ProgressObjectId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall", meta=(ClampMin="1")) int32 RequiredItemId = 6031;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall", meta=(ClampMin="0.1")) float CountdownSeconds = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall") FVector ExplosionOffset = FVector(0,0,140);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall", meta=(ClampMin="1")) float ExplosionRadiusCm = 400.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall", meta=(ClampMin="0")) float ExplosionDamage = 60.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall", meta=(ClampMin="0")) float EdgeExplosionDamage = 15.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall") TSoftObjectPtr<UMaterialInterface> WarningMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rockfall|Preview") bool bPreviewCleared = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UStaticMeshComponent> CentralRocks;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UStaticMeshComponent> LeftCliff;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UStaticMeshComponent> RightCliff;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UStaticMeshComponent> ExplosiveBottle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UBoxComponent> BlockingCollision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UTextRenderComponent> CountdownText;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UTunaSweeperInteractableComponent> Interactable;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UDecalComponent> WarningDecal;
private:
    friend class FTunaSweeperRockfallTest;
    void ApplyPresentation();
    void StartCountdown();
    void Detonate();
    void ApplyExplosionDamage();
    void RefreshWarning();
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WarningMID;
    double CountdownStartedAt = 0;
    FTimerHandle DetonationTimer;
    bool bArmed = false;
    bool bCleared = false;
    bool bMutating = false;
    bool bSaveChanges = true;
    bool bSaveFailed = false;
};
