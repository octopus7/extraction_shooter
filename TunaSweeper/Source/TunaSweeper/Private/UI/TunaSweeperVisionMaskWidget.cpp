#include "UI/TunaSweeperVisionMaskWidget.h"

#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

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
	MaskBrush.ImageSize = FVector2D(1.0f, 1.0f);
	MaskBrush.Tiling = ESlateBrushTileType::NoTile;
	MaskBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
	MaskBrush.TintColor = FSlateColor(FLinearColor::White);
}

void UTunaSweeperVisionMaskWidget::SetMaskMesh(
	TArray<FTunaSweeperVisionMaskVertex>&& InVertices,
	TArray<SlateIndex>&& InIndices)
{
	MaskVertices = MoveTemp(InVertices);
	MaskIndices = MoveTemp(InIndices);
}

void UTunaSweeperVisionMaskWidget::ClearMaskMesh()
{
	MaskVertices.Reset();
	MaskIndices.Reset();
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

	if (GetVisibility() == ESlateVisibility::Collapsed)
	{
		return PaintedLayerId;
	}

	if (MaskVertices.Num() >= 3 && MaskIndices.Num() >= 3 && FSlateApplication::IsInitialized())
	{
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		if (WhiteBrush && FSlateApplication::Get().GetRenderer())
		{
			PaintVertices.Reset(MaskVertices.Num());
			PaintVertices.Reserve(MaskVertices.Num());

			const FSlateRenderTransform& SlateRenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
			for (const FTunaSweeperVisionMaskVertex& MaskVertex : MaskVertices)
			{
				PaintVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
					SlateRenderTransform,
					FVector2f(MaskVertex.Position),
					FVector2f::ZeroVector,
					MaskVertex.Color));
			}

			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				PaintedLayerId + 1,
				FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush),
				PaintVertices,
				MaskIndices,
				nullptr,
				0,
				0);

			return PaintedLayerId + 1;
		}
	}

	if (!MaskTexture)
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
