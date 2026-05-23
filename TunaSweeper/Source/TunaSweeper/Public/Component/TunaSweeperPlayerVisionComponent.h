#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "TunaSweeperPlayerVisionComponent.generated.h"

class APlayerController;
class UTexture2D;
class UTunaSweeperVisionMaskWidget;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperPlayerVisionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SightDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "1.0", ClampMax = "360.0", UIMin = "1.0", UIMax = "360.0"))
	float FieldOfViewDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AlwaysVisibleRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TraceHeight = 40.0f;

	UPROPERTY()
	int32 RayCount = 360;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "0.1", ClampMax = "45.0", UIMin = "0.1", UIMax = "5.0"))
	float RayAngleStepDegrees = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "1", ClampMax = "16", UIMin = "1", UIMax = "16"))
	int32 MaskDownsampleFactor = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "0", ClampMax = "16", UIMin = "0", UIMax = "16"))
	int32 BlurRadius = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float UpdateIntervalSeconds = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255"))
	int32 HiddenMaskAlpha = 220;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperPlayerVisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperPlayerVisionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void ForceRefreshVisionMask();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vision")
	UTexture2D* GetMaskTexture() const { return MaskTexture; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision")
	FTunaSweeperPlayerVisionSettings VisionSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision")
	bool bRenderVisionOverlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision")
	int32 OverlayZOrder = 1;

	UPROPERTY()
	int32 DebugRayStride = 1;

	UPROPERTY()
	float DebugDrawLifeTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision|Debug")
	bool bEnableDebugOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision|Debug")
	bool bShowDebugStatusMessage = true;

private:
	APlayerController* ResolveLocalPlayerController() const;
	bool ShouldUpdateVision() const;
	bool IsVisionDebugEnabled() const;
	void EnsureOverlayWidget(APlayerController* PlayerController);
	void ConfigureOverlayWidgetForViewport(const FIntPoint& InViewportSize);
	bool EnsureMaskTexture(const FIntPoint& InViewportSize);
	bool BuildVisibleRayDistances(
		TArray<float>& OutRayDistances,
		FVector& OutTraceOrigin,
		float& OutFacingYawDegrees,
		float& OutRayAngleStepDegrees);
	float TraceVisibleDistance(const FVector& TraceOrigin, const FVector& Direction, bool bInsideFieldOfView, FHitResult& OutHit) const;
	int32 RasterizeVisionMaskFromView(
		APlayerController* PlayerController,
		const TArray<float>& RayDistances,
		const FVector& TraceOrigin,
		float FacingYawDegrees,
		float RayAngleStepDegrees);
	void ApplyBlurToMask();
	void RebuildTexturePixels();
	void UploadMaskTexture();
	void DrawVisionDebug(
		const FVector& TraceOrigin,
		const FVector& Direction,
		float TraceDistance,
		const FHitResult& Hit) const;
	void DrawVisionDebugInsideFieldOfView() const;
	void DrawVisionDebugRangeWires() const;
	void DrawVisionDebugBounds(const FVector& TraceOrigin, const FVector& FacingDirection) const;
	void ShowVisionDebugStatus(const FString& StatusText, FColor TextColor = FColor::Cyan) const;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MaskTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperVisionMaskWidget> VisionMaskWidget;

	TArray<uint8> VisibilityMask;
	TArray<uint8> BlurScratchMask;
	TArray<uint8> BlurredMask;
	TArray<uint8> TexturePixels;
	FIntPoint MaskSize = FIntPoint::ZeroValue;
	FIntPoint ViewportSize = FIntPoint::ZeroValue;
	float TimeSinceLastMaskUpdate = 0.0f;
	mutable double LastDebugStatusTimeSeconds = -100.0;
};
