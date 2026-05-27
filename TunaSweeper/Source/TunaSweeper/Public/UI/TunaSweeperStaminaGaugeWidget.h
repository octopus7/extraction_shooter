#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "TunaSweeperStaminaGaugeWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperStaminaGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Stamina")
	void SetStaminaGauge(float InStaminaPercent, float InGaugeOpacity);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Stamina", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RingThickness = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Stamina")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.03f, 0.035f, 0.54f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Stamina")
	FLinearColor FillColor = FLinearColor(0.23f, 0.92f, 0.72f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Stamina")
	FLinearColor EmptyColor = FLinearColor(0.95f, 0.24f, 0.18f, 0.95f);

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

	float StaminaPercent = 1.0f;
	float GaugeOpacity = 0.0f;
	mutable TArray<FSlateVertex> PaintVertices;
	mutable TArray<SlateIndex> PaintIndices;
};
