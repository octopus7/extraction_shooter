#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperTransparentObstacleActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperTransparentObstacleActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperTransparentObstacleActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Obstacle")
	void ConfigureObstacleDefaults(FName InObstacleId, const FVector& InBoxExtent);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Obstacle")
	FName GetObstacleId() const { return ObstacleId; }

protected:
	void ApplyCollisionDefaults();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	FName ObstacleId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	FVector BoxExtent = FVector(260.0f, 45.0f, 140.0f);
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWorldProgressCompletedActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperWorldProgressCompletedActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

private:
	void ApplyRepairedBridgeVisualMesh();
};
