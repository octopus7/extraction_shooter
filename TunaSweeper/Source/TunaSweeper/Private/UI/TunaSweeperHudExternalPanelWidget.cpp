#include "UI/TunaSweeperHudExternalPanelWidget.h"

#include "Components/OverlaySlot.h"
#include "Components/Widget.h"
#include "UI/ItemContainerWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperWorkbenchPanelWidget.h"

namespace
{
	bool HasVisibleStorageContainer(const UStorageContainerWidget* StorageContainerWidget)
	{
		return StorageContainerWidget != nullptr;
	}

	bool HasVisibleShopContainer(const UShopContainerWidget* ShopContainerWidget)
	{
		return ShopContainerWidget != nullptr;
	}

	void AlignOverlayChild(UWidget* Widget, EVerticalAlignment VerticalAlignment)
	{
		if (Widget)
		{
			if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Widget->Slot))
			{
				OverlaySlot->SetHorizontalAlignment(HAlign_Right);
				OverlaySlot->SetVerticalAlignment(VerticalAlignment);
			}
		}
	}
}

void UTunaSweeperHudExternalPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyLootContainerPanelLayout();
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

	if (StorageContainerWidget)
	{
		StorageContainerWidget->SetStorageView();
	}
	else if (LootContainerWidget)
	{
		LootContainerWidget->SetStorageView();
	}
}

void UTunaSweeperHudExternalPanelWidget::SetShopContainer(int32 ShopId)
{
	SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::Shop);

	if (ShopContainerWidget)
	{
		ShopContainerWidget->SetShopView(ShopId);
	}
	else if (LootContainerWidget)
	{
		LootContainerWidget->SetShopView(ShopId);
	}
}

void UTunaSweeperHudExternalPanelWidget::SetWorkbenchContainer(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::Workbench);

	if (LootContainerWidget && !WorkbenchPanelWidget)
	{
		LootContainerWidget->SetWorkbenchView(WorkbenchId, WorkbenchMode);
	}
	if (WorkbenchPanelWidget)
	{
		WorkbenchPanelWidget->SetWorkbenchContext(WorkbenchId, WorkbenchMode);
	}
}

bool UTunaSweeperHudExternalPanelWidget::AssignWorkbenchDismantleCandidateToTarget(
	const FTunaSweeperItemSlotReference& SlotReference)
{
	return PanelMode == ETunaSweeperHudExternalPanelMode::Workbench &&
		WorkbenchPanelWidget &&
		WorkbenchPanelWidget->AssignDismantleCandidateToTarget(SlotReference);
}

bool UTunaSweeperHudExternalPanelWidget::AssignFocusedWorkbenchDismantleCandidateToTarget()
{
	return PanelMode == ETunaSweeperHudExternalPanelMode::Workbench &&
		WorkbenchPanelWidget &&
		WorkbenchPanelWidget->AssignFocusedDismantleCandidateToTarget();
}

bool UTunaSweeperHudExternalPanelWidget::AssignWorkbenchBlueprintItemToTarget(
	const FTunaSweeperItemSlotReference& SlotReference)
{
	return PanelMode == ETunaSweeperHudExternalPanelMode::Workbench &&
		WorkbenchPanelWidget &&
		WorkbenchPanelWidget->AssignBlueprintItemToTarget(SlotReference);
}

bool UTunaSweeperHudExternalPanelWidget::AssignFocusedWorkbenchBlueprintItemToTarget()
{
	return PanelMode == ETunaSweeperHudExternalPanelMode::Workbench &&
		WorkbenchPanelWidget &&
		WorkbenchPanelWidget->AssignFocusedBlueprintItemToTarget();
}

void UTunaSweeperHudExternalPanelWidget::ApplyPanelMode()
{
	ApplyLootContainerPanelLayout();

	if (LootingBoxPanel)
	{
		const bool bUseFallbackLootContainerPanel =
			(PanelMode == ETunaSweeperHudExternalPanelMode::Storage && !HasVisibleStorageContainer(StorageContainerWidget)) ||
			(PanelMode == ETunaSweeperHudExternalPanelMode::Shop && !HasVisibleShopContainer(ShopContainerWidget)) ||
			(PanelMode == ETunaSweeperHudExternalPanelMode::Workbench && !WorkbenchPanelWidget);
		const bool bShowContainerPanel =
			PanelMode == ETunaSweeperHudExternalPanelMode::LootingBox ||
			bUseFallbackLootContainerPanel;
		LootingBoxPanel->SetVisibility(bShowContainerPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ShopPanel)
	{
		ShopPanel->SetVisibility(
			PanelMode == ETunaSweeperHudExternalPanelMode::Shop && HasVisibleShopContainer(ShopContainerWidget)
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (StoragePanel)
	{
		StoragePanel->SetVisibility(
			PanelMode == ETunaSweeperHudExternalPanelMode::Storage && HasVisibleStorageContainer(StorageContainerWidget)
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
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

void UTunaSweeperHudExternalPanelWidget::ApplyLootContainerPanelLayout()
{
	AlignOverlayChild(LootContainerWidget, VAlign_Top);
	AlignOverlayChild(StorageContainerWidget, VAlign_Fill);
	AlignOverlayChild(ShopContainerWidget, VAlign_Top);
}
