#include "TunaSweeperGameHudWidgetShared.h"

void UTunaSweeperGameHudWidget::SetCenterPanelsVisible(bool bVisible)
{
	if (!bVisible)
	{
		CloseLootContainerPanelIfOpen();
	}

	ActiveHudMode = bVisible && ActiveHudMode == ETunaSweeperHudMode::None
		? ETunaSweeperHudMode::Inventory
		: (bVisible ? ActiveHudMode : ETunaSweeperHudMode::None);
	ApplyHudModeVisibility();
}

void UTunaSweeperGameHudWidget::SetInventoryAreaVisible(bool bVisible)
{
	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ApplyHudModeVisibility();
	}

	if (InventoryAreaWidget)
	{
		SetTransitionedWidgetVisibility(
			InventoryAreaWidget,
			bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			InventoryAreaTransitionEdge);
		InventoryAreaWidget->SetInventoryVisible(bVisible);
	}
}

void UTunaSweeperGameHudWidget::SetItemInfoPanelVisible(bool bVisible)
{
	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ApplyHudModeVisibility();
	}

	if (ItemInfoPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			ItemInfoPanelWidget,
			bVisible && ActiveHudMode == ETunaSweeperHudMode::Inventory
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			ItemInfoPanelTransitionEdge);
	}
}

void UTunaSweeperGameHudWidget::ShowExternalPanel(ETunaSweeperHudExternalPanelMode PanelMode)
{
	if (PanelMode != ETunaSweeperHudExternalPanelMode::LootingBox &&
		PanelMode != ETunaSweeperHudExternalPanelMode::Storage &&
		PanelMode != ETunaSweeperHudExternalPanelMode::Shop)
	{
		if (PanelMode != ETunaSweeperHudExternalPanelMode::Workbench)
		{
			CloseLootContainerPanelIfOpen();
		}
	}

	if (PanelMode != ETunaSweeperHudExternalPanelMode::None)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
	}

	if (ExternalPanelWidget)
	{
		if (PanelMode != ETunaSweeperHudExternalPanelMode::None)
		{
			bClearExternalPanelModeAfterHide = false;
			ExternalPanelWidget->SetExternalPanelMode(PanelMode);
		}
		else if (!bClearExternalPanelModeAfterHide ||
			(!IsSlateVisibilityShown(ExternalPanelWidget->GetVisibility()) && !HasActiveHudTransition(ExternalPanelWidget)))
		{
			bClearExternalPanelModeAfterHide = false;
			ExternalPanelWidget->SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::None);
		}
	}

	ApplyHudModeVisibility();
}

void UTunaSweeperGameHudWidget::ShowInventoryOnlyPanel()
{
	if (IsBunkerMap())
	{
		ShowStoragePanel();
		return;
	}

	SetHudMode(ETunaSweeperHudMode::Inventory);
	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::None);
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ToggleInventoryOnlyPanel()
{
	if (ActiveHudMode == ETunaSweeperHudMode::None)
	{
		ShowInventoryOnlyPanel();
	}
	else
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearSelectedItemSelection();
		}

		CloseLootContainerPanelIfOpen();
		SetHudMode(ETunaSweeperHudMode::None);
	}
}

void UTunaSweeperGameHudWidget::ShowLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance)
{
	ActiveHudMode = ETunaSweeperHudMode::Inventory;

	if (ExternalPanelWidget)
	{
		bClearExternalPanelModeAfterHide = false;
		ExternalPanelWidget->SetLootContainerInstance(ContainerInstance);
	}

	ApplyHudModeVisibility();
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowStoragePanel()
{
	if (!IsBunkerMap())
	{
		return;
	}

	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::Storage);
	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetStorageContainer();
	}

	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowShopPanel(int32 ShopId)
{
	if (!IsBunkerMap())
	{
		return;
	}

	CloseLootContainerPanelIfOpen();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetActiveShop(ShopId);
	}

	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::Shop);
	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetShopContainer(ShopId);
	}

	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowWorkbenchPanel(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	if (!IsBunkerMap())
	{
		return;
	}

	CloseLootContainerPanelIfOpen();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetActiveWorkbench(WorkbenchId, WorkbenchMode);
		if (WorkbenchMode == ETunaSweeperWorkbenchMode::Craft)
		{
			TunaGameInstance->ClearSelectedItemSelection();
		}
	}

	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::Workbench);
	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetWorkbenchContainer(WorkbenchId, WorkbenchMode);
	}

	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowMemoPanel(int32 MemoId)
{
	SetHudMode(ETunaSweeperHudMode::Memo);
	EnsureMemoPanelWidget();
	if (MemoPanelWidget)
	{
		MemoPanelWidget->OpenMemo(MemoId);
	}
}

void UTunaSweeperGameHudWidget::ShowQuestPanel(FName QuestId)
{
	bQuestPanelOpenedFromInteraction = true;
	SetHudMode(ETunaSweeperHudMode::Quest);
	EnsureQuestPanelWidgets();
	if (InteractionQuestPanelWidget)
	{
		InteractionQuestPanelWidget->InitializeQuest(QuestId);
	}
}

void UTunaSweeperGameHudWidget::ShowMenuQuestPanel(FName QuestId)
{
	bQuestPanelOpenedFromInteraction = false;
	SetHudMode(ETunaSweeperHudMode::Quest);
	EnsureQuestPanelWidgets();
	if (MenuQuestPanelWidget)
	{
		MenuQuestPanelWidget->InitializeQuest(QuestId);
	}
}

void UTunaSweeperGameHudWidget::ShowHousingFacilityContextMenu(FGuid InstanceId, FVector2D ScreenPosition)
{
	if (!InstanceId.IsValid())
	{
		HideHousingFacilityContextMenu();
		return;
	}

	EnsureHousingFacilityContextMenuWidget();
	if (!HousingContextMenuPanel)
	{
		return;
	}

	HousingContextMenuInstanceId = InstanceId;
	if (HousingContextStoreText)
	{
		HousingContextStoreText->SetText(ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.housing.context.store"),
			TEXT("수납")));
	}

	const FVector2D MenuSize(132.0f, 42.0f);
	const FVector2D CursorOffset(10.0f, 10.0f);
	FVector2D MenuPosition = ScreenPosition;
	FVector2D MenuBounds = UWidgetLayoutLibrary::GetViewportSize(this);
	if (const UCanvasPanel* RootCanvas = WidgetTree
		? Cast<UCanvasPanel>(WidgetTree->RootWidget)
		: nullptr)
	{
		const FGeometry& RootGeometry = RootCanvas->GetCachedGeometry();
		const FVector2D LocalSize = RootGeometry.GetLocalSize();
		if (LocalSize.X > 1.0f && LocalSize.Y > 1.0f)
		{
			MenuPosition = RootGeometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
			MenuBounds = LocalSize;
		}
	}

	const FVector2D ClampedPosition(
		FMath::Clamp(MenuPosition.X + CursorOffset.X, 0.0f, FMath::Max(0.0f, MenuBounds.X - MenuSize.X - 8.0f)),
		FMath::Clamp(MenuPosition.Y + CursorOffset.Y, 0.0f, FMath::Max(0.0f, MenuBounds.Y - MenuSize.Y - 8.0f)));

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(HousingContextMenuPanel->Slot))
	{
		CanvasSlot->SetPosition(ClampedPosition);
		CanvasSlot->SetSize(MenuSize);
	}

	HousingContextMenuPanel->SetVisibility(ESlateVisibility::Visible);
	UE_LOG(
		LogTunaSweeperGameHud,
		Log,
		TEXT("Showing housing facility context menu. InstanceId=%s RawMouse=(%.1f, %.1f) LocalMouse=(%.1f, %.1f) Menu=(%.1f, %.1f) Bounds=(%.1f, %.1f)"),
		*InstanceId.ToString(),
		ScreenPosition.X,
		ScreenPosition.Y,
		MenuPosition.X,
		MenuPosition.Y,
		ClampedPosition.X,
		ClampedPosition.Y,
		MenuBounds.X,
		MenuBounds.Y);
}

void UTunaSweeperGameHudWidget::HideHousingFacilityContextMenu()
{
	HousingContextMenuInstanceId.Invalidate();
	if (HousingContextMenuPanel)
	{
		HousingContextMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::SetHudMode(ETunaSweeperHudMode InHudMode)
{
	if (InHudMode != ETunaSweeperHudMode::None)
	{
		HideHousingFacilityContextMenu();
	}

	if (InHudMode != ETunaSweeperHudMode::None && IsHousingModeActive())
	{
		return;
	}

	if (ActiveHudMode == ETunaSweeperHudMode::Quest && InHudMode != ETunaSweeperHudMode::Quest)
	{
		if (MenuQuestPanelWidget)
		{
			MenuQuestPanelWidget->ResetQuestSelection();
		}
		if (InteractionQuestPanelWidget)
		{
			InteractionQuestPanelWidget->ResetQuestSelection();
		}
	}

	if (InHudMode != ETunaSweeperHudMode::Quest)
	{
		bQuestPanelOpenedFromInteraction = false;
	}

	if (InHudMode != ETunaSweeperHudMode::Inventory)
	{
		CloseLootContainerPanelIfOpen();
	}

	if (InHudMode != ETunaSweeperHudMode::None)
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr)
		{
			HousingSubsystem->CloseHousingMode();
		}
	}

	ActiveHudMode = InHudMode;
	if (ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		IsBunkerMap() &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::None)
	{
		bClearExternalPanelModeAfterHide = false;
		ExternalPanelWidget->SetStorageContainer();
	}
	ApplyHudModeVisibility();
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::SetExtractionProgress(
	float CurrentSeconds,
	float RequiredSeconds,
	bool bActive)
{
	ExtractionProgressCurrentSeconds = FMath::Max(0.0f, CurrentSeconds);
	ExtractionProgressRequiredSeconds = FMath::Max(0.1f, RequiredSeconds);
	bExtractionProgressActive = bActive && ExtractionProgressCurrentSeconds > 0.0f;
	RefreshExtractionProgressWidget();
}

bool UTunaSweeperGameHudWidget::IsInventoryUiOpen() const
{
	if (ActiveHudMode != ETunaSweeperHudMode::None)
	{
		return true;
	}

	auto IsWidgetVisible = [](const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}

		const ESlateVisibility Visibility = Widget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	};

	return
		IsWidgetVisible(InventoryAreaWidget) ||
		IsWidgetVisible(ItemInfoPanelWidget) ||
		IsWidgetVisible(ExternalPanelWidget) ||
		IsWidgetVisible(InventoryQuickSlotPanel) ||
		IsWidgetVisible(ShopSellPanelWidget);
}

bool UTunaSweeperGameHudWidget::IsShopPanelOpen() const
{
	return ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Shop;
}

bool UTunaSweeperGameHudWidget::IsStoragePanelOpen() const
{
	return ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Storage;
}

bool UTunaSweeperGameHudWidget::IsWorkbenchPanelOpen() const
{
	return ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Workbench;
}

ETunaSweeperWorkbenchMode UTunaSweeperGameHudWidget::GetWorkbenchPanelMode() const
{
	const UTunaSweeperGameInstance* WorkbenchGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return WorkbenchGameInstance && IsWorkbenchPanelOpen()
		? WorkbenchGameInstance->GetActiveWorkbenchMode()
		: ETunaSweeperWorkbenchMode::Craft;
}

bool UTunaSweeperGameHudWidget::TryAssignWorkbenchDismantleCandidateToTarget(
	const FTunaSweeperItemSlotReference& SlotReference)
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::Dismantle &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignWorkbenchDismantleCandidateToTarget(SlotReference);
}

bool UTunaSweeperGameHudWidget::TryAssignFocusedWorkbenchDismantleCandidateToTarget()
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::Dismantle &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignFocusedWorkbenchDismantleCandidateToTarget();
}

bool UTunaSweeperGameHudWidget::TryAssignWorkbenchBlueprintItemToTarget(
	const FTunaSweeperItemSlotReference& SlotReference)
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::BlueprintRegister &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignWorkbenchBlueprintItemToTarget(SlotReference);
}

bool UTunaSweeperGameHudWidget::TryAssignFocusedWorkbenchBlueprintItemToTarget()
{
	return IsWorkbenchPanelOpen() &&
		GetWorkbenchPanelMode() == ETunaSweeperWorkbenchMode::BlueprintRegister &&
		ExternalPanelWidget &&
		ExternalPanelWidget->AssignFocusedWorkbenchBlueprintItemToTarget();
}

bool UTunaSweeperGameHudWidget::TrySellSelectedShopItem()
{
	if (!IsShopPanelOpen())
	{
		return false;
	}

	EnsureShopSellPanelWidget();
	return ShopSellPanelWidget && ShopSellPanelWidget->TrySellSelectedHoveredItem();
}

