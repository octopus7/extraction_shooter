#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RegionalGroundFogActor.generated.h"

class ULocalFogVolumeComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** One softly fading circular part of a local ground-fog region. Distances are Unreal centimeters. */
USTRUCT(BlueprintType)
struct FRegionalGroundFogNode
{
	GENERATED_BODY()

	/** Center relative to the region actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Node")
	FVector LocalCenter = FVector::ZeroVector;

	/** Outer edge at which the visual card layer reaches zero density. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Node", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float OuterRadius = 1600.0f;

	/** Full-density radius used by the drifting-card layer. Must not exceed Outer Radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Node", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CoreRadius = 1000.0f;

	/** Multiplier for this node's Local Fog Volume density. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog Node", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float DensityMultiplier = 1.0f;
};

/** Editor-only visualization anchor. The editor module draws the region node radii for this component. */
UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent), hidecategories = (Activation, Collision, Cooking, Navigation, Physics, Rendering))
class REGIONALGROUNDFOG_API URegionalGroundFogVisualizationComponent : public USceneComponent
{
	GENERATED_BODY()
};

/**
 * Placeable, self-contained local ground-fog region. It owns all rendered fog volumes and never
 * locates or mutates any pre-existing fog actor in the world.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = Fog, meta = (DisplayName = "Regional Ground Fog"))
class REGIONALGROUNDFOG_API ARegionalGroundFogActor : public AActor
{
	GENERATED_BODY()

public:
	ARegionalGroundFogActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Adds a node next to the last node. Intended for Details-panel use. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Regional Ground Fog")
	void AddFogNode();

	/** Removes the last node while preserving at least one node. Intended for Details-panel use. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Regional Ground Fog")
	void RemoveLastFogNode();

	/** Reapplies the shared rendering settings to all owned fog-volume nodes. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Regional Ground Fog")
	void RefreshFogRegion();

	const TArray<FRegionalGroundFogNode>& GetFogNodes() const { return FogNodes; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Regional Ground Fog")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Regional Ground Fog")
	TObjectPtr<URegionalGroundFogVisualizationComponent> Visualization;

	/** Up to eight nodes are rendered per actor to keep tiled local-fog complexity predictable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regional Ground Fog", meta = (TitleProperty = "OuterRadius"))
	TArray<FRegionalGroundFogNode> FogNodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Local Fog", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float RadialFogDensity = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Local Fog", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float HeightFogDensity = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Local Fog", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "5000.0"))
	float HeightFogFalloff = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Local Fog", meta = (UIMin = "-2.0", UIMax = "2.0"))
	float HeightFogOffset = -0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Local Fog")
	FLinearColor FogAlbedo = FLinearColor(0.78f, 0.86f, 0.94f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Local Fog", meta = (ClampMin = "0.0", ClampMax = "0.999", UIMin = "0.0", UIMax = "0.999"))
	float ScatteringDistribution = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Local Fog", meta = (ClampMin = "-127", ClampMax = "127"))
	int32 FogSortPriority = 0;

	/** Enables translucent, gently drifting cards in addition to the actual Local Fog Volumes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards")
	bool bEnableDriftingCards = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards", meta = (ClampMin = "0", ClampMax = "48", UIMin = "0", UIMax = "48"))
	int32 DriftingCardCount = 18;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float CardDiameter = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CardOpacity = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CardHeight = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CardHeightVariance = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DriftSpeed = 13.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurlAmplitude = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards")
	FLinearColor CardColor = FLinearColor(0.75f, 0.84f, 0.93f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drifting Cards")
	int32 RandomSeed = 217;

	UPROPERTY(EditDefaultsOnly, Category = "Drifting Cards")
	TSoftObjectPtr<UMaterialInterface> CardMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CardPlaneMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULocalFogVolumeComponent>> FogVolumeComponents;

	void SynchronizeFogVolumeComponents();
	void ClearDriftingCards();
	void CreateDriftingCards();
	void RespawnCard(int32 CardIndex, bool bRandomizeNode);
	int32 GetActiveNodeCount() const;
	void NormalizeFogNodes();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	struct FRuntimeDriftingCard
	{
		TObjectPtr<UStaticMeshComponent> Component;
		TObjectPtr<UMaterialInstanceDynamic> Material;
		int32 NodeIndex = INDEX_NONE;
		FVector2D Direction = FVector2D::UnitX();
		float Phase = 0.0f;
		float SizeMultiplier = 1.0f;
	};

	TArray<FRuntimeDriftingCard> RuntimeCards;
	FRandomStream RandomStream;
};
