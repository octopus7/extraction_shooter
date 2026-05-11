#include "UI/TunaSweeperHudExternalPanelWidget.h"

#include "Components/Widget.h"

void UTunaSweeperHudExternalPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyPanelMode();
}

void UTunaSweeperHudExternalPanelWidget::SetExternalPanelMode(ETunaSweeperHudExternalPanelMode InPanelMode)
{
	PanelMode = InPanelMode;
	ApplyPanelMode();
}

void UTunaSweeperHudExternalPanelWidget::ApplyPanelMode()
{
	if (LootingBoxPanel)
	{
		LootingBoxPanel->SetVisibility(PanelMode == ETunaSweeperHudExternalPanelMode::LootingBox ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ShopPanel)
	{
		ShopPanel->SetVisibility(PanelMode == ETunaSweeperHudExternalPanelMode::Shop ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (StoragePanel)
	{
		StoragePanel->SetVisibility(PanelMode == ETunaSweeperHudExternalPanelMode::Storage ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

