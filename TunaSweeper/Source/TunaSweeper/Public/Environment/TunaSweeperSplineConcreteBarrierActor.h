#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperSplineConcreteBarrierActor.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperSplineConcreteBarrierActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSplineConcreteBarrierActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spline Barrier")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Barrier")
	TObjectPtr<UStaticMesh> BarrierMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Barrier|Landscape Snap")
	bool bSnapToLandscape = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Barrier|Landscape Snap", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float LandscapeTraceDistance = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Barrier|Landscape Snap")
	TEnumAsByte<ECollisionChannel> LandscapeTraceChannel = ECC_Visibility;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SplineMeshes;

	void SnapSplinePointsToLandscape();
	void RebuildSplineMeshes();
};
