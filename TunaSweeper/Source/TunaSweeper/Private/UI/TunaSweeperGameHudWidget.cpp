#include "UI/TunaSweeperGameHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"

void UTunaSweeperGameHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnSelectedInventoryItemChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged);
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
		QuestSubsystem->OnQuestProgressChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleQuestProgressChanged);
	}

	EnsureQuestTrackerWidgets();
	CacheAmmoReloadWidgets();
	SetCenterPanelsVisible(false);
	SetItemInfoPanelVisible(false);
	RefreshBottomStatusFromGameInstance();
	RefreshQuestTrackerFromQuestSubsystem();
	RefreshQuickSlotsFromGameState();
	RefreshReloadWidgets();
	RefreshDialogueHudVisibility();
}

void UTunaSweeperGameHudWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTunaSweeperGameHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshBottomStatusFromGameInstance();
	RefreshQuestTrackerFromQuestSubsystem();
	RefreshQuickSlotsFromGameState();
	RefreshReloadWidgets();
	RefreshDialogueHudVisibility();
}

void UTunaSweeperGameHudWidget::SetCenterPanelsVisible(bool bVisible)
{
	if (CenterContentPanel)
	{
		CenterContentPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::SetInventoryAreaVisible(bool bVisible)
{
	if (bVisible)
	{
		SetCenterPanelsVisible(true);
	}

	if (InventoryAreaWidget)
	{
		InventoryAreaWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::SetItemInfoPanelVisible(bool bVisible)
{
	if (bVisible)
	{
		SetCenterPanelsVisible(true);
	}

	if (ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::ShowExternalPanel(ETunaSweeperHudExternalPanelMode PanelMode)
{
	if (PanelMode != ETunaSweeperHudExternalPanelMode::None)
	{
		SetCenterPanelsVisible(true);
	}

	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetVisibility(
			PanelMode == ETunaSweeperHudExternalPanelMode::None
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
		ExternalPanelWidget->SetExternalPanelMode(PanelMode);
	}
}

void UTunaSweeperGameHudWidget::ShowInventoryOnlyPanel()
{
	SetCenterPanelsVisible(true);
	SetInventoryAreaVisible(true);
	HandleSelectedInventoryItemChanged();
	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::None);
}

void UTunaSweeperGameHudWidget::ToggleInventoryOnlyPanel()
{
	const bool bCenterVisible = CenterContentPanel && CenterContentPanel->GetVisibility() != ESlateVisibility::Collapsed;

	if (!bCenterVisible)
	{
		ShowInventoryOnlyPanel();
	}
	else
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearSelectedItemSelection();
		}

		SetInventoryAreaVisible(false);
		SetItemInfoPanelVisible(false);
		ShowExternalPanel(ETunaSweeperHudExternalPanelMode::None);
		SetCenterPanelsVisible(false);
	}
}

void UTunaSweeperGameHudWidget::ShowLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance)
{
	SetCenterPanelsVisible(true);
	SetInventoryAreaVisible(true);
	HandleSelectedInventoryItemChanged();

	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ExternalPanelWidget->SetLootContainerInstance(ContainerInstance);
	}
}

bool UTunaSweeperGameHudWidget::IsInventoryUiOpen() const
{
	auto IsWidgetVisible = [](const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}

		const ESlateVisibility Visibility = Widget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	};

	if (CenterContentPanel)
	{
		return IsWidgetVisible(CenterContentPanel);
	}

	return IsWidgetVisible(InventoryAreaWidget) || IsWidgetVisible(ExternalPanelWidget);
}

void UTunaSweeperGameHudWidget::RefreshBottomStatusFromGameInstance()
{
	if (!BottomStatusWidget)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperPlayerHudState HudState = TunaGameInstance ? TunaGameInstance->PlayerHudState : FTunaSweeperPlayerHudState();

	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		const APawn* Pawn = PlayerController->GetPawn();
		const UTunaSweeperVitalsComponent* VitalsComponent = nullptr;
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(Pawn))
		{
			VitalsComponent = TunaCharacter->GetVitalsComponent();
		}
		else if (Pawn)
		{
			VitalsComponent = Pawn->FindComponentByClass<UTunaSweeperVitalsComponent>();
		}

		if (VitalsComponent)
		{
			const FTunaSweeperVitalsState& VitalsState = VitalsComponent->GetVitalsState();
			HudState.Health = VitalsState.Health;
			HudState.Food = VitalsState.Food;
			HudState.Hydration = VitalsState.Hydration;
		}
	}

	BottomStatusWidget->SetHudState(HudState);
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

	int32 SelectedSlotNumber = 0;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn()))
		{
			SelectedSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
		}
	}
	QuickSlotBarWidget->SetSelectedQuickSlot(SelectedSlotNumber);

	for (int32 SlotNumber = 1; SlotNumber <= 2; ++SlotNumber)
	{
		FTunaSweeperItemInstance WeaponInstance;
		FTunaSweeperItemDefinition WeaponDefinition;
		if (!TunaGameInstance ||
			!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SlotNumber, WeaponInstance, WeaponDefinition))
		{
			QuickSlotBarWidget->ClearQuickSlotIcon(SlotNumber);
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

		FText AmmoTypeText = FText::FromString(TEXT("\uD0C4\uC57D \uBBF8\uC9C0\uC815"));
		const int32 AmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SlotNumber);
		if (AmmoItemId != INDEX_NONE)
		{
			AmmoTypeText = FText::FromString(FString::Printf(TEXT("Ammo %d"), AmmoItemId));
			if (ItemDataSubsystem)
			{
				ItemDataSubsystem->TryGetItemNameText(AmmoItemId, ETunaSweeperItemTextLanguage::Korean, AmmoTypeText);
			}
		}
		QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, AmmoTypeText, true);
		QuickSlotBarWidget->SetWeaponAmmoText(
			SlotNumber,
			TunaGameInstance->GetWeaponLoadedAmmoCount(SlotNumber),
			TunaGameInstance->GetWeaponInventoryAmmoCount(SlotNumber),
			true);
	}
}

void UTunaSweeperGameHudWidget::RefreshReloadWidgets()
{
	CacheAmmoReloadWidgets();

	const bool bDialogueActive = IsDialogueSequenceActive();
	ATunaSweeperTopDownCharacter* TunaCharacter = nullptr;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn());
	}

	const bool bShowReload = !bDialogueActive && TunaCharacter && TunaCharacter->IsWeaponReloading();
	const float ReloadProgress = bShowReload ? TunaCharacter->GetReloadProgress() : 0.0f;
	bool bShowReloadPrompt = false;
	if (!bDialogueActive && !bShowReload && TunaCharacter && !TunaCharacter->IsAmmoSelectionOpen() && !IsInventoryUiOpen())
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
		QuickSlotBarWidget->SetReloadProgress(ReloadProgress, bShowReload);
	}

	if (CenterReloadGaugeRoot)
	{
		CenterReloadGaugeRoot->SetVisibility(bShowReload ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterReloadPromptRoot)
	{
		CenterReloadPromptRoot->SetVisibility(bShowReloadPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterReloadPercentText)
	{
		CenterReloadPercentText->SetText(
			bShowReload
				? FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ReloadProgress * 100.0f)))
				: FText::GetEmpty());
	}

	const int32 FilledSegmentCount = FMath::CeilToInt(ReloadProgress * CenterReloadSegments.Num());
	for (int32 SegmentIndex = 0; SegmentIndex < CenterReloadSegments.Num(); ++SegmentIndex)
	{
		if (CenterReloadSegments[SegmentIndex])
		{
			CenterReloadSegments[SegmentIndex]->SetRenderOpacity(
				bShowReload && SegmentIndex < FilledSegmentCount ? 1.0f : 0.18f);
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

void UTunaSweeperGameHudWidget::CacheAmmoReloadWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	CenterReloadGaugeRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadGaugeRoot")));
	CenterReloadPromptRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadPromptRoot")));
	CenterReloadPercentText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CenterReloadPercentText"))));
	CenterReloadSegments.SetNum(12);
	for (int32 SegmentNumber = 1; SegmentNumber <= CenterReloadSegments.Num(); ++SegmentNumber)
	{
		CenterReloadSegments[SegmentNumber - 1] = Cast<UBorder>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("CenterReloadSegment%02d"), SegmentNumber))));
	}
}

void UTunaSweeperGameHudWidget::RefreshDialogueHudVisibility()
{
	const bool bDialogueActive = IsDialogueSequenceActive();
	const ESlateVisibility BottomHudVisibility = bDialogueActive
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;

	if (BottomStatusWidget)
	{
		BottomStatusWidget->SetVisibility(BottomHudVisibility);
	}

	if (QuickSlotBarWidget)
	{
		QuickSlotBarWidget->SetVisibility(BottomHudVisibility);
	}
}

void UTunaSweeperGameHudWidget::EnsureQuestTrackerWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!QuestTrackerRoot)
	{
		QuestTrackerRoot = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("QuestTrackerRoot"))));
		QuestTrackerTitleText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("QuestTrackerTitleText"))));
		QuestTrackerObjectiveText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("QuestTrackerObjectiveText"))));
	}

	if (QuestTrackerRoot)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	QuestTrackerRoot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestTrackerRoot"));
	UVerticalBox* TrackerStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestTrackerStack"));
	QuestTrackerTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTrackerTitleText"));
	QuestTrackerObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTrackerObjectiveText"));
	if (!QuestTrackerRoot || !TrackerStack || !QuestTrackerTitleText || !QuestTrackerObjectiveText)
	{
		return;
	}

	QuestTrackerRoot->SetPadding(FMargin(14.0f, 10.0f));
	QuestTrackerRoot->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.78f));
	QuestTrackerRoot->SetContent(TrackerStack);

	QuestTrackerTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.98f, 1.0f, 1.0f)));
	QuestTrackerTitleText->SetAutoWrapText(false);
	FSlateFontInfo TitleFont = QuestTrackerTitleText->GetFont();
	TitleFont.Size = 18;
	QuestTrackerTitleText->SetFont(TitleFont);

	QuestTrackerObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.86f, 0.90f, 1.0f)));
	QuestTrackerObjectiveText->SetAutoWrapText(true);
	QuestTrackerObjectiveText->SetWrapTextAt(320.0f);
	FSlateFontInfo ObjectiveFont = QuestTrackerObjectiveText->GetFont();
	ObjectiveFont.Size = 15;
	QuestTrackerObjectiveText->SetFont(ObjectiveFont);

	UVerticalBoxSlot* TitleSlot = TrackerStack->AddChildToVerticalBox(QuestTrackerTitleText);
	if (TitleSlot)
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}
	TrackerStack->AddChildToVerticalBox(QuestTrackerObjectiveText);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(QuestTrackerRoot);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(24.0f, 104.0f));
		CanvasSlot->SetSize(FVector2D(360.0f, 132.0f));
		CanvasSlot->SetZOrder(5);
	}
}

void UTunaSweeperGameHudWidget::RefreshQuestTrackerFromQuestSubsystem()
{
	EnsureQuestTrackerWidgets();
	if (!QuestTrackerRoot)
	{
		return;
	}

	UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem || QuestSubsystem->GetTrackedQuestId().IsNone())
	{
		QuestTrackerRoot->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FTunaSweeperQuestDefinition QuestDefinition;
	const FName TrackedQuestId = QuestSubsystem->GetTrackedQuestId();
	if (!QuestSubsystem->TryGetQuestDefinition(TrackedQuestId, QuestDefinition))
	{
		QuestTrackerRoot->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TArray<FTunaSweeperObjectiveProgressView> ObjectiveProgress;
	if (!QuestSubsystem->GetQuestObjectiveProgress(TrackedQuestId, ObjectiveProgress))
	{
		QuestTrackerRoot->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (QuestTrackerTitleText)
	{
		QuestTrackerTitleText->SetText(QuestDefinition.Title);
	}

	if (QuestTrackerObjectiveText)
	{
		TArray<FString> ObjectiveLines;
		for (const FTunaSweeperObjectiveProgressView& Progress : ObjectiveProgress)
		{
			ObjectiveLines.Add(FString::Printf(
				TEXT("%s (%d/%d)"),
				*Progress.Text.ToString(),
				FMath::Clamp(Progress.CurrentCount, 0, FMath::Max(1, Progress.RequiredCount)),
				FMath::Max(1, Progress.RequiredCount)));
		}

		if (QuestSubsystem->GetQuestState(TrackedQuestId) == ETunaSweeperQuestState::RewardAvailable)
		{
			ObjectiveLines.Add(TEXT("\uBCF4\uC0C1 \uC218\uB839 \uAC00\uB2A5"));
		}

		QuestTrackerObjectiveText->SetText(FText::FromString(FString::Join(ObjectiveLines, LINE_TERMINATOR)));
	}

	QuestTrackerRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
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

	for (int32 AmmoItemId : AmmoItemIds)
	{
		FText AmmoName = FText::FromString(FString::Printf(TEXT("Ammo %d"), AmmoItemId));
		if (ItemDataSubsystem)
		{
			ItemDataSubsystem->TryGetItemNameText(AmmoItemId, ETunaSweeperItemTextLanguage::Korean, AmmoName);
		}
		OutOptionTexts.Add(AmmoName);
	}

	OutFocusedIndex = TunaCharacter->GetAmmoSelectionFocusIndex();
}

bool UTunaSweeperGameHudWidget::IsDialogueSequenceActive() const
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	return TunaPlayerController && TunaPlayerController->IsDialogueSequenceActive();
}

void UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged()
{
	const bool bCenterVisible = CenterContentPanel && CenterContentPanel->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bHasSelection = bCenterVisible &&
		GetGameInstance<UTunaSweeperGameInstance>() &&
		GetGameInstance<UTunaSweeperGameInstance>()->HasSelectedInventoryItem();

	SetItemInfoPanelVisible(bHasSelection);
	if (bHasSelection && ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->RefreshSelectedItemInfo();
	}
}

void UTunaSweeperGameHudWidget::HandleQuestProgressChanged()
{
	RefreshQuestTrackerFromQuestSubsystem();
}
