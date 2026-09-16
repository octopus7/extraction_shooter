#include "UI/TunaSweeperCheckIndicatorWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

class STunaSweeperCheckIndicator final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(STunaSweeperCheckIndicator) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) {}
	void SetChecked(bool bValue)
	{
		if (bChecked != bValue)
		{
			bChecked = bValue;
			Invalidate(EInvalidateWidgetReason::Paint);
		}
	}

private:
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(26.0f); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		const ESlateDrawEffect Effects = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		const FVector2D Size = Geometry.GetLocalSize();
		const FLinearColor Tint = Style.GetColorAndOpacityTint();
		// A soft, borderless box stays visible when unchecked. The mark is drawn, never a glyph.
		const FSlateRoundedBoxBrush Box(FLinearColor(0.64f, 0.85f, 0.86f, bChecked ? 0.26f : 0.13f), 6.0f);
		FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(), &Box, Effects,
			Box.TintColor.GetSpecifiedColor() * Tint);
		if (bChecked)
		{
			const TArray<FVector2f> Points = {
				FVector2f(Size.X * 0.24f, Size.Y * 0.51f),
				FVector2f(Size.X * 0.43f, Size.Y * 0.71f),
				FVector2f(Size.X * 0.78f, Size.Y * 0.29f)
			};
			FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Points,
				Effects, FLinearColor::White * Tint, true, 2.6f);
		}
		return Layer + 1;
	}
	bool bChecked = false;
};

void UTunaSweeperCheckIndicatorWidget::SetChecked(bool bInChecked)
{
	bChecked = bInChecked;
	if (Indicator) Indicator->SetChecked(bChecked);
}

TSharedRef<SWidget> UTunaSweeperCheckIndicatorWidget::RebuildWidget()
{
	Indicator = SNew(STunaSweeperCheckIndicator);
	Indicator->SetChecked(bChecked);
	return Indicator.ToSharedRef();
}

void UTunaSweeperCheckIndicatorWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (Indicator) Indicator->SetChecked(bChecked);
}

void UTunaSweeperCheckIndicatorWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Indicator.Reset();
}
