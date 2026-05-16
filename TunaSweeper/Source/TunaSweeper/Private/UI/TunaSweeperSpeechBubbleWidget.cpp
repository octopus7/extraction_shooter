#include "UI/TunaSweeperSpeechBubbleWidget.h"

#include "Components/TextBlock.h"

void UTunaSweeperSpeechBubbleWidget::SetBubbleText(const FText& InText)
{
	if (BubbleText)
	{
		BubbleText->SetText(InText);
	}
}
