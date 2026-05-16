#include "UI/TunaSweeperLevelTransitionWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "MediaTexture.h"

void UTunaSweeperLevelTransitionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVideoVisible(false);
	SetBlackOpacity(0.0f);
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
	if (VideoImage)
	{
		VideoImage->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
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
