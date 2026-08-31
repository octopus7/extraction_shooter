#include "UI/TunaThoughtIndicatorWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/Texture2D.h"

TSharedRef<SWidget> UTunaThoughtIndicatorWidget::RebuildWidget()
{
	EnsureWidgetTree();
	return Super::RebuildWidget();
}

void UTunaThoughtIndicatorWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureWidgetTree();
	ApplyPresentation();
	ApplyLayout();
}

void UTunaThoughtIndicatorWidget::Configure(
	UTexture2D* InTexture,
	FVector2D InImageSizePixels,
	FVector2D InCanvasSizePixels)
{
	IndicatorTexture = InTexture;
	ImageSizePixels.X = FMath::Max(1.0f, InImageSizePixels.X);
	ImageSizePixels.Y = FMath::Max(1.0f, InImageSizePixels.Y);
	CanvasSizePixels.X = FMath::Max(ImageSizePixels.X, InCanvasSizePixels.X);
	CanvasSizePixels.Y = FMath::Max(ImageSizePixels.Y, InCanvasSizePixels.Y);

	EnsureWidgetTree();
	ApplyPresentation();
	ApplyLayout();
}

void UTunaThoughtIndicatorWidget::SetBobOffsetPixels(float InOffsetPixels)
{
	BobOffsetPixels = InOffsetPixels;
	EnsureWidgetTree();
	ApplyLayout();
}

void UTunaThoughtIndicatorWidget::EnsureWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (!WidgetTree->RootWidget)
	{
		BuildNativeWidgetTree();
		return;
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	IconOverlay = Cast<UOverlay>(WidgetTree->FindWidget(TEXT("IconOverlay")));
	ShadowImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("ShadowImage")));
	IndicatorImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("IndicatorImage")));
}

void UTunaThoughtIndicatorWidget::BuildNativeWidgetTree()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("IndicatorRoot"));
	WidgetTree->RootWidget = RootCanvas;

	IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("IconOverlay"));
	if (UCanvasPanelSlot* IconSlot = RootCanvas->AddChildToCanvas(IconOverlay))
	{
		IconSlot->SetAutoSize(false);
		IconSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		IconSlot->SetAlignment(FVector2D::ZeroVector);
	}

	ShadowImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShadowImage"));
	if (UOverlaySlot* ShadowSlot = IconOverlay->AddChildToOverlay(ShadowImage))
	{
		ShadowSlot->SetHorizontalAlignment(HAlign_Fill);
		ShadowSlot->SetVerticalAlignment(VAlign_Fill);
	}

	IndicatorImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IndicatorImage"));
	if (UOverlaySlot* IndicatorSlot = IconOverlay->AddChildToOverlay(IndicatorImage))
	{
		IndicatorSlot->SetHorizontalAlignment(HAlign_Fill);
		IndicatorSlot->SetVerticalAlignment(VAlign_Fill);
	}

	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	IconOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ShadowImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	IndicatorImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyPresentation();
	ApplyLayout();
}

void UTunaThoughtIndicatorWidget::ApplyPresentation()
{
	if (!ShadowImage || !IndicatorImage)
	{
		return;
	}

	ShadowImage->SetBrushFromTexture(IndicatorTexture, false);
	ShadowImage->SetColorAndOpacity(FLinearColor(0.015f, 0.08f, 0.085f, 0.30f));
	ShadowImage->SetRenderTranslation(FVector2D(4.0f, 5.0f));

	IndicatorImage->SetBrushFromTexture(IndicatorTexture, false);
	IndicatorImage->SetColorAndOpacity(FLinearColor::White);
}

void UTunaThoughtIndicatorWidget::ApplyLayout()
{
	if (!IconOverlay)
	{
		return;
	}

	UCanvasPanelSlot* IconSlot = Cast<UCanvasPanelSlot>(IconOverlay->Slot);
	if (!IconSlot)
	{
		return;
	}

	const FVector2D CenteredPosition(
		(CanvasSizePixels.X - ImageSizePixels.X) * 0.5f,
		(CanvasSizePixels.Y - ImageSizePixels.Y) * 0.5f + BobOffsetPixels);
	IconSlot->SetPosition(CenteredPosition);
	IconSlot->SetSize(ImageSizePixels);
}
