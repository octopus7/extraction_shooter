#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMeleeImpactBurstActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTunaSweeperVisionSubjectComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMeleeImpactBurstActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperMeleeImpactBurstActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Effect")
	void SetBurstColor(const FLinearColor& InBurstColor);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> BurstParticles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVisionSubjectComponent> VisionSubjectComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect")
	FLinearColor BurstColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);

private:
	void ApplyBurstColorToMaterials();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BurstDynamicMaterials;

	TArray<FVector> BurstTargetLocations;
	TArray<FVector> BurstBaseScales;
	float ElapsedLifetimeSeconds = 0.0f;
};
