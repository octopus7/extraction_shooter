#include "UI/TunaSweeperSpeechBubbleWidget.h"

#include "Components/TextBlock.h"
#include "UI/TunaSweeperUIFont.h"

void UTunaSweeperSpeechBubbleWidget::SetBubbleText(const FText& InText)
{
	if (BubbleText)
	{
		TunaSweeperUIFont::ApplyFont(BubbleText, BubbleText->GetFont().Size);
		BubbleText->SetText(InText);
	}
}
