#include "UI/TunaSweeperVehicleFuelWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"

namespace
{
	FLinearColor FuelColor(float Ratio)
	{
		// The lower half passes red -> orange -> yellow -> green. The upper
		// half stays exactly the same green so a healthy tank is easy to read.
		const FLinearColor Stops[] = {
			FLinearColor(1.0f, 0.025f, 0.015f),
			FLinearColor(1.0f, 0.28f, 0.015f),
			FLinearColor(1.0f, 0.85f, 0.015f),
			FLinearColor(0.08f, 0.8f, 0.12f)
		};
		if (Ratio >= 0.5f) return Stops[3];
		const float Position = FMath::Clamp(Ratio * 6.0f, 0.0f, 3.0f);
		const int32 Index = FMath::Min(FMath::FloorToInt(Position), 2);
		return FMath::Lerp(Stops[Index], Stops[Index + 1], Position - Index);
	}
}

void UTunaSweeperVehicleFuelWidget::SetFuelRatio(float InFuelRatio)
{
	const float NewRatio = FMath::IsFinite(InFuelRatio) ? FMath::Clamp(InFuelRatio, 0.0f, 1.0f) : 0.0f;
	if (FuelRatio != NewRatio)
	{
		FuelRatio = NewRatio;
		InvalidateLayoutAndVolatility();
	}
}

int32 UTunaSweeperVehicleFuelWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	if (Size.X <= 16.0f || Size.Y <= 16.0f) return BaseLayer;

	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	const ESlateDrawEffect Effects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	constexpr float OuterWidth = 14.0f;
	const float Radius = FMath::Min(Size.X, Size.Y) * 0.5f - OuterWidth * 0.5f - 1.0f;
	const FVector2D Center = Size * 0.5f;
	auto MakeSpiral = [&](float End)
	{
		TArray<FVector2D> Points;
		const int32 Segments = FMath::Max(1, FMath::CeilToInt(96.0f * End));
		Points.Reserve(Segments + 1);
		for (int32 I = 0; I <= Segments; ++I)
		{
			const float Progress = End * static_cast<float>(I) / Segments;
			// Clockwise from lower-left, leaving a 90-degree opening below.
			// Radius grows from 50% to 100%; the center never collapses to a dot.
			const float Angle = FMath::DegreesToRadians(135.0f + 270.0f * Progress);
			const float Distance = Radius * FMath::Lerp(0.5f, 1.0f, Progress);
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance);
		}
		return Points;
	};

	auto DrawStroke = [&](const TArray<FVector2D>& Points, float Width, const FLinearColor& Color, int32 Layer)
	{
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(),
			Points, Effects, Color * Tint, true, Width);
		// Round the two ends with the same tinted material as the stroke.
		const FSlateRoundedBoxBrush Cap(Color, Width * 0.5f);
		for (const FVector2D& End : {Points[0], Points.Last()})
		{
			FSlateDrawElement::MakeBox(OutDrawElements, Layer,
				AllottedGeometry.ToPaintGeometry(FVector2D(Width),
					FSlateLayoutTransform(End - FVector2D(Width * 0.5f))), &Cap, Effects, Color * Tint);
		}
	};

	const TArray<FVector2D> Track = MakeSpiral(1.0f);
	// A fixed black/white track shows the available capacity at every fuel level.
	DrawStroke(Track, OuterWidth, FLinearColor::Black, BaseLayer + 1);
	DrawStroke(Track, 11.0f, FLinearColor::White, BaseLayer + 2);
	DrawStroke(Track, 8.0f, FLinearColor::Black, BaseLayer + 3);
	if (FuelRatio > 0.0f)
	{
		DrawStroke(MakeSpiral(FuelRatio), 6.0f, FuelColor(FuelRatio), BaseLayer + 4);
	}
	return BaseLayer + 4;
}
