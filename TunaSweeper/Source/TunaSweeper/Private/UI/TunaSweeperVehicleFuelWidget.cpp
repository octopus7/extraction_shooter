#include "UI/TunaSweeperVehicleFuelWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Layout/Clipping.h"
#include "Rendering/DrawElements.h"

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
	if (Size.X <= 10.0f || Size.Y <= 10.0f) return BaseLayer;

	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	const ESlateDrawEffect Effects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	auto DrawCapsule = [&](float Inset, const FLinearColor& Color, int32 Layer)
	{
		const FVector2D CapsuleSize = Size - FVector2D(Inset * 2.0f);
		const FSlateRoundedBoxBrush Brush(Color, static_cast<float>(CapsuleSize.Y * 0.5f));
		FSlateDrawElement::MakeBox(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(CapsuleSize, FSlateLayoutTransform(FVector2D(Inset))),
			&Brush, Effects, Color * Tint);
	};

	// The black outer rim and white inner rim remain readable against both snow and shadows.
	DrawCapsule(0.0f, FLinearColor::Black, BaseLayer + 1);
	DrawCapsule(2.0f, FLinearColor::White, BaseLayer + 2);
	DrawCapsule(4.0f, FLinearColor::Black, BaseLayer + 3);
	if (FuelRatio > 0.0f)
	{
		constexpr float FillInset = 5.0f;
		const FVector2D FillSize = Size - FVector2D(FillInset * 2.0f);
		// Clip the full capsule instead of shrinking it: the advancing edge stays proportional to fuel.
		OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry.ToPaintGeometry(
			FVector2D(FillSize.X * FuelRatio, FillSize.Y), FSlateLayoutTransform(FVector2D(FillInset)))));
		DrawCapsule(FillInset, FLinearColor(1.0f, 0.28f, 0.015f), BaseLayer + 4);
		OutDrawElements.PopClip();
	}
	return BaseLayer + 4;
}
