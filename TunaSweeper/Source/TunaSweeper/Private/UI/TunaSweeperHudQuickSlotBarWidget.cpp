#include "UI/TunaSweeperHudQuickSlotBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

void UTunaSweeperHudQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheNamedWidgets();
	SetSelectedQuickSlot(1);
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

void UTunaSweeperHudQuickSlotBarWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	SlotIconImages.SetNum(8);
	SlotSelectionFrames.SetNum(8);

	for (int32 SlotNumber = 1; SlotNumber <= 8; ++SlotNumber)
	{
		const int32 SlotIndex = SlotNumber - 1;
		SlotIconImages[SlotIndex] = Cast<UImage>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dIcon"), SlotNumber))));
		SlotSelectionFrames[SlotIndex] = WidgetTree->FindWidget(FName(*FString::Printf(TEXT("QuickSlot%dSelectionFrame"), SlotNumber)));
	}
}

int32 UTunaSweeperHudQuickSlotBarWidget::GetSlotIndex(int32 SlotNumber) const
{
	return SlotNumber - 1;
}
