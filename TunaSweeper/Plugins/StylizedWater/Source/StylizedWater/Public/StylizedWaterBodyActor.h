#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StylizedWaterBodyActor.generated.h"
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture2D;

UENUM(BlueprintType)
enum class EStylizedWaterPreset : uint8 { CalmLake, GentleBeach, FlowingRiver };

// WATER_MASK_REBUILD: only the native type name is retained for level migration.
UCLASS(BlueprintType, ClassGroup=(Rendering), meta=(DisplayName="Stylized Water"))
class STYLIZEDWATER_API AStylizedWaterBodyActor : public AActor
{
    GENERATED_BODY()
public:
    AStylizedWaterBodyActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Water|Actions") void RebuildSurface();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Water|Actions") void FitSurfaceToTerrain();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Water|Presets") void ApplyCalmLakePreset();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Water|Presets") void ApplyGentleBeachPreset();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Water|Presets") void ApplyFlowingRiverPreset();
    void ApplyPreset(EStylizedWaterPreset Preset, bool bRebuild = true);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Water") TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Water") TObjectPtr<UProceduralMeshComponent> WaterSurface;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shape", meta=(ClampMin="100", Units="cm")) FVector2D SurfaceSize = FVector2D(5000,5000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shape", meta=(ClampMin="2", ClampMax="192")) FIntPoint GridResolution = FIntPoint(64,64);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shape", meta=(Units="cm")) float WaterLevelOffset = 0;

    // R = signed shore distance (0.5 is waterline); G = normalized water depth.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask") TObjectPtr<UTexture2D> BoundaryMask;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(ToolTip="Off: mask follows actor position and yaw. On: use World Mask Center and World Mask Yaw.")) bool bWorldLockedMask = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(EditCondition="bWorldLockedMask", Units="cm")) FVector2D WorldMaskCenter = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(EditCondition="bWorldLockedMask", Units="deg")) float WorldMaskYaw = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(ToolTip="World extent covered by the mask. Zero uses Surface Size times actor scale.", Units="cm")) FVector2D MaskWorldSize = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(ClampMin="0.01")) FVector2D MaskUVScale = FVector2D(1,1);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask") FVector2D MaskUVOffset = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(ClampMin="1", Units="cm", ToolTip="R=0/1 encodes minus/plus this shore distance; match the imported mask.")) float MaskDistanceRange = 1000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(ClampMin="1", Units="cm", ToolTip="Water depth represented by G=1 in the imported mask.")) float MaskDepthRange = 700;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(ClampMin="1", Units="cm")) float EdgeFeather = 35;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Mask", meta=(Units="cm")) float ShoreOffset = 0;
    UPROPERTY(VisibleInstanceOnly, Category="Water|Mask") FString MaskResolution;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Color") FLinearColor ShallowColor = FLinearColor(0.13,0.49,0.48);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Color") FLinearColor MidColor = FLinearColor(0.026,0.23,0.35);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Color") FLinearColor DeepColor = FLinearColor(0.012,0.073,0.18);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Color", meta=(ClampMin="0",ClampMax="1")) float Opacity = 0.86;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Color", meta=(ClampMin="1",Units="cm")) float DepthColorRange = 700;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Color", meta=(ClampMin="0",ClampMax="1",ToolTip="Blend imported G depth with the explicitly fitted terrain depth.")) float TerrainDepthInfluence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore", meta=(ClampMin="0",Units="cm")) float ShoreRunup = 14;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore", meta=(ClampMin="0")) float ShoreWaveSpeed = 0.09;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore", meta=(ClampMin="1",Units="cm")) float FoamWidth = 22;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore", meta=(ClampMin="20",Units="cm")) float ShoreWavelength = 180;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore", meta=(ClampMin="0",ClampMax="1")) float FoamIntensity = 0.6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore") FLinearColor FoamColor = FLinearColor(0.82,0.9,0.86);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore", meta=(ClampMin="0",ClampMax="0.4")) float ShoreFilmOpacity = 0.14;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Shore", meta=(ClampMin="0.1",Units="cm")) float IntersectionFade = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Flow") FVector2D FlowDirection = FVector2D(1,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Flow", meta=(ClampMin="0",Units="cm/s")) float FlowSpeed = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Flow", meta=(ClampMin="30",Units="cm")) float RippleScale = 230;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Flow", meta=(ClampMin="0",ClampMax="1")) float RippleStrength = 0.12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Flow", meta=(ClampMin="0",ClampMax="0.1")) float RefractionStrength = 0.015;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Flow", meta=(ClampMin="0",ClampMax="4")) float AnimationSpeed = 1;

    UPROPERTY(EditAnywhere, Category="Water|Terrain", meta=(ClampMin="10",Units="cm")) float TraceHeight = 1500;
    UPROPERTY(EditAnywhere, Category="Water|Terrain", meta=(ClampMin="10",Units="cm")) float MaximumDepth = 2000;
    UPROPERTY(EditAnywhere, Category="Water|Terrain", meta=(ClampMin="0.5",Units="cm")) float TerrainFilmLift = 3;
    UPROPERTY(EditAnywhere, Category="Water|Terrain") TEnumAsByte<ECollisionChannel> TerrainTraceChannel = ECC_Visibility;
    UPROPERTY(VisibleInstanceOnly, Category="Water|Terrain") FString LastTerrainFit;

    // WATER_SKY_PARALLAX_EXPERIMENT_BEGIN. The base material has no sky dependency.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Sky Reflection", meta=(DisplayName="Use Painted Sky Reflection")) bool bEnableSkyParallax = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Sky Reflection", meta=(EditCondition="bEnableSkyParallax")) TSoftObjectPtr<UTexture2D> SkyTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Sky Reflection", meta=(EditCondition="bEnableSkyParallax",ClampMin="0",ClampMax="1")) float SkyReflectionStrength = 0.68;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Sky Reflection", meta=(EditCondition="bEnableSkyParallax",ClampMin="100",Units="cm")) float SkyHeight = 6500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Sky Reflection", meta=(EditCondition="bEnableSkyParallax",ClampMin="100",Units="cm")) float SkyWorldSize = 28000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Sky Reflection", meta=(EditCondition="bEnableSkyParallax",Units="cm")) FVector2D SkyWorldAnchor = FVector2D::ZeroVector;
    // WATER_SKY_PARALLAX_EXPERIMENT_END
private:
    void BuildSurface(bool bTrace);
    void UpdateMaterial();
    UPROPERTY() TArray<float> FittedHeights;
    UPROPERTY() TArray<float> FittedDepths;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
    // Reflected soft references make the saved materials discoverable by the cooker.
    UPROPERTY() TSoftObjectPtr<UMaterialInterface> BaseMaterial;
    // WATER_SKY_PARALLAX_EXPERIMENT
    UPROPERTY() TSoftObjectPtr<UMaterialInterface> SkyMaterial;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
    virtual void PostEditMove(bool bFinished) override;
#endif
};
