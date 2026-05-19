#include "UI/TunaSweeperHudQuickSlotBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

namespace
{
	constexpr float QuickSlotRootWidth = 620.0f;
	constexpr float QuickSlotRowWidth = 556.0f;
	constexpr float WeaponSlotWidth = 82.0f;
	constexpr float SlotGapWidth = 8.0f;
	constexpr float AmmoSelectorPanelOffsetY = 28.0f;
}

void UTunaSweeperHudQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheNamedWidgets();
	SetSelectedQuickSlot(0);
	SetReloadProgress(0.0f, false);
	SetAmmoSelectorOptions(TArray<FText>(), INDEX_NONE, 0, false);
}

void UTunaSweeperHudQuickSlotBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	CacheNamedWidgets();
}

void UTunaSweeperHudQuickSlotBarWidget::SetQuickSlotIcon(int32 SlotNumber, UTexture2D* IconTexture)
{
	CacheNamedWidgets();

	const int32 SlotIndex = GetSlotIndex(SlotNumber);
	if (!SlotIconImages.IsValidIndex(SlotIndex) || !SlotIconImages[SlotIndex])
	{
		return;
	}

	if (IconTexture)
	{
		SlotIconImages[SlotIndex]->SetBrushFromTexture(IconTexture, true);
		SlotIconImages[SlotIndex]->SetOpacity(1.0f);
	}
	else
	{
		ClearQuickSlotIcon(SlotNumber);
	}
}

void UTunaSweeperHudQuickSlotBarWidget::ClearQuickSlotIcon(int32 SlotNumber)
{
	CacheNamedWidgets();

	const int32 SlotIndex = GetSlotIndex(SlotNumber);
	if (SlotIconImages.IsValidIndex(SlotIndex) && SlotIconImages[SlotIndex])
	{
		SlotIconImages[SlotIndex]->SetBrushFromTexture(nullptr, false);
		SlotIconImages[SlotIndex]->SetOpacity(0.0f);
	}
}

void UTunaSweeperHudQuickSlotBarWidget::SetSelectedQuickSlot(int32 SlotNumber)
{
	CacheNamedWidgets();

	const int32 SelectedIndex = GetSlotIndex(SlotNumber);
	for (int32 Index = 0; Index < SlotSelectionFrames.Num(); ++Index)
	{
		if (SlotSelectionFrames[Index])
		{
			SlotSelectionFrames[Index]->SetVisibility(
				Index == SelectedIndex
					? ESlateVisibility::HitTestInvisible
					: ESlateVisibility::Hidden);
		}

		const bool bShowAmmoKey = Index == SelectedIndex && Index < 2;
		if (SlotAmmoKeyBackgrounds.IsValidIndex(Index) && SlotAmmoKeyBackgrounds[Index])
		{
			SlotAmmoKeyBackgrounds[Index]->SetVisibility(bShowAmmoKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (SlotAmmoKeyTexts.IsValidIndex(Index) && SlotAmmoKeyTexts[Index])
		{
			SlotAmmoKeyTexts[Index]->SetVisibility(bShowAmmoKey ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			SlotAmmoKeyTexts[Index]->SetText(bShowAmmoKey ? FText::FromString(TEXT("T")) : FText::GetEmpty());
		}
	}
}

void UTunaSweeperHudQuickSlotBarWidget::SetWeaponAmmoTypeText(
	int32 SlotNumber,
	const FText& AmmoTypeText,
	bool bVisible)
{
	CacheNamedWidgets();

	const int32 SlotIndex = GetSlotIndex(SlotNumber);
	if (!SlotAmmoTypeTexts.IsValidIndex(SlotIndex) || !SlotAmmoTypeTexts[SlotIndex])
	{
		return;
	}

	if (SlotAmmoTypeContainers.IsValidIndex(SlotIndex) && SlotAmmoTypeContainers[SlotIndex])
	{
		SlotAmmoTypeContainers[SlotIndex]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SlotAmmoTypeTexts[SlotIndex]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SlotAmmoTypeTexts[SlotIndex]->SetText(bVisible ? AmmoTypeText : FText::GetEmpty());
}

void UTunaSweeperHudQuickSlotBarWidget::SetWeaponAmmoText(
	int32 SlotNumber,
	int32 LoadedAmmoCount,
	int32 InventoryAmmoCount,
	bool bVisible)
{
	CacheNamedWidgets();

	const int32 SlotIndex = GetSlotIndex(SlotNumber);
	if (!SlotAmmoTexts.IsValidIndex(SlotIndex) || !SlotAmmoTexts[SlotIndex])
	{
		return;
	}

	if (SlotAmmoTextContainers.IsValidIndex(SlotIndex) && SlotAmmoTextContainers[SlotIndex])
	{
		SlotAmmoTextContainers[SlotIndex]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SlotAmmoTexts[SlotIndex]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SlotAmmoTexts[SlotIndex]->SetText(
		bVisible
			? FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::Max(0, LoadedAmmoCount), FMath::Max(0, InventoryAmmoCount)))
			: FText::GetEmpty());
}

void UTunaSweeperHudQuickSlotBarWidget::SetReloadProgress(float Progress, bool bVisible)
{
	CacheNamedWidgets();

	if (ReloadProgressPanel)
	{
		ReloadProgressPanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ReloadProgressBar)
	{
		ReloadProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	}
}

void UTunaSweeperHudQuickSlotBarWidget::SetAmmoSelectorOptions(
	const TArray<FText>& OptionTexts,
	int32 FocusedOptionIndex,
	int32 WeaponSlotNumber,
	bool bVisible)
{
	CacheNamedWidgets();

	const int32 OptionCount = FMath::Min(OptionTexts.Num(), AmmoSelectorOptionTexts.Num());
	const bool bShowPanel = bVisible && OptionCount > 0;
	if (AmmoSelectorPanel)
	{
		SetAmmoSelectorPanelPosition(WeaponSlotNumber);
		AmmoSelectorPanel->SetVisibility(bShowPanel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetAmmoSelectorPromptVisible(FText::GetEmpty(), false);
	SetAmmoSelectorKeyHintVisible(bShowPanel);

	for (int32 OptionIndex = 0; OptionIndex < AmmoSelectorOptionTexts.Num(); ++OptionIndex)
	{
		const bool bShowOption = bShowPanel && OptionIndex < OptionCount;
		if (AmmoSelectorOptionTexts[OptionIndex])
		{
			AmmoSelectorOptionTexts[OptionIndex]->SetVisibility(bShowOption ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			AmmoSelectorOptionTexts[OptionIndex]->SetText(bShowOption ? OptionTexts[OptionIndex] : FText::GetEmpty());
		}

		if (AmmoSelectorOptionBackgrounds.IsValidIndex(OptionIndex) && AmmoSelectorOptionBackgrounds[OptionIndex])
		{
			AmmoSelectorOptionBackgrounds[OptionIndex]->SetVisibility(bShowOption ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			AmmoSelectorOptionBackgrounds[OptionIndex]->SetRenderOpacity(OptionIndex == FocusedOptionIndex ? 1.0f : 0.62f);
		}
	}
}

void UTunaSweeperHudQuickSlotBarWidget::SetAmmoSelectorPrompt(
	int32 WeaponSlotNumber,
	const FText& PromptText,
	bool bVisible)
{
	CacheNamedWidgets();

	const bool bShowPanel = bVisible && !PromptText.IsEmpty();
	if (AmmoSelectorPanel)
	{
		SetAmmoSelectorPanelPosition(WeaponSlotNumber);
		AmmoSelectorPanel->SetVisibility(bShowPanel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetAmmoSelectorPromptVisible(PromptText, bShowPanel);
	SetAmmoSelectorKeyHintVisible(bShowPanel);

	for (int32 OptionIndex = 0; OptionIndex < AmmoSelectorOptionTexts.Num(); ++OptionIndex)
	{
		if (AmmoSelectorOptionTexts[OptionIndex])
		{
			AmmoSelectorOptionTexts[OptionIndex]->SetVisibility(ESlateVisibility::Collapsed);
			AmmoSelectorOptionTexts[OptionIndex]->SetText(FText::GetEmpty());
		}

		if (AmmoSelectorOptionBackgrounds.IsValidIndex(OptionIndex) && AmmoSelectorOptionBackgrounds[OptionIndex])
		{
			AmmoSelectorOptionBackgrounds[OptionIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTunaSweeperHudQuickSlotBarWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	SlotIconImages.SetNum(8);
	SlotSelectionFrames.SetNum(8);
	SlotAmmoTexts.SetNum(8);
	SlotAmmoTextContainers.SetNum(8);
	SlotAmmoTypeTexts.SetNum(8);
	SlotAmmoTypeContainers.SetNum(8);
	SlotAmmoKeyBackgrounds.SetNum(8);
	SlotAmmoKeyTexts.SetNum(8);
	AmmoSelectorOptionBackgrounds.SetNum(6);
	AmmoSelectorOptionTexts.SetNum(6);

	for (int32 SlotNumber = 1; SlotNumber <= 8; ++SlotNumber)
	{
		const int32 SlotIndex = SlotNumber - 1;
		SlotIconImages[SlotIndex] = Cast<UImage>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dIcon"), SlotNumber))));
		SlotSelectionFrames[SlotIndex] = WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dSelectionFrame"), SlotNumber)));
		SlotAmmoTypeTexts[SlotIndex] = Cast<UTextBlock>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dAmmoTypeText"), SlotNumber))));
		SlotAmmoTypeContainers[SlotIndex] = WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dAmmoTypeContainer"), SlotNumber)));
		SlotAmmoKeyBackgrounds[SlotIndex] = WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dAmmoKeyBackground"), SlotNumber)));
		SlotAmmoKeyTexts[SlotIndex] = Cast<UTextBlock>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dAmmoKeyText"), SlotNumber))));
		SlotAmmoTexts[SlotIndex] = Cast<UTextBlock>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dAmmoText"), SlotNumber))));
		SlotAmmoTextContainers[SlotIndex] = WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dAmmoTextContainer"), SlotNumber)));
	}

	AmmoSelectorPanel = WidgetTree->FindWidget(FName(TEXT("AmmoSelectorPanel")));
	AmmoSelectorPromptBackground = WidgetTree->FindWidget(FName(TEXT("AmmoSelectorPromptBackground")));
	AmmoSelectorPromptText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("AmmoSelectorPromptText"))));
	AmmoSelectorKeyBackground = WidgetTree->FindWidget(FName(TEXT("AmmoSelectorKeyBackground")));
	AmmoSelectorKeyText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("AmmoSelectorKeyText"))));
	ReloadProgressPanel = WidgetTree->FindWidget(FName(TEXT("ReloadProgressPanel")));
	ReloadProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(FName(TEXT("ReloadProgressBar"))));

	for (int32 OptionNumber = 1; OptionNumber <= AmmoSelectorOptionTexts.Num(); ++OptionNumber)
	{
		const int32 OptionIndex = OptionNumber - 1;
		AmmoSelectorOptionBackgrounds[OptionIndex] = WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("AmmoOption%dBackground"), OptionNumber)));
		AmmoSelectorOptionTexts[OptionIndex] = Cast<UTextBlock>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("AmmoOption%dText"), OptionNumber))));
	}
}

int32 UTunaSweeperHudQuickSlotBarWidget::GetSlotIndex(int32 SlotNumber) const
{
	return SlotNumber - 1;
}

float UTunaSweeperHudQuickSlotBarWidget::GetWeaponSlotCenterOffsetX(int32 WeaponSlotNumber) const
{
	if (WeaponSlotNumber < 1 || WeaponSlotNumber > 2)
	{
		return 0.0f;
	}

	const float SlotRowLeft = (QuickSlotRootWidth - QuickSlotRowWidth) * 0.5f;
	const float SlotCenterX = SlotRowLeft + (WeaponSlotWidth * 0.5f) + ((WeaponSlotNumber - 1) * (WeaponSlotWidth + SlotGapWidth));
	return SlotCenterX - (QuickSlotRootWidth * 0.5f);
}

void UTunaSweeperHudQuickSlotBarWidget::SetAmmoSelectorPanelPosition(int32 WeaponSlotNumber)
{
	if (!AmmoSelectorPanel)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AmmoSelectorPanel->Slot);
	if (!CanvasSlot)
	{
		return;
	}

	CanvasSlot->SetAutoSize(true);
	CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
	CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	CanvasSlot->SetPosition(FVector2D(GetWeaponSlotCenterOffsetX(WeaponSlotNumber), AmmoSelectorPanelOffsetY));
}

void UTunaSweeperHudQuickSlotBarWidget::SetAmmoSelectorPromptVisible(const FText& PromptText, bool bVisible)
{
	if (AmmoSelectorPromptBackground)
	{
		AmmoSelectorPromptBackground->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (AmmoSelectorPromptText)
	{
		AmmoSelectorPromptText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		AmmoSelectorPromptText->SetText(bVisible ? PromptText : FText::GetEmpty());
	}
}

void UTunaSweeperHudQuickSlotBarWidget::SetAmmoSelectorKeyHintVisible(bool bVisible)
{
	if (AmmoSelectorKeyBackground)
	{
		AmmoSelectorKeyBackground->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (AmmoSelectorKeyText)
	{
		AmmoSelectorKeyText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		AmmoSelectorKeyText->SetText(bVisible ? FText::FromString(TEXT("T")) : FText::GetEmpty());
	}
}
