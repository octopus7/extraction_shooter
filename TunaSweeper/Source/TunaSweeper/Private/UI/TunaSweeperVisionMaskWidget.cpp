#include "UI/TunaSweeperVisionMaskWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UTunaSweeperVisionMaskWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VisionMaskRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}

	if (!MaskImage)
	{
		MaskImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VisionMaskImage"));
		if (MaskImage)
		{
			RootCanvas->AddChild(MaskImage);
			if (UCanvasPanelSlot* ImageSlot = Cast<UCanvasPanelSlot>(MaskImage->Slot))
			{
				ImageSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				ImageSlot->SetOffsets(FMargin(0.0f));
				ImageSlot->SetAlignment(FVector2D::ZeroVector);
				ImageSlot->SetZOrder(0);
			}
		}
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (MaskImage)
	{
		MaskImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		MaskImage->SetBrush(MaskBrush);
		MaskImage->SetColorAndOpacity(FLinearColor::White);
	}
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
	if (MaskImage)
	{
		MaskImage->SetBrush(MaskBrush);
	}
}

void UTunaSweeperVisionMaskWidget::SetMaskVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (MaskImage)
	{
		MaskImage->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
