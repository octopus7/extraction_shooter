#include "UI/TunaSweeperHudQuickSlotBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

void UTunaSweeperHudQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheNamedWidgets();
	SetSelectedQuickSlot(0);
	SetReloadProgress(0.0f, false);
	SetAmmoSelectorOptions(TArray<FText>(), INDEX_NONE, false);
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
	}
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
	bool bVisible)
{
	CacheNamedWidgets();

	if (AmmoSelectorPanel)
	{
		AmmoSelectorPanel->SetVisibility(bVisible && OptionTexts.Num() > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	const int32 OptionCount = FMath::Min(OptionTexts.Num(), AmmoSelectorOptionTexts.Num());
	for (int32 OptionIndex = 0; OptionIndex < AmmoSelectorOptionTexts.Num(); ++OptionIndex)
	{
		const bool bShowOption = bVisible && OptionIndex < OptionCount;
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

void UTunaSweeperHudQuickSlotBarWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	SlotIconImages.SetNum(8);
	SlotSelectionFrames.SetNum(8);
	SlotAmmoTexts.SetNum(8);
	AmmoSelectorOptionBackgrounds.SetNum(6);
	AmmoSelectorOptionTexts.SetNum(6);

	for (int32 SlotNumber = 1; SlotNumber <= 8; ++SlotNumber)
	{
		const int32 SlotIndex = SlotNumber - 1;
		SlotIconImages[SlotIndex] = Cast<UImage>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dIcon"), SlotNumber))));
		SlotSelectionFrames[SlotIndex] = WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dSelectionFrame"), SlotNumber)));
		SlotAmmoTexts[SlotIndex] = Cast<UTextBlock>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dAmmoText"), SlotNumber))));
	}

	AmmoSelectorPanel = WidgetTree->FindWidget(FName(TEXT("AmmoSelectorPanel")));
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
