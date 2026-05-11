#include "UI/TunaSweeperHudItemInfoPanelWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UTunaSweeperHudItemInfoPanelWidget::SetSelectedItemInfo(const FText& ItemName, const FText& ItemDescription, bool bShowModdingPanel)
{
	if (SelectedItemNameText)
	{
		SelectedItemNameText->SetText(ItemName);
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->SetText(ItemDescription);
	}

	SetModdingPanelVisible(bShowModdingPanel);
}

void UTunaSweeperHudItemInfoPanelWidget::ClearSelectedItemInfo()
{
	if (SelectedItemNameText)
	{
		SelectedItemNameText->SetText(FText::FromString(TEXT("No Item")));
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->SetText(FText::GetEmpty());
	}

	SetModdingPanelVisible(false);
}

void UTunaSweeperHudItemInfoPanelWidget::SetModdingPanelVisible(bool bVisible)
{
	if (ModdingPanel)
	{
		ModdingPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

