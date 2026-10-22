#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "PixelWorldProfile.generated.h"

class ULandscapeLayerInfoObject;
class UMaterialInterface;
class UStaticMesh;

USTRUCT()
struct FPixelWorldVegetation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Vegetation") TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere, Category="Vegetation") TSubclassOf<AActor> ActorClass;
    UPROPERTY(EditAnywhere, Category="Vegetation", meta=(ClampMin="0")) float DensityPer100SquareMeters = 5.f;
    UPROPERTY(EditAnywhere, Category="Vegetation", meta=(ClampMin="0")) float MinimumSpacingMeters = 1.f;
    UPROPERTY(EditAnywhere, Category="Vegetation", meta=(ClampMin="0.01")) float MinimumScale = 0.8f;
    UPROPERTY(EditAnywhere, Category="Vegetation", meta=(ClampMin="0.01")) float MaximumScale = 1.2f;
    UPROPERTY(EditAnywhere, Category="Vegetation", meta=(ClampMin="0", ClampMax="90")) float MaximumSlopeDegrees = 40.f;
    UPROPERTY(EditAnywhere, Category="Vegetation") float HeightOffsetCm = 0.f;
    UPROPERTY(EditAnywhere, Category="Vegetation") bool bAlignToSurface = false;
    UPROPERTY(EditAnywhere, Category="Vegetation") bool bCollision = false;
};

USTRUCT()
struct FPixelWorldColorRule
{
    GENERATED_BODY()
    // Deliberately separate bytes: no sRGB color picker or FLinearColor conversion.
    UPROPERTY(EditAnywhere, Category="Rule") uint8 Red = 0;
    UPROPERTY(EditAnywhere, Category="Rule") uint8 Green = 255;
    UPROPERTY(EditAnywhere, Category="Rule") uint8 Blue = 0;
    UPROPERTY(EditAnywhere, Category="Rule") float HeightMeters = 0.f;
    UPROPERTY(EditAnywhere, Category="Rule") TObjectPtr<ULandscapeLayerInfoObject> LandscapeLayer;
    UPROPERTY(EditAnywhere, Category="Rule") TArray<FPixelWorldVegetation> Vegetation;
};

UCLASS()
class PIXELWORLDBUILDEREDITOR_API UPixelWorldProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    virtual bool IsEditorOnly() const override { return true; }
    UPROPERTY(EditAnywhere, Category="Input", meta=(FilePathFilter="png")) FFilePath ColorPng;
    UPROPERTY(EditAnywhere, Category="Input", meta=(FilePathFilter="png")) FFilePath HeightPng;
    UPROPERTY(EditAnywhere, Category="Input") float HeightmapMinimumMeters = 0.f;
    UPROPERTY(EditAnywhere, Category="Input") float HeightmapMaximumMeters = 20.f;
    UPROPERTY(EditAnywhere, Category="World", meta=(ClampMin="0.01")) float MetersPerPixel = 1.f;
    UPROPERTY(EditAnywhere, Category="World") FVector Origin = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="World") int32 Seed = 12345;
    UPROPERTY(EditAnywhere, Category="World", meta=(ClampMin="0", ClampMax="8")) int32 HeightSmoothingPasses = 0;
    UPROPERTY(EditAnywhere, Category="World") TObjectPtr<UMaterialInterface> LandscapeMaterial;
    UPROPERTY(EditAnywhere, Category="Rules") TArray<FPixelWorldColorRule> Rules = { FPixelWorldColorRule() };
};
