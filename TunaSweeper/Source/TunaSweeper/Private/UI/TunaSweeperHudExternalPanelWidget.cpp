#include "UI/TunaSweeperHudExternalPanelWidget.h"

#include "Components/Widget.h"
#include "UI/TunaSweeperLootContainerWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperWorkbenchPanelWidget.h"

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

void UTunaSweeperHudExternalPanelWidget::SetShopContainer(int32 ShopId)
{
	SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::Shop);

	if (LootContainerWidget)
	{
		LootContainerWidget->SetShopView(ShopId);
	}
}

void UTunaSweeperHudExternalPanelWidget::SetWorkbenchContainer(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::Workbench);

	if (LootContainerWidget)
	{
		LootContainerWidget->SetWorkbenchView(WorkbenchId, WorkbenchMode);
	}
	if (WorkbenchPanelWidget)
	{
		WorkbenchPanelWidget->SetWorkbenchContext(WorkbenchId, WorkbenchMode);
	}
}

void UTunaSweeperHudExternalPanelWidget::ApplyPanelMode()
{
	if (LootingBoxPanel)
	{
		const bool bShowContainerPanel =
			PanelMode == ETunaSweeperHudExternalPanelMode::LootingBox ||
			PanelMode == ETunaSweeperHudExternalPanelMode::Storage ||
			PanelMode == ETunaSweeperHudExternalPanelMode::Shop ||
			(PanelMode == ETunaSweeperHudExternalPanelMode::Workbench && !WorkbenchPanelWidget);
		LootingBoxPanel->SetVisibility(bShowContainerPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ShopPanel)
	{
		ShopPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (StoragePanel)
	{
		StoragePanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (WorkbenchPanel)
	{
		WorkbenchPanel->SetVisibility(
			PanelMode == ETunaSweeperHudExternalPanelMode::Workbench && WorkbenchPanelWidget
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	else if (WorkbenchPanelWidget)
	{
		WorkbenchPanelWidget->SetVisibility(
			PanelMode == ETunaSweeperHudExternalPanelMode::Workbench
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}
