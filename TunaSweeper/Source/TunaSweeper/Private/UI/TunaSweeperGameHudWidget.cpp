#include "UI/TunaSweeperGameHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Player/TunaSweeperPlayerController.h"
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

	CacheAmmoReloadWidgets();
	SetCenterPanelsVisible(false);
	SetItemInfoPanelVisible(false);
	RefreshBottomStatusFromGameInstance();
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

	Super::NativeDestruct();
}

void UTunaSweeperGameHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshBottomStatusFromGameInstance();
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
