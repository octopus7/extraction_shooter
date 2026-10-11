#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperBombObstacleActor.generated.h"
class UStaticMeshComponent;
class UBoxComponent;
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperBombObstacleActor : public AActor
{
    GENERATED_BODY()
public:
    ATunaSweeperBombObstacleActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer) override;
    UFUNCTION(BlueprintCallable) void RefreshObstacle();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle") FName ProgressObjectId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Obstacle") TObjectPtr<UStaticMeshComponent> ObstacleMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Obstacle") TObjectPtr<UBoxComponent> BlockingVolume;
private:
    void UpdateBlockingVolume();
};
