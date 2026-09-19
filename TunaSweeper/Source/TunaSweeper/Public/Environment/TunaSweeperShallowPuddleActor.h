#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperShallowPuddleActor.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FTunaSweeperPuddleFootstep,
	FVector, SurfaceLocation, float, SpeedCmPerSecond, bool, bSprinting, AActor*, StepInstigator);

/** Independent shallow water surface. Position the actor at the water level, above solid ground. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperShallowPuddleActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperShallowPuddleActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Half size of the supporting rectangle; the irregular outline sits inside it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Shape", meta = (ClampMin = "1.0", Units = "cm"))
	FVector2D HalfExtentCm = FVector2D(150, 100);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OutlineIrregularity = 0.8f;

	/** Maximum distance from the water level down to ground for footsteps and wet-edge projection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Shape", meta = (ClampMin = "0.1", ClampMax = "50.0", Units = "cm"))
	float MaxWaterDepthCm = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Water")
	TObjectPtr<UMaterialInterface> WaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Water", meta = (ClampMin = "0.02", ClampMax = "1.0"))
	float WaterRoughness = 0.09f;

	/** Absorption coefficients in inverse centimetres, not an opaque surface tint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Water")
	FLinearColor Absorption = FLinearColor(0.025f, 0.012f, 0.008f, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Water")
	FLinearColor Scattering = FLinearColor(0.0015f, 0.002f, 0.0025f, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Water", meta = (ClampMin = "0.0", ClampMax = "0.15"))
	float RippleStrength = 0.018f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Water", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float RippleSpeed = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Wet Edge")
	TObjectPtr<UMaterialInterface> WetEdgeMaterial;

	/** Edge width relative to HalfExtentCm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Wet Edge", meta = (ClampMin = "0.01", ClampMax = "0.4"))
	float WetEdgeWidth = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Wet Edge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Wetness = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Wet Edge")
	FLinearColor WetGroundColor = FLinearColor(0.055f, 0.045f, 0.035f, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle|Footstep")
	TObjectPtr<USoundBase> WaterFootstepSound;

	/** Niagara authoring is deferred. Bind a future effect to this event in BP_ShallowPuddle. */
	UPROPERTY(BlueprintAssignable, Category = "Puddle|Footstep")
	FTunaSweeperPuddleFootstep OnPuddleFootstep;

	/** Call after changing parameters or moving/scaling the puddle at runtime. Editor construction calls it automatically. */
	UFUNCTION(BlueprintCallable, Category = "Puddle")
	void RefreshPuddle();

	UFUNCTION(BlueprintPure, Category = "Puddle")
	bool ContainsGroundPoint(FVector GroundPoint) const;

	UFUNCTION(BlueprintCallable, Category = "Puddle|Footstep")
	void NotifyFootstep(FVector GroundPoint, float SpeedCmPerSecond, bool bSprinting, AActor* StepInstigator);

	static ATunaSweeperShallowPuddleActor* FindPuddleAtGroundPoint(UWorld* World, const FVector& GroundPoint);
	UStaticMeshComponent* GetSurface() const { return Surface; }

private:
	bool IsWaterSurfaceActive() const;
	FVector2D GetWorldHalfExtent() const;
	void ApplySharedParameters(UMaterialInstanceDynamic* Material) const;

	UPROPERTY(VisibleAnywhere, Category = "Puddle")
	TObjectPtr<UStaticMeshComponent> Surface;

	UPROPERTY(VisibleAnywhere, Category = "Puddle")
	TObjectPtr<UDecalComponent> WetEdge;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WetEdgeInstance;
};
