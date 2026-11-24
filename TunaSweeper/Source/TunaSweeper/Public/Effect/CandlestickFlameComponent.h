#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CandlestickFlameComponent.generated.h"

class UMaterialInterface;
class UPointLightComponent;

/** Cosmetic flame centered on the tip of a candle wick. */
UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UCandlestickFlameComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UCandlestickFlameComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FSpark
	{
		FVector Origin;
		FVector Velocity;
		float StartTime;
		float Lifetime;
	};

	void EmitSparks(float FlameHeight);
	void UpdateVisuals();

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> CandleLight;

	TArray<FSpark> Sparks;
	FRandomStream Random;
	float BaseLightIntensity = 0.0f;
	float AnimationTime = 0.0f;
	float NextSparkBurstTime = 0.0f;
	float LastSparkBurstTime = -1.0f;
};
