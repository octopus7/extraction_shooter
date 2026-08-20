#include "TunaSweeperGameHudWidgetShared.h"

void UTunaSweeperGameHudWidget::RefreshBottomStatusFromGameInstance()
{
	if (!BottomStatusWidget && !InventoryAreaWidget && !InventoryWeightPanel)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperPlayerHudState HudState = TunaGameInstance ? TunaGameInstance->PlayerHudState : FTunaSweeperPlayerHudState();
	const UTunaSweeperScratchComponent* ScratchComponent = nullptr;

	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		const APawn* Pawn = PlayerController->GetPawn();
		const UTunaSweeperVitalsComponent* VitalsComponent = nullptr;
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(Pawn))
		{
			VitalsComponent = TunaCharacter->GetVitalsComponent();
			ScratchComponent = TunaCharacter->GetScratchComponent();
		}
		else if (Pawn)
		{
			VitalsComponent = Pawn->FindComponentByClass<UTunaSweeperVitalsComponent>();
		}

		if (VitalsComponent)
		{
			const FTunaSweeperVitalsState& VitalsState = VitalsComponent->GetVitalsState();
			HudState.Health = VitalsState.Health;
			HudState.MaxHealth = VitalsState.MaxHealth;
			HudState.Food = VitalsState.Food;
			HudState.MaxFood = VitalsState.MaxFood;
			HudState.Hydration = VitalsState.Hydration;
			HudState.MaxHydration = VitalsState.MaxHydration;
		}
	}

	if (BottomStatusWidget)
	{
		BottomStatusWidget->SetHudState(HudState);
		BottomStatusWidget->SetScratchState(
			ScratchComponent ? ScratchComponent->GetCurrentScratch() : 0.0f,
			ScratchComponent ? ScratchComponent->GetMaxScratch() : 100.0f);
	}

	if (InventoryAreaWidget)
	{
		InventoryAreaWidget->SetHudState(HudState);
	}

	RefreshInventoryWeightPanelFromHudState(HudState);
}

void UTunaSweeperGameHudWidget::RefreshInventoryWeightPanelFromHudState(const FTunaSweeperPlayerHudState& HudState)
{
	EnsureInventoryWeightPanelWidget();
	if (!InventoryWeightPanel)
	{
		return;
	}

	FTunaSweeperPlayerHudState NormalizedHudState = HudState;
	NormalizedHudState.NormalizeWeightLimits();
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();

	FNumberFormattingOptions NumberFormat;
	NumberFormat.MinimumFractionalDigits = 0;
	NumberFormat.MaximumFractionalDigits = 1;
	auto MakeWeightNumberText = [&NumberFormat](float Value)
	{
		return FText::AsNumber(Value, &NumberFormat);
	};

	if (InventoryWeightLabelText)
	{
		InventoryWeightLabelText->SetText(ResolveUiText(
			TunaGameInstance,
			TEXT("ui.inventory.weight_label"),
			TEXT("\uC18C\uC9C0 \uC911\uB7C9")));
	}

	if (InventoryWeightText)
	{
		InventoryWeightText->SetText(FText::Format(
			ResolveUiText(TunaGameInstance, TEXT("ui.inventory.weight_pattern"), TEXT("{0}/{1}kg")),
			MakeWeightNumberText(NormalizedHudState.CurrentCarryWeight),
			MakeWeightNumberText(NormalizedHudState.MaxCarryWeight)));
	}

	if (InventoryWeightGauge)
	{
		const float GaugePercent = NormalizedHudState.MovementBlockedWeight > 0.0f
			? NormalizedHudState.CurrentCarryWeight / NormalizedHudState.MovementBlockedWeight
			: 0.0f;
		InventoryWeightGauge->SetPercent(FMath::Clamp(GaugePercent, 0.0f, 1.0f));
		InventoryWeightGauge->SetFillColorAndOpacity(
			NormalizedHudState.IsCarryWeightMovementBlocked()
				? FLinearColor(0.92f, 0.16f, 0.10f, 1.0f)
				: NormalizedHudState.IsCarryWeightOverLimit()
					? FLinearColor(0.96f, 0.74f, 0.18f, 1.0f)
					: FLinearColor(0.60f, 0.84f, 0.36f, 1.0f));
	}

	if (InventoryWeightWarningText)
	{
		InventoryWeightWarningText->SetText(ResolveUiText(
			TunaGameInstance,
			TEXT("ui.inventory.overweight"),
			TEXT("\uACFC\uC911\uB7C9")));
		InventoryWeightWarningText->SetVisibility(
			NormalizedHudState.IsCarryWeightOverLimit()
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::RefreshDebuffBarFromPlayer()
{
	EnsureDebuffBarWidget();
	bDebuffBarHasActiveDebuffs = false;
	if (!DebuffBarWidget)
	{
		return;
	}

	TArray<FTunaSweeperActiveDebuffState> ActiveDebuffs;
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UTunaSweeperDebuffComponent* DebuffComponent = PlayerPawn
		? PlayerPawn->FindComponentByClass<UTunaSweeperDebuffComponent>()
		: nullptr;
	if (DebuffComponent)
	{
		ActiveDebuffs = DebuffComponent->GetActiveDebuffs();
		bDebuffBarHasActiveDebuffs = ActiveDebuffs.Num() > 0;
	}

	DebuffBarWidget->SetActiveDebuffs(ActiveDebuffs);
}

void UTunaSweeperGameHudWidget::RefreshQuickSlotsFromGameState()
{
	if (!QuickSlotBarWidget)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	int32 SelectedSlotNumber = 0;
	bool bMeleeQuickSlotSelected = false;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn()))
		{
			SelectedSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
			bMeleeQuickSlotSelected = TunaCharacter->IsMeleeWeaponSelected();
		}
	}
	if (bMeleeQuickSlotSelected)
	{
		QuickSlotBarWidget->SetSelectedMeleeQuickSlot();
	}
	else
	{
		QuickSlotBarWidget->SetSelectedQuickSlot(SelectedSlotNumber);
	}

	for (int32 SlotNumber = 1; SlotNumber <= 2; ++SlotNumber)
	{
		FTunaSweeperItemInstance WeaponInstance;
		FTunaSweeperItemDefinition WeaponDefinition;
		if (!TunaGameInstance ||
			!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SlotNumber, WeaponInstance, WeaponDefinition))
		{
			QuickSlotBarWidget->ClearQuickSlotIcon(SlotNumber);
			QuickSlotBarWidget->SetQuickSlotItemGrade(SlotNumber, ETunaSweeperItemGrade::Common, false);
			QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
			QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
			continue;
		}

		UTexture2D* IconTexture = nullptr;
		if (ItemDataSubsystem)
		{
			const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(WeaponDefinition);
			if (!IconObjectPath.IsEmpty())
			{
				IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
			}
		}
		QuickSlotBarWidget->SetQuickSlotIcon(SlotNumber, IconTexture);
		QuickSlotBarWidget->SetQuickSlotItemGrade(SlotNumber, WeaponDefinition.ItemGrade, true);

		FText AmmoTypeText = ResolveUiText(TunaGameInstance, TEXT("ui.hud.ammo_unset"), TEXT("\uD0C4\uC57D \uBBF8\uC9C0\uC815"));
		const int32 AmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SlotNumber);
		if (AmmoItemId != INDEX_NONE)
		{
			AmmoTypeText = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(AmmoItemId));
			if (ItemDataSubsystem)
			{
				ItemDataSubsystem->TryGetItemNameText(AmmoItemId, Language, AmmoTypeText);
			}
		}
		QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, AmmoTypeText, true);
		QuickSlotBarWidget->SetWeaponAmmoText(
			SlotNumber,
			TunaGameInstance->GetWeaponLoadedAmmoCount(SlotNumber),
			TunaGameInstance->GetWeaponInventoryAmmoCount(SlotNumber),
			true);
	}

	FTunaSweeperItemInstance MeleeInstance;
	FTunaSweeperItemDefinition MeleeDefinition;
	if (!TunaGameInstance ||
		!TunaGameInstance->TryGetEquipmentMeleeSlotItem(MeleeInstance, MeleeDefinition) ||
		!ItemDataSubsystem)
	{
		QuickSlotBarWidget->ClearMeleeQuickSlotIcon();
		QuickSlotBarWidget->SetMeleeQuickSlotItemGrade(ETunaSweeperItemGrade::Common, false);
	}
	else
	{
		UTexture2D* IconTexture = nullptr;
		const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(MeleeDefinition);
		if (!IconObjectPath.IsEmpty())
		{
			IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
		}
		QuickSlotBarWidget->SetMeleeQuickSlotIcon(IconTexture);
		QuickSlotBarWidget->SetMeleeQuickSlotItemGrade(MeleeDefinition.ItemGrade, true);
	}

	static const TArray<FTunaSweeperInventorySlot> EmptyQuickSlots;
	const TArray<FTunaSweeperInventorySlot>& UsableQuickSlots = TunaGameInstance
		? TunaGameInstance->GetUsableQuickSlots()
		: EmptyQuickSlots;
	for (int32 SlotNumber = InventoryQuickSlotFirstNumber; SlotNumber <= InventoryQuickSlotLastNumber; ++SlotNumber)
	{
		const int32 SlotIndex = SlotNumber - InventoryQuickSlotFirstNumber;
		FTunaSweeperItemInstance ItemInstance;
		FTunaSweeperItemDefinition ItemDefinition;
		if (!UsableQuickSlots.IsValidIndex(SlotIndex) ||
			!TunaGameInstance ||
			!TunaGameInstance->TryGetItemInstance(UsableQuickSlots[SlotIndex].ItemUid, ItemInstance) ||
			!ItemDataSubsystem ||
			!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			QuickSlotBarWidget->ClearQuickSlotIcon(SlotNumber);
			QuickSlotBarWidget->SetQuickSlotItemGrade(SlotNumber, ETunaSweeperItemGrade::Common, false);
			QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
			QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
			continue;
		}

		UTexture2D* IconTexture = nullptr;
		const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
		if (!IconObjectPath.IsEmpty())
		{
			IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
		}

		QuickSlotBarWidget->SetQuickSlotIcon(SlotNumber, IconTexture);
		QuickSlotBarWidget->SetQuickSlotItemGrade(SlotNumber, ItemDefinition.ItemGrade, true);
		QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
		QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
	}
}

void UTunaSweeperGameHudWidget::RefreshInventoryQuickSlotPanel()
{
	if (InventoryQuickSlotWidgets.Num() <= 0)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	static const TArray<FTunaSweeperInventorySlot> EmptyQuickSlots;
	const TArray<FTunaSweeperInventorySlot>& UsableQuickSlots = TunaGameInstance
		? TunaGameInstance->GetUsableQuickSlots()
		: EmptyQuickSlots;

	for (int32 SlotIndex = 0; SlotIndex < InventoryQuickSlotWidgets.Num(); ++SlotIndex)
	{
		if (!InventoryQuickSlotWidgets[SlotIndex])
		{
			continue;
		}

		const FTunaSweeperInventorySlot& QuickSlot = UsableQuickSlots.IsValidIndex(SlotIndex)
			? UsableQuickSlots[SlotIndex]
			: FTunaSweeperInventorySlot();
		InventoryQuickSlotWidgets[SlotIndex]->SetTileData(BuildQuickSlotTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			QuickSlot,
			SlotIndex));
	}
}

void UTunaSweeperGameHudWidget::RefreshLocalizedTexts()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (InventoryQuickSlotGuideText)
	{
		InventoryQuickSlotGuideText->SetText(ResolveUiText(
			TunaGameInstance,
			TEXT("ui.hud.quick_slot_guide"),
			TEXT("\uC544\uC774\uD15C\uC744 \uC2AC\uB86F\uC73C\uB85C \uB4DC\uB798\uADF8\uD558\uC5EC \uD035\uC2AC\uB86F\uC744 \uC124\uC815\uD558\uC138\uC694")));
	}

	if (CenterReloadPromptText)
	{
		CenterReloadPromptText->SetText(ResolveUiText(
			TunaGameInstance,
			TEXT("ui.hud.reload_prompt"),
			TEXT("\uC7AC\uC7A5\uC804")));
	}

	if (UnsupportedModeText)
	{
		UnsupportedModeText->SetText(ResolveUiText(
			TunaGameInstance,
			TEXT("ui.common.unimplemented"),
			TEXT("\uBBF8\uAD6C\uD604")));
	}
}

void UTunaSweeperGameHudWidget::RefreshCancelableActionWidgets(const FGeometry* GeometryForPlacement)
{
	CacheAmmoCancelableActionWidgets();

	const bool bDialogueActive = IsDialogueSequenceActive();
	const bool bHousingModeActive = IsHousingModeActive();
	ATunaSweeperTopDownCharacter* TunaCharacter = nullptr;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn());
	}

	const bool bShowCancelableAction = !bDialogueActive && !bHousingModeActive && TunaCharacter && TunaCharacter->IsCancelableActionActive();
	const float CancelableActionProgress = bShowCancelableAction ? TunaCharacter->GetCancelableActionProgress() : 0.0f;
	const bool bUseCrosshairReloadGauge = bShowCancelableAction && IsReloadGaugeReplacingCrosshair(TunaCharacter);
	if (GeometryForPlacement)
	{
		UpdateCenterCancelableActionGaugePlacement(*GeometryForPlacement, bUseCrosshairReloadGauge);
	}
	UpdateMouseCursorForReloadGauge(bUseCrosshairReloadGauge);

	bool bShowReloadPrompt = false;
	if (!bDialogueActive && !bHousingModeActive && !bShowCancelableAction && TunaCharacter && !TunaCharacter->IsAmmoSelectionOpen() && !IsInventoryUiOpen())
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			const int32 SelectedWeaponSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
			bShowReloadPrompt =
				SelectedWeaponSlotNumber > 0 &&
				TunaGameInstance->IsEquipmentWeaponSlotOccupied(SelectedWeaponSlotNumber) &&
				TunaGameInstance->GetWeaponMagazineCapacity(SelectedWeaponSlotNumber) > 0 &&
				TunaGameInstance->GetWeaponLoadedAmmoCount(SelectedWeaponSlotNumber) <= 0 &&
				TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber) != INDEX_NONE &&
				TunaGameInstance->GetWeaponInventoryAmmoCount(SelectedWeaponSlotNumber) > 0;
		}
	}

	if (QuickSlotBarWidget)
	{
		QuickSlotBarWidget->SetCancelableActionProgress(CancelableActionProgress, bShowCancelableAction);
	}

	if (CenterCancelableActionGaugeRoot)
	{
		CenterCancelableActionGaugeRoot->SetVisibility(bShowCancelableAction ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterReloadPromptRoot)
	{
		CenterReloadPromptRoot->SetVisibility(bShowReloadPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterCancelableActionPercentText)
	{
		CenterCancelableActionPercentText->SetText(
			bShowCancelableAction
				? FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CancelableActionProgress * 100.0f)))
				: FText::GetEmpty());
	}

	if (CenterCancelableActionRingWidget)
	{
		CenterCancelableActionRingWidget->SetCancelableActionProgress(CancelableActionProgress, bShowCancelableAction);
	}

	const int32 FilledSegmentCount = FMath::CeilToInt(CancelableActionProgress * CenterCancelableActionSegments.Num());
	for (int32 SegmentIndex = 0; SegmentIndex < CenterCancelableActionSegments.Num(); ++SegmentIndex)
	{
		if (CenterCancelableActionSegments[SegmentIndex])
		{
			CenterCancelableActionSegments[SegmentIndex]->SetRenderOpacity(
				bShowCancelableAction && SegmentIndex < FilledSegmentCount ? 1.0f : 0.18f);
		}
	}

	TArray<FText> AmmoOptionTexts;
	int32 FocusedOptionIndex = INDEX_NONE;
	BuildAmmoSelectorOptionTexts(AmmoOptionTexts, FocusedOptionIndex);
	if (QuickSlotBarWidget)
	{
		const int32 SelectedWeaponSlotNumber = TunaCharacter ? TunaCharacter->GetSelectedWeaponSlotNumber() : 0;
		const bool bAmmoSelectionOpen = TunaCharacter && TunaCharacter->IsAmmoSelectionOpen();
		if (bAmmoSelectionOpen)
		{
			QuickSlotBarWidget->SetAmmoSelectorOptions(
				AmmoOptionTexts,
				FocusedOptionIndex,
				SelectedWeaponSlotNumber,
				true);
		}
		else
		{
			QuickSlotBarWidget->SetAmmoSelectorPrompt(SelectedWeaponSlotNumber, FText::GetEmpty(), false);
		}
	}
}

void UTunaSweeperGameHudWidget::CacheAmmoCancelableActionWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	CenterCancelableActionGaugeRoot = WidgetTree->FindWidget(FName(TEXT("CenterCancelableActionGaugeRoot")));
	if (!CenterCancelableActionGaugeRoot)
	{
		CenterCancelableActionGaugeRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadGaugeRoot")));
	}
	CenterCancelableActionRingWidget = Cast<UTunaSweeperReloadRingWidget>(WidgetTree->FindWidget(FName(TEXT("CenterCancelableActionRingWidget"))));
	if (!CenterCancelableActionRingWidget)
	{
		CenterCancelableActionRingWidget = Cast<UTunaSweeperReloadRingWidget>(WidgetTree->FindWidget(FName(TEXT("CenterReloadRingWidget"))));
	}
	CenterReloadPromptRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadPromptRoot")));
	CenterReloadPromptText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CenterReloadPromptText"))));
	CenterCancelableActionPercentText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CenterCancelableActionPercentText"))));
	if (!CenterCancelableActionPercentText)
	{
		CenterCancelableActionPercentText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CenterReloadPercentText"))));
	}
	CenterCancelableActionSegments.SetNum(12);
	for (int32 SegmentNumber = 1; SegmentNumber <= CenterCancelableActionSegments.Num(); ++SegmentNumber)
	{
		UBorder* Segment = Cast<UBorder>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("CenterCancelableActionSegment%02d"), SegmentNumber))));
		if (!Segment)
		{
			Segment = Cast<UBorder>(WidgetTree->FindWidget(
				FName(*FString::Printf(TEXT("CenterReloadSegment%02d"), SegmentNumber))));
		}
		CenterCancelableActionSegments[SegmentNumber - 1] = Segment;
		if (!Segment)
		{
			continue;
		}

		const float AngleDegrees = (SegmentNumber - 1) * 30.0f - 90.0f;
		const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
		const FVector2D SegmentPosition(
			FMath::Cos(AngleRadians) * 36.0f,
			FMath::Sin(AngleRadians) * 36.0f);
		if (UCanvasPanelSlot* SegmentSlot = Cast<UCanvasPanelSlot>(Segment->Slot))
		{
			SegmentSlot->SetPosition(SegmentPosition);
			SegmentSlot->SetSize(FVector2D(12.0f, 5.0f));
		}
		Segment->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Segment->SetRenderTransformAngle(AngleDegrees + 90.0f);
	}
}

void UTunaSweeperGameHudWidget::RefreshDialogueHudVisibility()
{
	if (IsHousingModeActive())
	{
		ForceCollapseHudWidget(BottomStatusWidget);
		ForceCollapseHudWidget(QuickSlotBarWidget);
		ForceCollapseHudWidget(DebuffBarWidget);
		return;
	}

	const bool bSuppressBottomHud = IsGameplayBottomHudSuppressed();
	const ESlateVisibility BottomStatusVisibility = bSuppressBottomHud
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	const ESlateVisibility QuickSlotVisibility = bSuppressBottomHud
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	const ESlateVisibility DebuffBarVisibility = (bSuppressBottomHud || !bDebuffBarHasActiveDebuffs)
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;

	if (BottomStatusWidget)
	{
		SetTransitionedWidgetVisibility(BottomStatusWidget, BottomStatusVisibility, BottomStatusTransitionEdge);
	}

	if (QuickSlotBarWidget)
	{
		SetTransitionedWidgetVisibility(QuickSlotBarWidget, QuickSlotVisibility, QuickSlotBarTransitionEdge);
	}

	if (DebuffBarWidget)
	{
		SetTransitionedWidgetVisibility(DebuffBarWidget, DebuffBarVisibility, DebuffBarTransitionEdge);
	}
}

void UTunaSweeperGameHudWidget::RefreshExtractionProgressWidget()
{
	EnsureExtractionProgressWidget();
	if (!ExtractionProgressWidget)
	{
		return;
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ExtractionProgressWidget->Slot))
	{
		CanvasSlot->SetPosition(FVector2D(0.0f, FMath::Max(0.0f, ExtractionProgressTopOffset)));
		CanvasSlot->SetSize(FVector2D(
			FMath::Max(1.0, ExtractionProgressWidgetSize.X),
			FMath::Max(1.0, ExtractionProgressWidgetSize.Y)));
	}

	const bool bShouldShowProgress =
		bExtractionProgressActive &&
		ExtractionProgressCurrentSeconds > 0.0f &&
		!IsGameplayBottomHudSuppressed();
	ExtractionProgressWidget->SetExtractionProgress(
		ExtractionProgressCurrentSeconds,
		ExtractionProgressRequiredSeconds,
		bShouldShowProgress);
}

void UTunaSweeperGameHudWidget::RefreshCursorDistanceWidget()
{
	EnsureCursorDistanceWidget();
	if (!CursorDistancePanel)
	{
		return;
	}

	auto HideDistancePanel = [this]()
	{
		LastCursorDistanceMeters = INDEX_NONE;
		SetTransitionedWidgetVisibility(
			CursorDistancePanel,
			ESlateVisibility::Collapsed,
			CursorDistanceTransitionEdge);
	};

	if (ActiveHudMode != ETunaSweeperHudMode::None || IsGameplayBottomHudSuppressed())
	{
		HideDistancePanel();
		return;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	ATunaSweeperTopDownCharacter* TunaCharacter = TunaPlayerController
		? Cast<ATunaSweeperTopDownCharacter>(TunaPlayerController->GetPawn())
		: nullptr;
	if (!TunaPlayerController || !TunaCharacter || TunaCharacter->IsDead())
	{
		HideDistancePanel();
		return;
	}

	FVector CursorWorldPoint = FVector::ZeroVector;
	if (!TunaPlayerController->TryGetCursorWorldPointOnPlane(TunaCharacter->GetActorLocation().Z, CursorWorldPoint))
	{
		HideDistancePanel();
		return;
	}

	const float DistanceMeters = FVector::Dist2D(TunaCharacter->GetActorLocation(), CursorWorldPoint) / 100.0f;
	const int32 RoundedDistanceMeters = FMath::Max(0, FMath::RoundToInt(DistanceMeters));
	if (CursorDistanceText && LastCursorDistanceMeters != RoundedDistanceMeters)
	{
		CursorDistanceText->SetText(FText::FromString(FString::Printf(TEXT("%dM"), RoundedDistanceMeters)));
		CursorDistanceText->SetColorAndOpacity(FSlateColor(GetCursorDistanceTextColor(RoundedDistanceMeters)));
		LastCursorDistanceMeters = RoundedDistanceMeters;
	}

	SetTransitionedWidgetVisibility(
		CursorDistancePanel,
		ESlateVisibility::HitTestInvisible,
		CursorDistanceTransitionEdge);
}

void UTunaSweeperGameHudWidget::ForceCollapseHudWidget(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	CacheHudTransitionBaseline(Widget);
	ActiveHudTransitions.RemoveAll([Widget](const FHudWidgetTransition& Transition)
	{
		return Transition.Widget.Get() == Widget;
	});

	const TWeakObjectPtr<UWidget> WidgetKey(Widget);
	if (const FWidgetTransform* BaseTransform = HudTransitionBaseTransforms.Find(WidgetKey))
	{
		Widget->SetRenderTransform(*BaseTransform);
	}
	if (const float* BaseOpacity = HudTransitionBaseOpacities.Find(WidgetKey))
	{
		Widget->SetRenderOpacity(*BaseOpacity);
	}
	Widget->SetVisibility(ESlateVisibility::Collapsed);
}

void UTunaSweeperGameHudWidget::BuildAmmoSelectorOptionTexts(TArray<FText>& OutOptionTexts, int32& OutFocusedIndex) const
{
	OutOptionTexts.Reset();
	OutFocusedIndex = INDEX_NONE;

	const APlayerController* PlayerController = GetOwningPlayer();
	const ATunaSweeperTopDownCharacter* TunaCharacter = PlayerController
		? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
		: nullptr;
	if (!TunaCharacter || !TunaCharacter->IsAmmoSelectionOpen())
	{
		return;
	}

	TArray<int32> AmmoItemIds;
	TunaCharacter->GetAmmoSelectionItemIds(AmmoItemIds);
	if (AmmoItemIds.Num() <= 0)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	for (int32 AmmoItemId : AmmoItemIds)
	{
		FText AmmoName = FText::Format(
			ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
			FText::AsNumber(AmmoItemId));
		if (ItemDataSubsystem)
		{
			ItemDataSubsystem->TryGetItemNameText(AmmoItemId, Language, AmmoName);
		}
		OutOptionTexts.Add(AmmoName);
	}

	OutFocusedIndex = TunaCharacter->GetAmmoSelectionFocusIndex();
}
