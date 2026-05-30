#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "TunaSweeperReloadRingWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperReloadRingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Cancelable Action")
	void SetCancelableActionProgress(float InProgress, bool bInVisible);

	void SetReloadProgress(float InReloadProgress, bool bInVisible) { SetCancelableActionProgress(InReloadProgress, bInVisible); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cancelable Action")
	float GetCancelableActionProgress() const { return CancelableActionProgress; }

	float GetReloadProgress() const { return CancelableActionProgress; }

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Cancelable Action", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RingThickness = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Cancelable Action", meta = (ClampMin = "1.0", ClampMax = "360.0", UIMin = "1.0", UIMax = "360.0"))
	float GaugeSweepDegrees = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Cancelable Action")
	float GaugeStartAngleDegrees = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Cancelable Action")
	FLinearColor TrackColor = FLinearColor(0.18f, 0.24f, 0.22f, 0.62f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Cancelable Action")
	FLinearColor FillColor = FLinearColor(0.62f, 0.98f, 0.62f, 0.98f);

private:
	void AddRingSegment(
		float StartAngleRadians,
		float SweepRadians,
		float InnerRadius,
		float OuterRadius,
		const FLinearColor& Color,
		const FSlateRenderTransform& AccumulatedRenderTransform,
		const FVector2D& Center,
		TArray<FSlateVertex>& OutVertices,
		TArray<SlateIndex>& OutIndices) const;

	float CancelableActionProgress = 0.0f;
	bool bVisibleGauge = false;
	mutable TArray<FSlateVertex> PaintVertices;
	mutable TArray<SlateIndex> PaintIndices;
};
