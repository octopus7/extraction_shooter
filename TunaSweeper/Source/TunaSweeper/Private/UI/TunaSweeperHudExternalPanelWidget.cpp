#include "UI/TunaSweeperHudExternalPanelWidget.h"

#include "Components/Widget.h"
#include "UI/TunaSweeperLootContainerWidget.h"
#include "UI/TunaSweeperUIFont.h"

void UTunaSweeperHudExternalPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyPanelMode();
}

void UTunaSweeperHudExternalPanelWidget::SetExternalPanelMode(ETunaSweeperHudExternalPanelMode InPanelMode)
{
	PanelMode = InPanelMode;
	ApplyPanelMode();
}

void UTunaSweeperHudExternalPanelWidget::SetLootContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance)
{
	SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::LootingBox);

	if (LootContainerWidget)
	{
		LootContainerWidget->SetContainerInstance(InContainerInstance);
	}
}

void UTunaSweeperHudExternalPanelWidget::SetStorageContainer()
{
	SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::Storage);

	if (LootContainerWidget)
	{
		LootContainerWidget->SetStorageView();
	}
}

void UTunaSweeperHudExternalPanelWidget::ApplyPanelMode()
{
	if (LootingBoxPanel)
	{
		const bool bShowContainerPanel =
			PanelMode == ETunaSweeperHudExternalPanelMode::LootingBox ||
			PanelMode == ETunaSweeperHudExternalPanelMode::Storage;
		LootingBoxPanel->SetVisibility(bShowContainerPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ShopPanel)
	{
		ShopPanel->SetVisibility(PanelMode == ETunaSweeperHudExternalPanelMode::Shop ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (StoragePanel)
	{
		StoragePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}
