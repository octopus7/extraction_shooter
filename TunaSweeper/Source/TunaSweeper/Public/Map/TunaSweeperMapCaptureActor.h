#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMapCaptureActor.generated.h"

class UBoxComponent;
class USceneComponent;
class USceneCaptureComponent2D;
struct FPropertyChangedEvent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMapCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperMapCaptureActor();

	virtual bool IsEditorOnly() const override { return true; }
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(CallInEditor, Category = "TunaSweeper|Map Capture")
	void AutoDetectCaptureBounds();

	UFUNCTION(CallInEditor, Category = "TunaSweeper|Map Capture")
	void CaptureOpaqueRgbPng();

	UFUNCTION(CallInEditor, Category = "TunaSweeper|Map Capture")
	void AutoDetectBoundsAndCaptureOpaqueRgbPng();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Map Capture")
	FVector2D WorldLocationToMapUV(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Map Capture")
	FVector MapUVToWorldLocation(const FVector2D& MapUV, float WorldZ = 0.0f) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoundsPreviewComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Bounds", meta = (ClampMin = "100.0", UIMin = "100.0"))
	FVector2D CaptureWorldSize = FVector2D(6000.0f, 6000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Bounds", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BoundsPaddingCm = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect", meta = (ClampMin = "25.0", UIMin = "25.0"))
	float AutoDetectGridStepCm = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect", meta = (ClampMin = "100.0", UIMin = "100.0"))
	FVector2D AutoDetectSearchExtent = FVector2D(12000.0f, 12000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect")
	float AutoDetectTraceStartHeight = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float AutoDetectTraceDepth = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect")
	TEnumAsByte<ECollisionChannel> AutoDetectTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect")
	bool bIgnoreMovableComponentsForBounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect")
	bool bRequireIncludedActorTag = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Auto Detect", meta = (EditCondition = "bRequireIncludedActorTag"))
	TArray<FName> IncludedActorTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Output", meta = (ClampMin = "256", UIMin = "256", UIMax = "4096"))
	int32 LongSideResolution = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Output")
	bool bAutoDetectBoundsBeforeCapture = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Output")
	FString RgbPngOutputPath = TEXT("Saved/MapCaptures/{level}_Map_RGB.png");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Capture|Output")
	FString MaskPngPath = TEXT("Saved/MapCaptures/{level}_Map_Mask.png");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Capture|Last Result")
	FVector2D LastDetectedLocalMin = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Capture|Last Result")
	FVector2D LastDetectedLocalMax = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Capture|Last Result")
	FIntPoint LastCaptureResolution = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Capture|Last Result")
	FString LastWrittenRgbPngAbsolutePath;

private:
	void UpdatePreviewComponents();
	bool AutoDetectCaptureBoundsInternal();
	bool CaptureOpaqueRgbPngInternal();
	bool IsBoundsHitUsable(const FHitResult& Hit) const;
	FIntPoint ResolveCaptureResolution() const;
	FString ResolveRgbOutputPath() const;
	bool WritePngFile(const FString& AbsolutePath, const TArray<FColor>& Pixels, int32 Width, int32 Height) const;
};
