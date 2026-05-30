#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "TunaSweeperHudStatusRingWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudStatusRingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetStatusPercent(float InStatusPercent);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetRingColors(const FLinearColor& InTrackColor, const FLinearColor& InFillColor);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	float GetStatusPercent() const { return StatusPercent; }

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RingThickness = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", ClampMax = "360.0", UIMin = "1.0", UIMax = "360.0"))
	float GaugeSweepDegrees = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD")
	float GaugeStartAngleDegrees = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD")
	FLinearColor TrackColor = FLinearColor(0.035f, 0.040f, 0.045f, 0.78f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD")
	FLinearColor FillColor = FLinearColor(0.95f, 0.28f, 0.34f, 1.0f);

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

	float StatusPercent = 1.0f;
	mutable TArray<FSlateVertex> PaintVertices;
	mutable TArray<SlateIndex> PaintIndices;
};
