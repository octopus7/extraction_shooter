#include "UI/TunaSweeperSpeechBubbleWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/TunaSweeperUIFont.h"

void UTunaSweeperSpeechBubbleWidget::SetBubbleText(const FText& InText)
{
	ResolvePresentationWidgets();
	const bool bAlertPresentation = IsAlertBubbleText(InText);
	ApplyAlertPresentation(bAlertPresentation);

	if (BubbleText)
	{
		TunaSweeperUIFont::ApplyFont(BubbleText, bAlertPresentation ? 38 : DefaultFontSize);
		BubbleText->SetText(InText);
	}
}

bool UTunaSweeperSpeechBubbleWidget::IsAlertBubbleText(const FText& InText)
{
	return InText.ToString().TrimStartAndEnd().Equals(TEXT("!"), ESearchCase::CaseSensitive);
}

void UTunaSweeperSpeechBubbleWidget::ResolvePresentationWidgets()
{
	if (!BubbleBackground)
	{
		BubbleBackground = Cast<UBorder>(GetWidgetFromName(TEXT("BubbleBackground")));
	}
	if (!BubbleTail)
	{
		BubbleTail = Cast<UBorder>(GetWidgetFromName(TEXT("BubbleTail")));
	}
	if (!BubbleBackgroundSlot && BubbleBackground)
	{
		BubbleBackgroundSlot = Cast<UCanvasPanelSlot>(BubbleBackground->Slot);
	}

	if (!bDefaultPresentationCached && BubbleBackground && BubbleBackgroundSlot && BubbleText)
	{
		DefaultBackgroundBrush = BubbleBackground->Background;
		DefaultBackgroundPadding = BubbleBackground->GetPadding();
		DefaultBackgroundPosition = BubbleBackgroundSlot->GetPosition();
		DefaultBackgroundSize = BubbleBackgroundSlot->GetSize();
		DefaultFontSize = BubbleText->GetFont().Size;
		if (BubbleTail)
		{
			DefaultTailVisibility = BubbleTail->GetVisibility();
		}
		bDefaultPresentationCached = true;
	}
}

void UTunaSweeperSpeechBubbleWidget::ApplyAlertPresentation(const bool bAlertPresentation)
{
	if (!bDefaultPresentationCached || !BubbleBackground || !BubbleBackgroundSlot)
	{
		return;
	}

	if (bAlertPresentation)
	{
		if (!AlertCloudTexture)
		{
			AlertCloudTexture = LoadObject<UTexture2D>(
				nullptr,
				TEXT("/Game/UI/SpeechBubble/T_EnemyAlertCloud.T_EnemyAlertCloud"));
		}

		if (AlertCloudTexture)
		{
			FSlateBrush CloudBrush;
			CloudBrush.DrawAs = ESlateBrushDrawType::Image;
			CloudBrush.SetImageSize(FVector2D(1024.0f, 1024.0f));
			CloudBrush.SetResourceObject(AlertCloudTexture);
			CloudBrush.TintColor = FSlateColor(FLinearColor::White);
			BubbleBackground->SetBrush(CloudBrush);
		}
		BubbleBackground->SetPadding(FMargin(7.0f));
		BubbleBackgroundSlot->SetPosition(FVector2D(0.0f, 1.0f));
		BubbleBackgroundSlot->SetSize(FVector2D(72.0f, 72.0f));
		if (BubbleTail)
		{
			BubbleTail->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	BubbleBackground->SetBrush(DefaultBackgroundBrush);
	BubbleBackground->SetPadding(DefaultBackgroundPadding);
	BubbleBackgroundSlot->SetPosition(DefaultBackgroundPosition);
	BubbleBackgroundSlot->SetSize(DefaultBackgroundSize);
	if (BubbleTail)
	{
		BubbleTail->SetVisibility(DefaultTailVisibility);
	}
}
