#include "UI/TunaSweeperVisionMaskWidget.h"

#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"

void UTunaSweeperVisionMaskWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperVisionMaskWidget::SetMaskTexture(UTexture2D* InMaskTexture)
{
	MaskTexture = InMaskTexture;
	MaskBrush.SetResourceObject(MaskTexture);
	MaskBrush.DrawAs = ESlateBrushDrawType::Image;
	MaskBrush.Tiling = ESlateBrushTileType::NoTile;
	MaskBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
	MaskBrush.TintColor = FSlateColor(FLinearColor::White);
}

void UTunaSweeperVisionMaskWidget::SetMaskVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

int32 UTunaSweeperVisionMaskWidget::NativePaint(
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

	if (!MaskTexture || GetVisibility() == ESlateVisibility::Collapsed)
	{
		return PaintedLayerId;
	}

	MaskBrush.ImageSize = AllottedGeometry.GetLocalSize();
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		PaintedLayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		&MaskBrush,
		ESlateDrawEffect::None,
		InWidgetStyle.GetColorAndOpacityTint());

	return PaintedLayerId + 1;
}
