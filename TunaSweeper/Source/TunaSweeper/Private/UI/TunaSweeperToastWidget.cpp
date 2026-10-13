#include "UI/TunaSweeperToastWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperToastWidget
{
	constexpr TCHAR FrameTexturePath[] = TEXT("/Game/UI/Toast/T_UI_QuestToastInnerFrameWide.T_UI_QuestToastInnerFrameWide");
}

TSharedRef<SWidget> UTunaSweeperToastWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	EnsureToastLayout();
	return Super::RebuildWidget();
}

void UTunaSweeperToastWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureToastLayout();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetToastOpacity(0.0f);
}

void UTunaSweeperToastWidget::SetToastMessage(const FText& MessageText)
{
	EnsureToastLayout();

	if (ToastText)
	{
		ToastText->SetText(MessageText);
	}
}

void UTunaSweeperToastWidget::SetToastOpacity(float InOpacity)
{
	EnsureToastLayout();

	if (ToastPanel)
	{
		const float ClampedOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
		ToastPanel->SetRenderOpacity(ClampedOpacity);
		ToastPanel->SetVisibility(ClampedOpacity > 0.0f
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperToastWidget::EnsureToastLayout()
{
	if (!WidgetTree || ToastPanel)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("ToastRootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}

	if (!RootCanvas)
	{
		return;
	}

	RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);

	ToastPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("ToastPanel"));
	UOverlay* ToastContent = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(),
		TEXT("ToastContent"));
	ToastFrameImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("ToastInnerFrame"));
	ToastText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ToastText"));
	if (!ToastPanel || !ToastContent || !ToastFrameImage || !ToastText)
	{
		ToastPanel = nullptr;
		ToastFrameImage = nullptr;
		ToastText = nullptr;
		return;
	}

	FSlateBrush ToastBrush;
	ToastBrush.DrawAs = ESlateBrushDrawType::Box;
	ToastBrush.TintColor = FSlateColor(FLinearColor(0.015f, 0.024f, 0.026f, 0.92f));
	ToastPanel->SetBrush(ToastBrush);
	ToastPanel->SetPadding(FMargin(0.0f));
	ToastPanel->SetVisibility(ESlateVisibility::Collapsed);
	ToastPanel->SetRenderOpacity(0.0f);

	if (UTexture2D* FrameTexture = LoadObject<UTexture2D>(nullptr, TunaSweeperToastWidget::FrameTexturePath))
	{
		ToastFrameImage->SetBrushFromTexture(FrameTexture, false);
		FSlateBrush FrameBrush = ToastFrameImage->GetBrush();
		FrameBrush.DrawAs = ESlateBrushDrawType::Image;
		ToastFrameImage->SetBrush(FrameBrush);
	}
	ToastFrameImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.72f));
	ToastFrameImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* FrameSlot = ToastContent->AddChildToOverlay(ToastFrameImage))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
		FrameSlot->SetPadding(FMargin(6.0f, 5.0f));
	}

	ToastText->SetAutoWrapText(true);
	ToastText->SetWrapTextAt(500.0f);
	ToastText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.92f, 0.84f, 1.0f)));
	ToastText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(ToastText, 18, ETunaSweeperUIFontWeight::Bold);
	if (UOverlaySlot* TextSlot = ToastContent->AddChildToOverlay(ToastText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(34.0f, 10.0f));
	}
	ToastPanel->SetContent(ToastContent);

	UCanvasPanelSlot* ToastSlot = RootCanvas->AddChildToCanvas(ToastPanel);
	if (ToastSlot)
	{
		ToastSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		ToastSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		ToastSlot->SetPosition(FVector2D(0.0f, 88.0f));
		ToastSlot->SetSize(FVector2D(560.0f, 72.0f));
		ToastSlot->SetZOrder(0);
	}
}

void UTunaSweeperToastWidget::SetRewardPresentation(const FText& Message, UTexture2D* Texture)
{
    EnsureToastLayout();
    if (!ToastPanel || !ToastText) return;
    auto* Content = Cast<UOverlay>(ToastPanel->GetContent());
    if (!Content) return;
    if (!RewardIcon)
    {
        RewardIcon = WidgetTree->ConstructWidget<UImage>();
        auto* IconFrame = WidgetTree->ConstructWidget<USizeBox>();
        IconFrame->SetWidthOverride(36); IconFrame->SetHeightOverride(36);
        IconFrame->SetContent(RewardIcon);
        auto* IconSlot = Content->AddChildToOverlay(IconFrame);
        IconSlot->SetHorizontalAlignment(HAlign_Left); IconSlot->SetVerticalAlignment(VAlign_Center);
        IconSlot->SetPadding(FMargin(24, 0, 0, 0));
    }
    RewardIcon->SetBrushFromTexture(Texture);
    ToastText->SetText(Message);
    ToastText->SetJustification(ETextJustify::Left);
    ToastText->SetWrapTextAt(280);
    if (auto* TextSlot = Cast<UOverlaySlot>(ToastText->Slot)) TextSlot->SetPadding(FMargin(76, 10, 24, 10));
    if (auto* PanelSlot = Cast<UCanvasPanelSlot>(ToastPanel->Slot))
    {
        PanelSlot->SetAnchors(FAnchors(1.0f, 0.30f));
        PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
        PanelSlot->SetPosition(FVector2D(-32.0f, 0.0f));
        PanelSlot->SetSize(FVector2D(400.0f, 80.0f));
    }
}
