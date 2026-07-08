#include "TunaSweeperGameHudWidgetShared.h"

void UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const bool bCenterVisible =
		ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		CenterContentPanel &&
		CenterContentPanel->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bHasSelection = bCenterVisible &&
		TunaGameInstance &&
		TunaGameInstance->HasSelectedInventoryItem();

	bool bShowShopSellPanel = false;
	if (bHasSelection && IsShopPanelOpen() && TunaGameInstance)
	{
		int32 SalePrice = 0;
		bShowShopSellPanel = TunaGameInstance->TryGetSlotSellPrice(
			TunaGameInstance->GetSelectedItemSlotReference(),
			SalePrice);
	}

	SetShopSellPanelVisible(bShowShopSellPanel);
	SetItemInfoPanelVisible(bHasSelection && !bShowShopSellPanel);
	if (bShowShopSellPanel && ShopSellPanelWidget)
	{
		ShopSellPanelWidget->RefreshSelectedItem();
	}
	else if (bHasSelection && ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->RefreshSelectedItemInfo();
	}
}

void UTunaSweeperGameHudWidget::HandleQuestProgressChanged()
{
	if (MenuQuestPanelWidget)
	{
		MenuQuestPanelWidget->RefreshQuestView();
	}
	if (InteractionQuestPanelWidget)
	{
		InteractionQuestPanelWidget->RefreshQuestView();
	}
}

void UTunaSweeperGameHudWidget::HandleHousingStateChanged()
{
	if (!IsHousingModeActive())
	{
		HideHousingFacilityContextMenu();
	}
	ApplyHudModeVisibility();
	RefreshDialogueHudVisibility();
	RefreshCancelableActionWidgets();
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void UTunaSweeperGameHudWidget::HandleLanguageChanged()
{
	RefreshLocalizedTexts();
	ApplyHudModeVisibility();
	RefreshBottomStatusFromGameInstance();
	RefreshQuickSlotsFromGameState();
	RefreshInventoryQuickSlotPanel();
	if (ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->RefreshSelectedItemInfo();
	}
	if (ShopSellPanelWidget)
	{
		ShopSellPanelWidget->RefreshSelectedItem();
	}
	if (MemoPanelWidget)
	{
		MemoPanelWidget->RefreshMemoView();
	}
	if (MenuQuestPanelWidget)
	{
		MenuQuestPanelWidget->RefreshQuestView();
	}
	if (InteractionQuestPanelWidget)
	{
		InteractionQuestPanelWidget->RefreshQuestView();
	}
}

void UTunaSweeperGameHudWidget::HandleHousingContextStoreClicked()
{
	const FGuid InstanceId = HousingContextMenuInstanceId;
	HideHousingFacilityContextMenu();
	if (!InstanceId.IsValid())
	{
		UE_LOG(LogTunaSweeperGameHud, Warning, TEXT("Housing context store failed: invalid context instance id."));
		return;
	}

	UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (!HousingSubsystem)
	{
		UE_LOG(
			LogTunaSweeperGameHud,
			Warning,
			TEXT("Housing context store failed: housing subsystem is missing. InstanceId=%s"),
			*InstanceId.ToString());
		return;
	}

	if (!HousingSubsystem->StoreFacility(InstanceId, true))
	{
		UE_LOG(
			LogTunaSweeperGameHud,
			Warning,
			TEXT("Housing context store failed: subsystem rejected store. InstanceId=%s"),
			*InstanceId.ToString());
		return;
	}

	UE_LOG(LogTunaSweeperGameHud, Log, TEXT("Housing context store succeeded. InstanceId=%s"), *InstanceId.ToString());
}

void UTunaSweeperGameHudWidget::HandleHudModeTabSelected(ETunaSweeperHudMode SelectedMode)
{
	if (SelectedMode == ETunaSweeperHudMode::Quest)
	{
		ShowMenuQuestPanel();
		return;
	}

	SetHudMode(SelectedMode);
}
