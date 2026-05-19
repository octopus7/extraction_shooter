#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMeleeImpactBurstActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMeleeImpactBurstActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperMeleeImpactBurstActor();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> BurstParticles;

private:
	TArray<FVector> BurstTargetLocations;
	TArray<FVector> BurstBaseScales;
	float ElapsedLifetimeSeconds = 0.0f;
};
