#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NearbyWaterReflection.generated.h"

class UMaterialInstanceDynamic;
class UMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/** Local planar reflection for a horizontal water mesh. The surface material samples NearbyReflection. */
UCLASS()
class TUNASWEEPER_API ANearbyWaterReflection : public AActor
{
    GENERATED_BODY()
public:
    ANearbyWaterReflection();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(EditInstanceOnly, Category="Reflection") TObjectPtr<AActor> WaterActor;
    UPROPERTY(EditAnywhere, Category="Reflection") FName SurfaceName = TEXT("WaterSurface");
    UPROPERTY(EditAnywhere, Category="Reflection", meta=(ClampMin="0", Units="cm")) float ShoreRange = 550.f;
    UPROPERTY(EditAnywhere, Category="Reflection", meta=(ClampMin="0", Units="cm")) float MaxHeightAboveWater = 450.f;
    UPROPERTY(EditAnywhere, Category="Reflection", meta=(ClampMin="1", Units="cm")) float MaxObjectHalfWidth = 1000.f;
    UPROPERTY(EditAnywhere, Category="Reflection", meta=(ClampMin="100", Units="cm")) float ViewRange = 4500.f;
    UPROPERTY(EditAnywhere, Category="Reflection", meta=(ClampMin="64", ClampMax="1024")) int32 Resolution = 640;
private:
    void RefreshObjects(const FBox& WaterBounds);
    UPROPERTY(VisibleAnywhere, Category="Reflection") TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY(Transient) TObjectPtr<UMeshComponent> Surface;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OriginalMaterial;
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> Target;
    float UntilRefresh = 0.f;
};
