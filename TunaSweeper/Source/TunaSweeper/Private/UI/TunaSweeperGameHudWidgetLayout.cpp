#include "TunaSweeperGameHudWidgetShared.h"

void UTunaSweeperGameHudWidget::ApplyHudModeVisibility()
{
	const bool bUtilityModeOpen = ActiveHudMode != ETunaSweeperHudMode::None;
	const bool bInventoryMode = ActiveHudMode == ETunaSweeperHudMode::Inventory;
	const bool bMapMode = ActiveHudMode == ETunaSweeperHudMode::Map;
	const bool bMemoMode = ActiveHudMode == ETunaSweeperHudMode::Memo;
	const bool bQuestMode = ActiveHudMode == ETunaSweeperHudMode::Quest;
	const UTunaSweeperGameInstance* WorkbenchGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const bool bWorkbenchPanelOpen =
		bInventoryMode &&
		ExternalPanelWidget &&
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Workbench;
	const bool bCraftWorkbenchPanelOpen =
		bWorkbenchPanelOpen &&
		(!WorkbenchGameInstance || WorkbenchGameInstance->GetActiveWorkbenchMode() == ETunaSweeperWorkbenchMode::Craft);

	if (TopStatusReserveWidget)
	{
		const bool bShowTopStatusReserve = bUtilityModeOpen && !(bQuestMode && bQuestPanelOpenedFromInteraction);
		SetTransitionedWidgetVisibility(
			TopStatusReserveWidget,
			bShowTopStatusReserve ? ESlateVisibility::Visible : ESlateVisibility::Collapsed,
			TopStatusReserveTransitionEdge);
		TopStatusReserveWidget->SetActiveMode(ActiveHudMode);
	}

	if (CenterContentPanel)
	{
		CenterContentPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (InventoryAreaWidget)
	{
		const bool bShowInventoryArea = bUtilityModeOpen && bInventoryMode && !bCraftWorkbenchPanelOpen;
		SetTransitionedWidgetVisibility(
			InventoryAreaWidget,
			bShowInventoryArea ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			InventoryAreaTransitionEdge);
		InventoryAreaWidget->SetInventoryVisible(bShowInventoryArea);
	}

	EnsureInventoryWeightPanelWidget();
	if (InventoryWeightPanel)
	{
		const bool bShowInventoryWeightPanel = bUtilityModeOpen && bInventoryMode && !bCraftWorkbenchPanelOpen;
		SetTransitionedWidgetVisibility(
			InventoryWeightPanel,
			bShowInventoryWeightPanel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed,
			ETunaSweeperHudTransitionEdge::Bottom);
	}

	EnsureInventoryQuickSlotPanelWidget();
	if (InventoryQuickSlotPanel)
	{
		const bool bExternalPanelOpen =
			ExternalPanelWidget &&
			ExternalPanelWidget->GetExternalPanelMode() != ETunaSweeperHudExternalPanelMode::None;
		const bool bShowInventoryQuickSlotPanel =
			bUtilityModeOpen &&
			bInventoryMode &&
			!bExternalPanelOpen;
		SetTransitionedWidgetVisibility(
			InventoryQuickSlotPanel,
			bShowInventoryQuickSlotPanel
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			InventoryQuickSlotPanelTransitionEdge);
		if (bShowInventoryQuickSlotPanel)
		{
			RefreshInventoryQuickSlotPanel();
		}
	}

	if (ItemInfoPanelWidget && !bInventoryMode)
	{
		SetTransitionedWidgetVisibility(ItemInfoPanelWidget, ESlateVisibility::Collapsed, ItemInfoPanelTransitionEdge);
	}
	else if (ItemInfoPanelWidget && bCraftWorkbenchPanelOpen)
	{
		SetTransitionedWidgetVisibility(ItemInfoPanelWidget, ESlateVisibility::Collapsed, ItemInfoPanelTransitionEdge);
	}

	if (ShopSellPanelWidget && !bInventoryMode)
	{
		SetShopSellPanelVisible(false);
	}

	EnsureHousingPanelWidget();
	if (HousingPanelWidget)
	{
		const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr;
		const bool bShowHousingPanel =
			!bUtilityModeOpen &&
			HousingSubsystem &&
			HousingSubsystem->IsHousingModeOpen() &&
			!IsDialogueSequenceActive();
		SetTransitionedWidgetVisibility(
			HousingPanelWidget,
			bShowHousingPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			ETunaSweeperHudTransitionEdge::Right);
	}

	EnsureMapPanelWidget();
	if (MapPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			MapPanelWidget,
			bUtilityModeOpen && bMapMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed,
			MapPanelTransitionEdge);
		if (bUtilityModeOpen && bMapMode)
		{
			MapPanelWidget->RefreshMapView();
			MapPanelWidget->SetUserFocus(GetOwningPlayer());
		}
	}

	EnsureMemoPanelWidget();
	if (MemoPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			MemoPanelWidget,
			bUtilityModeOpen && bMemoMode ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			MemoPanelTransitionEdge);
		if (bUtilityModeOpen && bMemoMode)
		{
			MemoPanelWidget->RefreshMemoView();
		}
	}

	EnsureQuestPanelWidgets();
	if (MenuQuestPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			MenuQuestPanelWidget,
			bUtilityModeOpen && bQuestMode && !bQuestPanelOpenedFromInteraction
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			QuestPanelTransitionEdge);
		if (bUtilityModeOpen && bQuestMode && !bQuestPanelOpenedFromInteraction)
		{
			MenuQuestPanelWidget->RefreshQuestView();
		}
	}
	if (InteractionQuestPanelWidget)
	{
		SetTransitionedWidgetVisibility(
			InteractionQuestPanelWidget,
			bUtilityModeOpen && bQuestMode && bQuestPanelOpenedFromInteraction
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed,
			QuestPanelTransitionEdge);
		if (bUtilityModeOpen && bQuestMode && bQuestPanelOpenedFromInteraction)
		{
			InteractionQuestPanelWidget->RefreshQuestView();
		}
	}

	if (ExternalPanelWidget)
	{
		const bool bShowExternalPanel =
			bUtilityModeOpen &&
			bInventoryMode &&
			!bClearExternalPanelModeAfterHide &&
			ExternalPanelWidget->GetExternalPanelMode() != ETunaSweeperHudExternalPanelMode::None;
		SetTransitionedWidgetVisibility(
			ExternalPanelWidget,
			bShowExternalPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed,
			ExternalPanelTransitionEdge);
		if (bClearExternalPanelModeAfterHide &&
			!IsSlateVisibilityShown(ExternalPanelWidget->GetVisibility()) &&
			!HasActiveHudTransition(ExternalPanelWidget))
		{
			ExternalPanelWidget->SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::None);
			bClearExternalPanelModeAfterHide = false;
		}
	}

	if (UnsupportedModePanel)
	{
		SetTransitionedWidgetVisibility(
			UnsupportedModePanel,
			bUtilityModeOpen && !bInventoryMode && !bMapMode && !bMemoMode && !bQuestMode
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed,
			UnsupportedModePanelTransitionEdge);
	}

	if (UnsupportedModeText)
	{
		UnsupportedModeText->SetText(ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.common.unimplemented"),
			TEXT("\uBBF8\uAD6C\uD604")));
	}

	if (ModeTitleText)
	{
		const bool bShowModeTitle = bUtilityModeOpen && (bQuestMode || bMemoMode);
		const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
		SetTransitionedWidgetVisibility(
			ModeTitleText,
			bShowModeTitle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed,
			ModeTitleTransitionEdge);
		ModeTitleText->SetText(
			bQuestMode
				? ResolveUiText(TunaGameInstance, TEXT("ui.hud.mode.quest"), TEXT("\uD018\uC2A4\uD2B8"))
				: bMemoMode
					? ResolveUiText(TunaGameInstance, TEXT("ui.hud.mode.memo"), TEXT("\uBA54\uBAA8"))
					: FText::GetEmpty());
	}

	RefreshExtractionProgressWidget();
	RefreshCursorDistanceWidget();
}

void UTunaSweeperGameHudWidget::NormalizeCenterContentPanelLayout()
{
	if (UCanvasPanelSlot* CenterSlot = CenterContentPanel
		? Cast<UCanvasPanelSlot>(CenterContentPanel->Slot)
		: nullptr)
	{
		CenterSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CenterSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CenterSlot->SetOffsets(FMargin(
			UtilityPanelLeftInset,
			UtilityPanelTopOffset,
			UtilityPanelRightInset,
			UtilityPanelBottomInset));
	}

	auto StretchCanvasChildVertically = [](UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, float FallbackWidth)
	{
		UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (!CanvasSlot)
		{
			return;
		}

		const FVector2D CurrentSize = CanvasSlot->GetSize();
		const float Width = CurrentSize.X > 1.0f ? CurrentSize.X : FallbackWidth;
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, Width, 0.0f));
	};

	StretchCanvasChildVertically(
		InventoryAreaWidget,
		FAnchors(0.0f, 0.0f, 0.0f, 1.0f),
		FVector2D(0.0f, 0.0f),
		InventoryAreaPanelWidth);
	StretchCanvasChildVertically(
		ExternalPanelWidget,
		FAnchors(1.0f, 0.0f, 1.0f, 1.0f),
		FVector2D(1.0f, 0.0f),
		ExternalPanelWidth);

	if (UCanvasPanelSlot* ItemInfoSlot = ItemInfoPanelWidget
		? Cast<UCanvasPanelSlot>(ItemInfoPanelWidget->Slot)
		: nullptr)
	{
		float MaxPanelHeight = 620.0f;
		if (UWorld* World = GetWorld())
		{
			FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(World);
			const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(World);
			if (!FMath::IsNearlyZero(ViewportScale))
			{
				ViewportSize /= ViewportScale;
			}
			MaxPanelHeight = FMath::Max(1.0f, ViewportSize.Y - UtilityPanelTopOffset - UtilityPanelBottomInset);
		}

		ItemInfoSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		ItemInfoSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		ItemInfoSlot->SetAutoSize(true);
		ItemInfoSlot->SetOffsets(FMargin(0.0f, 0.0f, ItemInfoPanelWidth, MaxPanelHeight));
		ItemInfoPanelWidget->SetPanelLayoutLimits(ItemInfoPanelWidth, MaxPanelHeight);
	}
}

void UTunaSweeperGameHudWidget::CloseLootContainerPanelIfOpen()
{
	if (!ExternalPanelWidget ||
		ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::None)
	{
		return;
	}

	if (ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::LootingBox)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->NotifyActiveLootContainerUiClosed();
		}
	}
	else if (ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Shop)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearActiveShop();
		}
		SetShopSellPanelVisible(false);
	}
	else if (ExternalPanelWidget->GetExternalPanelMode() == ETunaSweeperHudExternalPanelMode::Workbench)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearActiveWorkbench();
		}
	}

	bClearExternalPanelModeAfterHide = true;
}

void UTunaSweeperGameHudWidget::EnsureExtractionProgressWidget()
{
	if (ExtractionProgressWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	ExtractionProgressWidget = WidgetTree->ConstructWidget<UTunaSweeperExtractionProgressWidget>(
		UTunaSweeperExtractionProgressWidget::StaticClass(),
		TEXT("ExtractionProgressWidget_Runtime"));
	if (!ExtractionProgressWidget)
	{
		return;
	}

	ExtractionProgressWidget->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(ExtractionProgressWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, FMath::Max(0.0f, ExtractionProgressTopOffset)));
		CanvasSlot->SetSize(FVector2D(
			FMath::Max(1.0, ExtractionProgressWidgetSize.X),
			FMath::Max(1.0, ExtractionProgressWidgetSize.Y)));
		CanvasSlot->SetZOrder(45);
	}
}

void UTunaSweeperGameHudWidget::EnsureCursorDistanceWidget()
{
	if (CursorDistancePanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	CursorDistancePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CursorDistancePanel"));
	UHorizontalBox* CursorDistanceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CursorDistanceRow"));
	USizeBox* CursorDistanceIconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CursorDistanceIconBox"));
	UOverlay* CursorDistanceIconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CursorDistanceIconOverlay"));
	USizeBox* CursorDistanceTextBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CursorDistanceTextBox"));
	CursorDistanceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CursorDistanceText"));
	if (!CursorDistancePanel || !CursorDistanceRow || !CursorDistanceIconBox || !CursorDistanceIconOverlay || !CursorDistanceTextBox || !CursorDistanceText)
	{
		return;
	}

	CursorDistancePanel->SetVisibility(ESlateVisibility::Collapsed);
	CursorDistancePanel->SetPadding(FMargin(9.0f, 5.0f, 11.0f, 5.0f));
	CursorDistancePanel->SetBrush(MakeHudRoundedBoxBrush(
		FVector2D(86.0f, 30.0f),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.70f),
		8.0f,
		FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
		0.0f));
	CursorDistancePanel->SetContent(CursorDistanceRow);

	CursorDistanceIconBox->SetWidthOverride(18.0f);
	CursorDistanceIconBox->SetHeightOverride(18.0f);
	CursorDistanceIconBox->SetContent(CursorDistanceIconOverlay);

	auto AddCursorDistanceIconPart = [this](
		UWidgetTree* InWidgetTree,
		UOverlay* IconOverlay,
		const TCHAR* PartName,
		const FVector2D& Size,
		EHorizontalAlignment HorizontalAlignment,
		EVerticalAlignment VerticalAlignment,
		const FMargin& InPadding,
		float Radius)
	{
		USizeBox* PartBox = InWidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(FString::Printf(TEXT("CursorDistance%sBox"), PartName)));
		UBorder* Part = InWidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(FString::Printf(TEXT("CursorDistance%s"), PartName)));
		if (!PartBox || !Part)
		{
			return;
		}

		PartBox->SetWidthOverride(Size.X);
		PartBox->SetHeightOverride(Size.Y);
		Part->SetBrush(MakeHudRoundedBoxBrush(
			Size,
			FLinearColor::White,
			Radius,
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			0.0f));
		PartBox->SetContent(Part);

		if (UOverlaySlot* PartSlot = IconOverlay->AddChildToOverlay(PartBox))
		{
			PartSlot->SetHorizontalAlignment(HorizontalAlignment);
			PartSlot->SetVerticalAlignment(VerticalAlignment);
			PartSlot->SetPadding(InPadding);
		}
	};

	USizeBox* RingBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CursorDistanceReticleRingBox"));
	UBorder* Ring = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CursorDistanceReticleRing"));
	if (RingBox && Ring)
	{
		RingBox->SetWidthOverride(13.0f);
		RingBox->SetHeightOverride(13.0f);
		Ring->SetBrush(MakeHudRoundedBoxBrush(
			FVector2D(13.0f, 13.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			6.5f,
			FLinearColor::White,
			1.4f));
		RingBox->SetContent(Ring);
		if (UOverlaySlot* RingSlot = CursorDistanceIconOverlay->AddChildToOverlay(RingBox))
		{
			RingSlot->SetHorizontalAlignment(HAlign_Center);
			RingSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	AddCursorDistanceIconPart(WidgetTree, CursorDistanceIconOverlay, TEXT("ReticleCenterDot"), FVector2D(3.0f, 3.0f), HAlign_Center, VAlign_Center, FMargin(0.0f), 1.5f);
	AddCursorDistanceIconPart(WidgetTree, CursorDistanceIconOverlay, TEXT("ReticleTopTick"), FVector2D(1.5f, 3.2f), HAlign_Center, VAlign_Top, FMargin(0.0f), 0.75f);
	AddCursorDistanceIconPart(WidgetTree, CursorDistanceIconOverlay, TEXT("ReticleBottomTick"), FVector2D(1.5f, 3.2f), HAlign_Center, VAlign_Bottom, FMargin(0.0f), 0.75f);
	AddCursorDistanceIconPart(WidgetTree, CursorDistanceIconOverlay, TEXT("ReticleLeftTick"), FVector2D(3.2f, 1.5f), HAlign_Left, VAlign_Center, FMargin(0.0f), 0.75f);
	AddCursorDistanceIconPart(WidgetTree, CursorDistanceIconOverlay, TEXT("ReticleRightTick"), FVector2D(3.2f, 1.5f), HAlign_Right, VAlign_Center, FMargin(0.0f), 0.75f);

	if (UHorizontalBoxSlot* IconSlot = CursorDistanceRow->AddChildToHorizontalBox(CursorDistanceIconBox))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	CursorDistanceText->SetText(FText::FromString(TEXT("0M")));
	CursorDistanceText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CursorDistanceText->SetJustification(ETextJustify::Right);
	TunaSweeperUIFont::ApplyFont(CursorDistanceText, 18.0f, ETunaSweeperUIFontWeight::Bold);
	CursorDistanceTextBox->SetWidthOverride(CursorDistanceTextWidth);
	CursorDistanceTextBox->SetContent(CursorDistanceText);
	if (UHorizontalBoxSlot* TextSlot = CursorDistanceRow->AddChildToHorizontalBox(CursorDistanceTextBox))
	{
		TextSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CursorDistancePanel);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(-CursorDistanceRightOffset, -CursorDistanceBottomOffset));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetZOrder(8);
	}
}

void UTunaSweeperGameHudWidget::EnsureDebuffBarWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	auto ConfigureDebuffBarCanvasSlot = [](UCanvasPanelSlot* CanvasSlot)
	{
		if (!CanvasSlot)
		{
			return;
		}

		CanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(DebuffBarLeftOffset, -DebuffBarBottomOffset));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetZOrder(36);
	};

	if (DebuffBarWidget)
	{
		ConfigureDebuffBarCanvasSlot(Cast<UCanvasPanelSlot>(DebuffBarWidget->Slot));
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	TSubclassOf<UTunaSweeperHudDebuffBarWidget> DebuffBarWidgetClass =
		LoadClass<UTunaSweeperHudDebuffBarWidget>(
			nullptr,
			TEXT("/Game/UI/WBP_HudDebuffBar.WBP_HudDebuffBar_C"));
	if (!DebuffBarWidgetClass)
	{
		DebuffBarWidgetClass = UTunaSweeperHudDebuffBarWidget::StaticClass();
	}

	DebuffBarWidget = CreateWidget<UTunaSweeperHudDebuffBarWidget>(
		GetOwningPlayer(),
		DebuffBarWidgetClass);
	if (!DebuffBarWidget)
	{
		return;
	}

	DebuffBarWidget->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(DebuffBarWidget);
	ConfigureDebuffBarCanvasSlot(CanvasSlot);
}

void UTunaSweeperGameHudWidget::EnsureInventoryWeightPanelWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	auto ConfigureWeightPanelCanvasSlot = [](UCanvasPanelSlot* CanvasSlot)
	{
		if (!CanvasSlot)
		{
			return;
		}

		CanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(UtilityPanelLeftInset, -InventoryWeightPanelBottomOffset));
		CanvasSlot->SetSize(FVector2D(InventoryWeightPanelWidth, InventoryWeightPanelHeight));
		CanvasSlot->SetZOrder(37);
	};

	if (InventoryWeightPanel)
	{
		ConfigureWeightPanelCanvasSlot(Cast<UCanvasPanelSlot>(InventoryWeightPanel->Slot));
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	InventoryWeightPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryWeightPanel"));
	UHorizontalBox* WeightRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryWeightRow"));
	InventoryWeightLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightLabelText"));
	USizeBox* WeightGaugeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventoryWeightGaugeBox"));
	InventoryWeightGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("InventoryWeightGauge"));
	InventoryWeightText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightText"));
	InventoryWeightWarningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightWarningText"));
	if (!InventoryWeightPanel || !WeightRow || !InventoryWeightLabelText || !WeightGaugeBox ||
		!InventoryWeightGauge || !InventoryWeightText || !InventoryWeightWarningText)
	{
		return;
	}

	InventoryWeightPanel->SetVisibility(ESlateVisibility::Collapsed);
	InventoryWeightPanel->SetPadding(FMargin(10.0f, 6.0f));
	InventoryWeightPanel->SetBrush(MakeHudRoundedBoxBrush(
		FVector2D(InventoryWeightPanelWidth, InventoryWeightPanelHeight),
		FLinearColor(0.005f, 0.008f, 0.010f, 0.76f),
		1.0f,
		FLinearColor(0.18f, 0.24f, 0.26f, 0.85f),
		1.0f));
	InventoryWeightPanel->SetContent(WeightRow);

	InventoryWeightLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.94f, 1.0f)));
	InventoryWeightLabelText->SetJustification(ETextJustify::Left);
	TunaSweeperUIFont::ApplyFont(InventoryWeightLabelText, 13.0f, ETunaSweeperUIFontWeight::Bold);
	UHorizontalBoxSlot* LabelSlot = WeightRow->AddChildToHorizontalBox(InventoryWeightLabelText);
	if (LabelSlot)
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	InventoryWeightGauge->SetPercent(0.0f);
	InventoryWeightGauge->SetFillColorAndOpacity(FLinearColor(0.60f, 0.84f, 0.36f, 1.0f));
	WeightGaugeBox->SetHeightOverride(16.0f);
	WeightGaugeBox->SetContent(InventoryWeightGauge);
	UHorizontalBoxSlot* GaugeSlot = WeightRow->AddChildToHorizontalBox(WeightGaugeBox);
	if (GaugeSlot)
	{
		GaugeSlot->SetPadding(FMargin(10.0f, 0.0f));
		GaugeSlot->SetVerticalAlignment(VAlign_Center);
		GaugeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	InventoryWeightText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	InventoryWeightText->SetJustification(ETextJustify::Right);
	TunaSweeperUIFont::ApplyFont(InventoryWeightText, 13.0f, ETunaSweeperUIFontWeight::Bold);
	UHorizontalBoxSlot* WeightTextSlot = WeightRow->AddChildToHorizontalBox(InventoryWeightText);
	if (WeightTextSlot)
	{
		WeightTextSlot->SetVerticalAlignment(VAlign_Center);
		WeightTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	InventoryWeightWarningText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.50f, 1.0f)));
	InventoryWeightWarningText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(InventoryWeightWarningText, 11.0f, ETunaSweeperUIFontWeight::Bold);
	UHorizontalBoxSlot* WarningSlot = WeightRow->AddChildToHorizontalBox(InventoryWeightWarningText);
	if (WarningSlot)
	{
		WarningSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		WarningSlot->SetVerticalAlignment(VAlign_Center);
		WarningSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(InventoryWeightPanel);
	ConfigureWeightPanelCanvasSlot(CanvasSlot);
}

void UTunaSweeperGameHudWidget::EnsureInventoryQuickSlotPanelWidget()
{
	if (InventoryQuickSlotPanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	TSubclassOf<UTunaSweeperItemThumbnailSlotWidget> EntryWidgetClass =
		LoadClass<UTunaSweeperItemThumbnailSlotWidget>(
			nullptr,
			TEXT("/Game/UI/WBP_ItemThumbnailSlot.WBP_ItemThumbnailSlot_C"));
	if (!EntryWidgetClass)
	{
		return;
	}

	InventoryQuickSlotPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryQuickSlotPanel"));
	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryQuickSlotStack"));
	UTextBlock* GuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryQuickSlotGuideText"));
	InventoryQuickSlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryQuickSlotRow"));
	if (!InventoryQuickSlotPanel || !PanelStack || !GuideText || !InventoryQuickSlotRow)
	{
		return;
	}

	InventoryQuickSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	InventoryQuickSlotPanel->SetPadding(FMargin(18.0f, 12.0f, 18.0f, 12.0f));
	InventoryQuickSlotPanel->SetBrush(MakeHudRoundedBoxBrush(
		FVector2D(InventoryQuickSlotPanelWidth, InventoryQuickSlotPanelHeight),
		FLinearColor(0.015f, 0.018f, 0.018f, 0.68f),
		8.0f,
		FLinearColor(0.48f, 0.54f, 0.52f, 0.48f),
		1.0f));
	InventoryQuickSlotPanel->SetContent(PanelStack);

	InventoryQuickSlotGuideText = GuideText;
	GuideText->SetText(ResolveUiText(
		GetGameInstance<UTunaSweeperGameInstance>(),
		TEXT("ui.hud.quick_slot_guide"),
		TEXT("\uC544\uC774\uD15C\uC744 \uC2AC\uB86F\uC73C\uB85C \uB4DC\uB798\uADF8\uD558\uC5EC \uD035\uC2AC\uB86F\uC744 \uC124\uC815\uD558\uC138\uC694")));
	GuideText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.94f, 0.90f, 1.0f)));
	GuideText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(GuideText, 24, ETunaSweeperUIFontWeight::Bold);
	UVerticalBoxSlot* GuideSlot = PanelStack->AddChildToVerticalBox(GuideText);
	if (GuideSlot)
	{
		GuideSlot->SetHorizontalAlignment(HAlign_Fill);
		GuideSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UVerticalBoxSlot* RowStackSlot = PanelStack->AddChildToVerticalBox(InventoryQuickSlotRow);
	if (RowStackSlot)
	{
		RowStackSlot->SetHorizontalAlignment(HAlign_Center);
	}

	InventoryQuickSlotWidgets.Reset();
	for (int32 SlotNumber = InventoryQuickSlotFirstNumber; SlotNumber <= InventoryQuickSlotLastNumber; ++SlotNumber)
	{
		UVerticalBox* SlotStack = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dStack"), SlotNumber)));
		USizeBox* SlotSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dSizeBox"), SlotNumber)));
		UTunaSweeperItemThumbnailSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UTunaSweeperItemThumbnailSlotWidget>(
			EntryWidgetClass,
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dWidget"), SlotNumber)));
		USizeBox* KeySizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeySizeBox"), SlotNumber)));
		UBorder* KeyBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeyBackground"), SlotNumber)));
		UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeyText"), SlotNumber)));
		if (!SlotStack || !SlotSizeBox || !SlotWidget || !KeySizeBox || !KeyBackground || !KeyText)
		{
			continue;
		}

		SlotSizeBox->SetWidthOverride(InventoryQuickSlotTileSize);
		SlotSizeBox->SetHeightOverride(InventoryQuickSlotTileSize);
		SlotWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SlotWidget->SetRenderScale(FVector2D(InventoryQuickSlotTileScale, InventoryQuickSlotTileScale));
		SlotSizeBox->SetContent(SlotWidget);
		UVerticalBoxSlot* SlotWidgetStackSlot = SlotStack->AddChildToVerticalBox(SlotSizeBox);
		if (SlotWidgetStackSlot)
		{
			SlotWidgetStackSlot->SetHorizontalAlignment(HAlign_Center);
		}

		KeySizeBox->SetWidthOverride(34.0f);
		KeySizeBox->SetHeightOverride(28.0f);
		KeyBackground->SetPadding(FMargin(8.0f, 1.0f));
		KeyBackground->SetBrush(MakeHudRoundedBoxBrush(
			FVector2D(34.0f, 28.0f),
			FLinearColor(0.96f, 0.96f, 0.96f, 0.98f),
			5.0f,
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			0.0f));
		KeyText->SetText(FText::AsNumber(SlotNumber));
		KeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.024f, 0.028f, 1.0f)));
		KeyText->SetJustification(ETextJustify::Center);
		TunaSweeperUIFont::ApplyFont(KeyText, 17, ETunaSweeperUIFontWeight::Bold);
		KeyBackground->SetContent(KeyText);
		KeySizeBox->SetContent(KeyBackground);
		UVerticalBoxSlot* KeyStackSlot = SlotStack->AddChildToVerticalBox(KeySizeBox);
		if (KeyStackSlot)
		{
			KeyStackSlot->SetHorizontalAlignment(HAlign_Center);
			KeyStackSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
		}

		UHorizontalBoxSlot* RowSlot = InventoryQuickSlotRow->AddChildToHorizontalBox(SlotStack);
		if (RowSlot)
		{
			RowSlot->SetPadding(FMargin(SlotNumber == InventoryQuickSlotFirstNumber ? 0.0f : 12.0f, 0.0f, 0.0f, 0.0f));
			RowSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		InventoryQuickSlotWidgets.Add(SlotWidget);
	}

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(InventoryQuickSlotPanel);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, -34.0f));
		CanvasSlot->SetSize(FVector2D(InventoryQuickSlotPanelWidth, InventoryQuickSlotPanelHeight));
		CanvasSlot->SetZOrder(35);
	}
}

void UTunaSweeperGameHudWidget::EnsureHousingPanelWidget()
{
	if (HousingPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	HousingPanelWidget = CreateWidget<UTunaSweeperHousingPanelWidget>(
		GetOwningPlayer(),
		UTunaSweeperHousingPanelWidget::StaticClass());
	if (!HousingPanelWidget)
	{
		return;
	}

	HousingPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(HousingPanelWidget);
	if (CanvasSlot)
	{
		constexpr float PanelWidth = 360.0f;
		constexpr float EdgeMargin = 24.0f;
		CanvasSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetOffsets(FMargin(-(PanelWidth + EdgeMargin), EdgeMargin, PanelWidth, EdgeMargin));
		CanvasSlot->SetZOrder(42);
	}
}

void UTunaSweeperGameHudWidget::EnsureHousingFacilityContextMenuWidget()
{
	if (HousingContextMenuPanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	HousingContextMenuPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("HousingFacilityContextMenu"));
	UVerticalBox* MenuStack = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("HousingFacilityContextMenuStack"));
	HousingContextStoreButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("HousingFacilityContextStoreButton"));
	HousingContextStoreText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("HousingFacilityContextStoreText"));
	if (!HousingContextMenuPanel || !MenuStack || !HousingContextStoreButton || !HousingContextStoreText)
	{
		return;
	}

	HousingContextMenuPanel->SetPadding(FMargin(5.0f));
	HousingContextMenuPanel->SetBrush(MakeHudRoundedBoxBrush(
		FVector2D(132.0f, 42.0f),
		FLinearColor(0.018f, 0.024f, 0.028f, 0.96f),
		6.0f,
		FLinearColor(0.42f, 0.82f, 0.94f, 0.60f),
		1.0f));
	HousingContextMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	HousingContextMenuPanel->SetContent(MenuStack);

	HousingContextStoreText->SetJustification(ETextJustify::Center);
	HousingContextStoreText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.98f, 1.0f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(HousingContextStoreText, 14, ETunaSweeperUIFontWeight::Bold);

	HousingContextStoreButton->SetContent(HousingContextStoreText);
	HousingContextStoreButton->SetBackgroundColor(FLinearColor(0.08f, 0.22f, 0.25f, 0.96f));
	HousingContextStoreButton->OnClicked.RemoveDynamic(this, &UTunaSweeperGameHudWidget::HandleHousingContextStoreClicked);
	HousingContextStoreButton->OnClicked.AddDynamic(this, &UTunaSweeperGameHudWidget::HandleHousingContextStoreClicked);
	if (UVerticalBoxSlot* ButtonSlot = MenuStack->AddChildToVerticalBox(HousingContextStoreButton))
	{
		ButtonSlot->SetPadding(FMargin(0.0f));
	}

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(HousingContextMenuPanel);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
		CanvasSlot->SetSize(FVector2D(132.0f, 42.0f));
		CanvasSlot->SetZOrder(60);
	}
}

void UTunaSweeperGameHudWidget::EnsureMapPanelWidget()
{
	if (MapPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	MapPanelWidget = CreateWidget<UTunaSweeperMapWidget>(
		GetOwningPlayer(),
		UTunaSweeperMapWidget::StaticClass());
	if (!MapPanelWidget)
	{
		return;
	}

	MapPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MapPanelWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetZOrder(-5);
	}
}

void UTunaSweeperGameHudWidget::EnsureMemoPanelWidget()
{
	if (MemoPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	MemoPanelWidget = CreateWidget<UTunaSweeperMemoWidget>(
		GetOwningPlayer(),
		UTunaSweeperMemoWidget::StaticClass());
	if (!MemoPanelWidget)
	{
		return;
	}

	MemoPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MemoPanelWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 34.0f));
		CanvasSlot->SetSize(FVector2D(1220.0f, 672.0f));
		CanvasSlot->SetZOrder(20);
	}
}

void UTunaSweeperGameHudWidget::EnsureQuestPanelWidgets()
{
	if ((MenuQuestPanelWidget && InteractionQuestPanelWidget) || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	auto ResolveQuestWidgetClass = [](const TCHAR* WidgetClassPath, UClass* FallbackClass)
	{
		TSoftClassPtr<UTunaSweeperQuestWidget> SoftWidgetClass{ FSoftObjectPath(WidgetClassPath) };
		TSubclassOf<UTunaSweeperQuestWidget> LoadedClass = SoftWidgetClass.LoadSynchronous();
		return LoadedClass ? LoadedClass : TSubclassOf<UTunaSweeperQuestWidget>(FallbackClass);
	};

	auto AddQuestWidgetToCanvas = [this, RootCanvas](
		TObjectPtr<UTunaSweeperQuestWidget>& OutWidget,
		TSubclassOf<UTunaSweeperQuestWidget> WidgetClass,
		const FMargin& Margins)
	{
		if (OutWidget || !WidgetClass)
		{
			return;
		}

		OutWidget = CreateWidget<UTunaSweeperQuestWidget>(GetOwningPlayer(), WidgetClass);
		if (!OutWidget)
		{
			return;
		}

		OutWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(OutWidget);
		if (CanvasSlot)
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetOffsets(Margins);
			CanvasSlot->SetZOrder(20);
		}
	};

	AddQuestWidgetToCanvas(
		MenuQuestPanelWidget,
		ResolveQuestWidgetClass(
			TEXT("/Game/UI/WBP_QuestMenu.WBP_QuestMenu_C"),
			UTunaSweeperMenuQuestWidget::StaticClass()),
		FMargin(
			QuestMenuHorizontalMargin,
			QuestMenuTopMargin,
			QuestMenuHorizontalMargin,
			QuestMenuBottomMargin));
	AddQuestWidgetToCanvas(
		InteractionQuestPanelWidget,
		ResolveQuestWidgetClass(
			TEXT("/Game/UI/WBP_QuestInteraction.WBP_QuestInteraction_C"),
			UTunaSweeperInteractionQuestWidget::StaticClass()),
		FMargin(
			QuestInteractionHorizontalMargin,
			QuestInteractionTopMargin,
			QuestInteractionHorizontalMargin,
			QuestInteractionBottomMargin));
}

void UTunaSweeperGameHudWidget::EnsureShopSellPanelWidget()
{
	if (ShopSellPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* ParentCanvas = ItemInfoPanelWidget
		? Cast<UCanvasPanel>(ItemInfoPanelWidget->GetParent())
		: nullptr;
	if (!ParentCanvas)
	{
		ParentCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	if (!ParentCanvas)
	{
		return;
	}

	ShopSellPanelWidget = CreateWidget<UTunaSweeperShopSellPanelWidget>(
		GetOwningPlayer(),
		UTunaSweeperShopSellPanelWidget::StaticClass());
	if (!ShopSellPanelWidget)
	{
		return;
	}

	ShopSellPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = ParentCanvas->AddChildToCanvas(ShopSellPanelWidget);
	if (!CanvasSlot)
	{
		return;
	}

	if (const UCanvasPanelSlot* ItemInfoCanvasSlot = ItemInfoPanelWidget
		? Cast<UCanvasPanelSlot>(ItemInfoPanelWidget->Slot)
		: nullptr)
	{
		CanvasSlot->SetAnchors(ItemInfoCanvasSlot->GetAnchors());
		CanvasSlot->SetAlignment(ItemInfoCanvasSlot->GetAlignment());
		CanvasSlot->SetPosition(ItemInfoCanvasSlot->GetPosition());
		CanvasSlot->SetSize(FVector2D(ShopSellPanelWidth, ShopSellPanelHeight));
		CanvasSlot->SetZOrder(ItemInfoCanvasSlot->GetZOrder() + 1);
		return;
	}

	CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CanvasSlot->SetPosition(FVector2D(0.0f, -24.0f));
	CanvasSlot->SetSize(FVector2D(ShopSellPanelWidth, ShopSellPanelHeight));
	CanvasSlot->SetZOrder(31);
}

void UTunaSweeperGameHudWidget::SetShopSellPanelVisible(bool bVisible)
{
	EnsureShopSellPanelWidget();

	if (!ShopSellPanelWidget)
	{
		return;
	}

	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ShopSellPanelWidget->RefreshSelectedItem();
	}

	SetTransitionedWidgetVisibilityFromTranslation(
		ShopSellPanelWidget,
		bVisible && ActiveHudMode == ETunaSweeperHudMode::Inventory
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed,
		FVector2D(0.0f, ShopSellPanelTransitionOffsetY));
}

