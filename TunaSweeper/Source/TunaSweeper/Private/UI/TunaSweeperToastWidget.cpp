#include "UI/TunaSweeperToastWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "UI/TunaSweeperUIFont.h"

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
	ToastText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ToastText"));
	if (!ToastPanel || !ToastText)
	{
		ToastPanel = nullptr;
		ToastText = nullptr;
		return;
	}

	FSlateBrush ToastBrush;
	ToastBrush.DrawAs = ESlateBrushDrawType::Box;
	ToastBrush.TintColor = FSlateColor(FLinearColor(0.015f, 0.024f, 0.026f, 0.92f));
	ToastPanel->SetBrush(ToastBrush);
	ToastPanel->SetPadding(FMargin(24.0f, 10.0f));
	ToastPanel->SetVisibility(ESlateVisibility::Collapsed);
	ToastPanel->SetRenderOpacity(0.0f);

	ToastText->SetAutoWrapText(true);
	ToastText->SetWrapTextAt(500.0f);
	ToastText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.92f, 0.84f, 1.0f)));
	ToastText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(ToastText, 18, ETunaSweeperUIFontWeight::Bold);
	ToastPanel->SetContent(ToastText);

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
