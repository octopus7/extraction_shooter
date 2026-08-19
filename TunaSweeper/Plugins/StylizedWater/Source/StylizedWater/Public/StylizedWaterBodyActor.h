#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StylizedWaterBodyActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EStylizedWaterPreset : uint8
{
	CalmLake UMETA(DisplayName = "Calm Lake"),
	GentleBeach UMETA(DisplayName = "Gentle Beach"),
	FlowingRiver UMETA(DisplayName = "Flowing River")
};

UCLASS(BlueprintType, Blueprintable, HideDropdown, ClassGroup = (Rendering), meta = (DisplayName = "Stylized Water Body (Internal)"))
class STYLIZEDWATER_API AStylizedWaterBodyActor : public AActor
{
	GENERATED_BODY()

public:
	AStylizedWaterBodyActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor, Category = "Stylized Water|Actions", meta = (DisplayName = "Rebuild And Bake Terrain Depth"))
	void RebuildAndBakeDepth();

	UFUNCTION(CallInEditor, Category = "Stylized Water|Actions", meta = (DisplayName = "Rebuild Without Terrain Trace"))
	void RebuildWithoutTerrainTrace();

	UFUNCTION(CallInEditor, Category = "Stylized Water|Presets", meta = (DisplayName = "Apply Calm Lake Preset"))
	void ApplyCalmLakePreset();

	UFUNCTION(CallInEditor, Category = "Stylized Water|Presets", meta = (DisplayName = "Apply Gentle Beach Preset"))
	void ApplyGentleBeachPreset();

	UFUNCTION(CallInEditor, Category = "Stylized Water|Presets", meta = (DisplayName = "Apply Flowing River Preset"))
	void ApplyFlowingRiverPreset();

	void ApplyPreset(EStylizedWaterPreset InPreset, bool bRebuild);
	void SetTemplateMaterialInstance(UMaterialInterface* InMaterial);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Water")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Water")
	TObjectPtr<UProceduralMeshComponent> WaterSurface;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Geometry", meta = (ClampMin = "100.0", UIMin = "100.0", Units = "cm"))
	FVector2D SurfaceSize = FVector2D(4000.0, 4000.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Geometry", meta = (ClampMin = "2", ClampMax = "256", UIMin = "4", UIMax = "128"))
	FIntPoint GridResolution = FIntPoint(48, 48);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Depth Bake")
	bool bSampleTerrainOnRebuild = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Depth Bake", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float TraceHeight = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Depth Bake", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float MaximumDryHeight = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Depth Bake", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float MaximumDepth = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Depth Bake")
	TEnumAsByte<ECollisionChannel> TerrainTraceChannel = ECC_Visibility;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stylized Water|Depth Bake", meta = (DisplayName = "Last Bake Result"))
	FString LastDepthBakeResult = TEXT("Not baked yet");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Color")
	FLinearColor ShallowColor = FLinearColor(0.16f, 0.72f, 0.76f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Color")
	FLinearColor MidColor = FLinearColor(0.035f, 0.43f, 0.63f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Color")
	FLinearColor DeepColor = FLinearColor(0.014f, 0.16f, 0.34f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Color")
	FLinearColor FoamColor = FLinearColor(0.94f, 0.96f, 0.88f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Color", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float DepthColorRange = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Color", meta = (ClampMin = "0.05", ClampMax = "0.95", UIMin = "0.05", UIMax = "0.95"))
	float MidColorPosition = 0.42f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Color", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "ImageGen Depth Gradient Influence"))
	float DepthGradientInfluence = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Opacity = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Roughness = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DistortionStrength = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.25"))
	float EmissiveStrength = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Waterline", meta = (Units = "cm"))
	float WaterLevelOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Waterline", meta = (ClampMin = "0.1", UIMin = "0.1", Units = "cm"))
	float WaterlineSoftness = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Shore Waves", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float ShoreRunup = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Shore Waves", meta = (ClampMin = "20.0", UIMin = "20.0", Units = "cm"))
	float ShoreWavelength = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Shore Waves", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ShoreWaveSpeed = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Shore Waves", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float ShoreFoamDepth = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Shore Waves", meta = (ClampMin = "0.01", ClampMax = "0.49", UIMin = "0.02", UIMax = "0.3"))
	float ShoreFoamWidth = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Shore Waves", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0"))
	float FoamIntensity = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Flow")
	FVector2D FlowDirection = FVector2D(1.0, 0.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Flow", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FlowSpeed = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Flow", meta = (ClampMin = "0.00001", UIMin = "0.00001"))
	float WaveWorldScale = 0.0035f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stylized Water|Flow", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float GeometryWaveAmplitude = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stylized Water|Internal", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> TemplateMaterialInstance;

protected:
	void BuildWaterMesh(bool bTraceTerrain);
	void EnsureDynamicMaterial();
	void UpdateMaterialParameters();
	float SampleSignedDepthAtWorldPosition(const FVector& SurfaceWorldPosition, bool& bOutHit) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
