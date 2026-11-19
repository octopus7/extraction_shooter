#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"

#include "ForestScatterRegion.generated.h"

class ADecalActor;
class ULandscapeLayerInfoObject;
class UMaterialInterface;
class UPCGComponent;
class UPCGGraph;
class USplineComponent;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FORESTSCATTER_API FForestScatterLayer
{
	GENERATED_BODY()

	/** Clear the mesh to omit this layer from the region. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Average number of instances in 100 square metres of eligible horizontal surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0", DisplayName = "Instances per 100 Square Metres"))
	float InstancesPer100SquareMeters = 0.0f;
};

/** Place or draw a closed planting outline over a green landscape map projection. */
UCLASS(Blueprintable, ClassGroup = (Procedural))
class FORESTSCATTER_API AForestScatterRegion : public AActor
{
	GENERATED_BODY()

public:
	AForestScatterRegion();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Forest Scatter")
	TObjectPtr<USplineComponent> Boundary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Forest Scatter")
	TObjectPtr<UPCGComponent> PCGComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Vegetation")
	FForestScatterLayer Shrubs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Vegetation")
	FForestScatterLayer Flowers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Vegetation")
	FForestScatterLayer Grass;

#if WITH_EDITORONLY_DATA
	/** Optional Landscape paint layers that support grass, shrubs and flowers when regenerating this region. */
	UPROPERTY(EditInstanceOnly, Category = "Forest Scatter|Vegetation")
	TArray<TObjectPtr<ULandscapeLayerInfoObject>> VegetationPaintLayers;

	/** Minimum combined weight of the selected paint layers at a planting point. */
	UPROPERTY(EditInstanceOnly, Category = "Forest Scatter|Vegetation", meta = (ClampMin = "0", ClampMax = "1"))
	float MinimumVegetationPaintWeight = 0.30f;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Stones")
	FForestScatterLayer Gravel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Stones")
	FForestScatterLayer Boulders;

	/** Materials for slot 1 of the flower mesh. An empty palette uses the mesh's original petals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Flowers")
	TArray<TSoftObjectPtr<UMaterialInterface>> FlowerPetalMaterials;

	/** Approximate diameter of a group of flowers sharing the same petal material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Flowers", meta = (ClampMin = "1", Units = "cm"))
	float FlowerColorPatchSize = 600.0f;

	/** PCG seed offsets start at one. Retired slots are null to keep the remaining layers in place. */
	TStaticArray<const FForestScatterLayer*, 6> GetGroundCoverLayers() const;

	/** Leave empty to discover loaded map guide decals automatically. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Forest Scatter|Map Guide")
	TArray<TObjectPtr<ADecalActor>> ProjectionDecals;

	/** Optional buildings. Ground contact uses their static mesh collision unless ForestScatterFootprint primitives mark the foundation. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Forest Scatter|Buildings")
	TArray<TObjectPtr<AActor>> GroundedBuildings;

	/** Vertical tolerance around the landscape surface when testing a building foundation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Scatter|Buildings", meta = (ClampMin = "0", Units = "cm"))
	float BuildingGroundTolerance = 10.0f;

	/** Rebuild the saved instances in the level editor from the current outline, map guide and buildings. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Forest Scatter", meta = (DevelopmentOnly))
	void RegenerateForest();

protected:
	virtual void PostRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void AssignForestGraph();

#if WITH_EDITOR
	bool TryRegenerateInEditor(float DeltaTime);
	FTSTicker::FDelegateHandle QueuedRegeneration;
#endif

	UPROPERTY()
	TSoftObjectPtr<UPCGGraph> DefaultForestGraph;
};
