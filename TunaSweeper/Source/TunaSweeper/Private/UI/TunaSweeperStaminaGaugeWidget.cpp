#include "UI/TunaSweeperStaminaGaugeWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void UTunaSweeperStaminaGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(0.0f);
}

void UTunaSweeperStaminaGaugeWidget::SetStaminaGauge(float InStaminaPercent, float InGaugeOpacity)
{
	StaminaPercent = FMath::Clamp(InStaminaPercent, 0.0f, 1.0f);
	GaugeOpacity = FMath::Clamp(InGaugeOpacity, 0.0f, 1.0f);
	SetRenderOpacity(GaugeOpacity);
	SetVisibility(GaugeOpacity > 0.01f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	InvalidateLayoutAndVolatility();
}

int32 UTunaSweeperStaminaGaugeWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 PaintedLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	if (GaugeOpacity <= 0.01f || !FSlateApplication::IsInitialized())
	{
		return PaintedLayerId;
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (!WhiteBrush || !FSlateApplication::Get().GetRenderer())
	{
		return PaintedLayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float OuterRadius = FMath::Max(1.0f, FMath::Min(LocalSize.X, LocalSize.Y) * 0.5f - 2.0f);
	const float InnerRadius = FMath::Max(1.0f, OuterRadius - FMath::Max(1.0f, RingThickness));
	const FVector2D Center(LocalSize.X * 0.5f, LocalSize.Y * 0.5f);
	const FSlateRenderTransform& AccumulatedRenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();

	PaintVertices.Reset();
	PaintIndices.Reset();
	PaintVertices.Reserve(292);
	PaintIndices.Reserve(438);

	const float GaugeSweepRadians = FMath::DegreesToRadians(FMath::Clamp(GaugeSweepDegrees, 1.0f, 360.0f));
	const float StartAngleRadians = FMath::DegreesToRadians(GaugeStartAngleDegrees);
	const FLinearColor EffectiveBackgroundColor = BackgroundColor.CopyWithNewOpacity(BackgroundColor.A * GaugeOpacity);
	AddRingSegment(
		StartAngleRadians,
		GaugeSweepRadians,
		InnerRadius,
		OuterRadius,
		EffectiveBackgroundColor,
		AccumulatedRenderTransform,
		Center,
		PaintVertices,
		PaintIndices);

	if (StaminaPercent > 0.0f)
	{
		const FLinearColor BaseFillColor = FMath::Lerp(EmptyColor, FillColor, StaminaPercent);
		const FLinearColor EffectiveFillColor = BaseFillColor.CopyWithNewOpacity(BaseFillColor.A * GaugeOpacity);
		AddRingSegment(
			StartAngleRadians,
			GaugeSweepRadians * StaminaPercent,
			InnerRadius,
			OuterRadius,
			EffectiveFillColor,
			AccumulatedRenderTransform,
			Center,
			PaintVertices,
			PaintIndices);
	}

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements,
		PaintedLayerId + 1,
		FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush),
		PaintVertices,
		PaintIndices,
		nullptr,
		0,
		0);

	return PaintedLayerId + 1;
}

void UTunaSweeperStaminaGaugeWidget::AddRingSegment(
	float StartAngleRadians,
	float SweepRadians,
	float InnerRadius,
	float OuterRadius,
	const FLinearColor& Color,
	const FSlateRenderTransform& AccumulatedRenderTransform,
	const FVector2D& Center,
	TArray<FSlateVertex>& OutVertices,
	TArray<SlateIndex>& OutIndices) const
{
	const float ClampedSweep = FMath::Clamp(SweepRadians, 0.0f, 2.0f * PI);
	if (ClampedSweep <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const int32 SegmentCount = FMath::Clamp(FMath::CeilToInt((ClampedSweep / (2.0f * PI)) * 72.0f), 2, 72);
	const FColor VertexColor = Color.ToFColor(true);
	const int32 BaseVertexIndex = OutVertices.Num();

	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float Angle = StartAngleRadians + ClampedSweep * Alpha;
		const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
		const FVector2D OuterPoint = Center + Direction * OuterRadius;
		const FVector2D InnerPoint = Center + Direction * InnerRadius;

		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			AccumulatedRenderTransform,
			FVector2f(OuterPoint),
			FVector2f::ZeroVector,
			VertexColor));
		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			AccumulatedRenderTransform,
			FVector2f(InnerPoint),
			FVector2f::ZeroVector,
			VertexColor));
	}

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const SlateIndex A = static_cast<SlateIndex>(BaseVertexIndex + SegmentIndex * 2);
		const SlateIndex B = static_cast<SlateIndex>(A + 1);
		const SlateIndex C = static_cast<SlateIndex>(A + 2);
		const SlateIndex D = static_cast<SlateIndex>(A + 3);

		OutIndices.Add(A);
		OutIndices.Add(C);
		OutIndices.Add(B);
		OutIndices.Add(C);
		OutIndices.Add(D);
		OutIndices.Add(B);
	}
}
