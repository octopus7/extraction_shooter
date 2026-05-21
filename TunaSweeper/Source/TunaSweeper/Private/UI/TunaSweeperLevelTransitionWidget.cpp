#include "UI/TunaSweeperLevelTransitionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "MediaTexture.h"

namespace
{
	constexpr float LetterboxPanelRatio = 0.10f;
	constexpr int32 VideoZOrder = 0;
	constexpr int32 LetterboxZOrder = 10;
	constexpr int32 MessageZOrder = 20;
	constexpr int32 BlackFadeZOrder = 30;

	void ConfigureLetterboxSlot(UCanvasPanelSlot* Slot, bool bTop)
	{
		if (!Slot)
		{
			return;
		}

		Slot->SetAnchors(bTop
			? FAnchors(0.0f, 0.0f, 1.0f, LetterboxPanelRatio)
			: FAnchors(0.0f, 1.0f - LetterboxPanelRatio, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetZOrder(LetterboxZOrder);
	}

	void SetCanvasZOrder(UWidget* Widget, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetZOrder(ZOrder);
		}
	}
}

void UTunaSweeperLevelTransitionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureLetterboxPanels();
	SetCanvasZOrder(VideoImage, VideoZOrder);
	SetCanvasZOrder(MessageBackground, MessageZOrder);
	SetCanvasZOrder(BlackFadePanel, BlackFadeZOrder);

	SetVideoVisible(false);
	SetBlackOpacity(0.0f);
	SetLetterboxEnabled(false);
	SetTransitionMessage(FText::GetEmpty());
}

void UTunaSweeperLevelTransitionWidget::SetVideoTexture(UMediaTexture* InMediaTexture)
{
	if (!VideoImage)
	{
		return;
	}

	FSlateBrush VideoBrush;
	VideoBrush.DrawAs = ESlateBrushDrawType::Image;
	VideoBrush.SetResourceObject(InMediaTexture);
	VideoBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));
	VideoImage->SetBrush(VideoBrush);
}

void UTunaSweeperLevelTransitionWidget::SetVideoVisible(bool bVisible)
{
	bVideoVisible = bVisible;
	if (VideoImage)
	{
		VideoImage->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	UpdateLetterboxVisibility();
}

void UTunaSweeperLevelTransitionWidget::SetBlackOpacity(float InOpacity)
{
	if (!BlackFadePanel)
	{
		return;
	}

	BlackFadePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BlackFadePanel->SetRenderOpacity(FMath::Clamp(InOpacity, 0.0f, 1.0f));
}

void UTunaSweeperLevelTransitionWidget::SetLetterboxEnabled(bool bEnabled)
{
	bLetterboxEnabled = bEnabled;
	EnsureLetterboxPanels();
	UpdateLetterboxVisibility();
}

void UTunaSweeperLevelTransitionWidget::SetTransitionMessage(const FText& InMessage)
{
	if (!TransitionMessageText)
	{
		return;
	}

	TransitionMessageText->SetText(InMessage);
	const ESlateVisibility MessageVisibility = InMessage.IsEmpty()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::SelfHitTestInvisible;

	TransitionMessageText->SetVisibility(MessageVisibility);
	if (MessageBackground)
	{
		MessageBackground->SetVisibility(MessageVisibility);
	}
}

void UTunaSweeperLevelTransitionWidget::EnsureLetterboxPanels()
{
	if (LetterboxTopPanel && LetterboxBottomPanel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	FSlateBrush LetterboxBrush;
	LetterboxBrush.DrawAs = ESlateBrushDrawType::Box;
	LetterboxBrush.TintColor = FSlateColor(FLinearColor::Black);
	LetterboxBrush.SetImageSize(FVector2D(1920.0f, 108.0f));

	if (!LetterboxTopPanel)
	{
		LetterboxTopPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("LetterboxTopPanel"));
		if (LetterboxTopPanel)
		{
			LetterboxTopPanel->SetBrush(LetterboxBrush);
			LetterboxTopPanel->SetVisibility(ESlateVisibility::Collapsed);
			ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxTopPanel), true);
		}
	}

	if (!LetterboxBottomPanel)
	{
		LetterboxBottomPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("LetterboxBottomPanel"));
		if (LetterboxBottomPanel)
		{
			LetterboxBottomPanel->SetBrush(LetterboxBrush);
			LetterboxBottomPanel->SetVisibility(ESlateVisibility::Collapsed);
			ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxBottomPanel), false);
		}
	}
}

void UTunaSweeperLevelTransitionWidget::UpdateLetterboxVisibility()
{
	const ESlateVisibility LetterboxVisibility = bVideoVisible && bLetterboxEnabled
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;

	if (LetterboxTopPanel)
	{
		LetterboxTopPanel->SetVisibility(LetterboxVisibility);
	}
	if (LetterboxBottomPanel)
	{
		LetterboxBottomPanel->SetVisibility(LetterboxVisibility);
	}
}
