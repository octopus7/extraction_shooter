#include "UI/TunaSweeperHudTopReserveWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "UI/TunaSweeperUIFont.h"

void UTunaSweeperHudTopReserveWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (InventoryModeButton)
	{
		InventoryModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked);
		InventoryModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked);
	}

	if (QuestModeButton)
	{
		QuestModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked);
		QuestModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked);
	}

	if (MapModeButton)
	{
		MapModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMapModeClicked);
		MapModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMapModeClicked);
	}

	if (MemoModeButton)
	{
		MemoModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked);
		MemoModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked);
	}

	RefreshTabVisuals();
}

void UTunaSweeperHudTopReserveWidget::NativeDestruct()
{
	if (InventoryModeButton)
	{
		InventoryModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked);
	}

	if (QuestModeButton)
	{
		QuestModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked);
	}

	if (MapModeButton)
	{
		MapModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMapModeClicked);
	}

	if (MemoModeButton)
	{
		MemoModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked);
	}

	Super::NativeDestruct();
}

void UTunaSweeperHudTopReserveWidget::SetActiveMode(ETunaSweeperHudMode InActiveMode)
{
	ActiveMode = InActiveMode;
	RefreshTabVisuals();
}

void UTunaSweeperHudTopReserveWidget::RefreshTabVisuals()
{
	SetTabVisual(ETunaSweeperHudMode::Inventory, InventoryModeButton, InventoryModeIcon);
	SetTabVisual(ETunaSweeperHudMode::Quest, QuestModeButton, QuestModeIcon);
	SetTabVisual(ETunaSweeperHudMode::Map, MapModeButton, MapModeIcon);
	SetTabVisual(ETunaSweeperHudMode::Memo, MemoModeButton, MemoModeIcon);
}

void UTunaSweeperHudTopReserveWidget::SetTabVisual(ETunaSweeperHudMode Mode, UButton* Button, UImage* Icon)
{
	const bool bActive = ActiveMode == Mode;

	if (Button)
	{
		Button->SetRenderOpacity(bActive ? 1.0f : 0.72f);
	}

	if (Icon)
	{
		Icon->SetColorAndOpacity(
			bActive
				? FLinearColor(0.82f, 0.98f, 0.88f, 1.0f)
				: FLinearColor(0.74f, 0.80f, 0.82f, 1.0f));
	}
}

void UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Inventory);
}

void UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Quest);
}

void UTunaSweeperHudTopReserveWidget::HandleMapModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Map);
}

void UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Memo);
}
