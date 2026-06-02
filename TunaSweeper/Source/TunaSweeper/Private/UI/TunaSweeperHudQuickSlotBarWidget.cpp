#include "UI/TunaSweeperHudQuickSlotBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UI/TunaSweeperItemRaritySlotAccentWidget.h"
#include "UI/TunaSweeperUIFont.h"

namespace
{
	constexpr float QuickSlotRootWidth = 694.0f;
	constexpr float QuickSlotRootHeight = 208.0f;
	constexpr float QuickSlotRowWidth = 690.0f;
	constexpr float WeaponSlotWidth = 82.0f;
	constexpr float SlotGapWidth = 8.0f;
	constexpr float AmmoSelectorPanelOffsetY = 28.0f;
	constexpr float CancelableActionPromptOffsetY = 6.0f;
	constexpr float CancelableActionProgressOffsetY = 36.0f;
	constexpr float CancelableActionProgressWidth = 210.0f;
	constexpr float CancelableActionProgressHeight = 18.0f;
	constexpr float CancelableActionProgressRadius = 9.0f;

	FSlateBrush MakeRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		float Radius = 4.0f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FProgressBarStyle MakeCancelableActionProgressBarStyle()
	{
		const FVector2D ProgressSize(CancelableActionProgressWidth, CancelableActionProgressHeight);

		FProgressBarStyle Style;
		Style.SetBackgroundImage(MakeRoundedBoxBrush(
			ProgressSize,
			FLinearColor(0.012f, 0.016f, 0.020f, 0.88f),
			FLinearColor(0.90f, 1.0f, 0.88f, 0.42f),
			1.0f,
			CancelableActionProgressRadius));
		Style.SetFillImage(MakeRoundedBoxBrush(
			ProgressSize,
			FLinearColor(0.50f, 1.0f, 0.68f, 1.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			0.0f,
			CancelableActionProgressRadius));
		Style.SetMarqueeImage(MakeRoundedBoxBrush(
			ProgressSize,
			FLinearColor(0.64f, 1.0f, 0.78f, 1.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			0.0f,
			CancelableActionProgressRadius));
		return Style;
	}
}

void UTunaSweeperHudQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheNamedWidgets();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	SetSelectedQuickSlot(0);
	SetCancelableActionProgress(0.0f, false);
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
	SetQuickSlotItemGrade(SlotNumber, ETunaSweeperItemGrade::Common, false);
}

void UTunaSweeperHudQuickSlotBarWidget::SetMeleeQuickSlotIcon(UTexture2D* IconTexture)
{
	CacheNamedWidgets();

	if (!QuickSlotMeleeIcon)
	{
		return;
	}

	if (IconTexture)
	{
		QuickSlotMeleeIcon->SetBrushFromTexture(IconTexture, true);
		QuickSlotMeleeIcon->SetOpacity(1.0f);
	}
	else
	{
		ClearMeleeQuickSlotIcon();
	}
}

void UTunaSweeperHudQuickSlotBarWidget::ClearMeleeQuickSlotIcon()
{
	CacheNamedWidgets();

	if (QuickSlotMeleeIcon)
	{
		QuickSlotMeleeIcon->SetBrushFromTexture(nullptr, false);
		QuickSlotMeleeIcon->SetOpacity(0.0f);
	}
	SetMeleeQuickSlotItemGrade(ETunaSweeperItemGrade::Common, false);
}

void UTunaSweeperHudQuickSlotBarWidget::SetQuickSlotItemGrade(
	int32 SlotNumber,
	ETunaSweeperItemGrade ItemGrade,
	bool bVisible)
{
	CacheNamedWidgets();

	const int32 SlotIndex = GetSlotIndex(SlotNumber);
	if (!SlotIconImages.IsValidIndex(SlotIndex) || !SlotIconImages[SlotIndex])
	{
		return;
	}

	if (!SlotRarityAccentWidgets.IsValidIndex(SlotIndex) || !SlotRarityAccentWidgets[SlotIndex])
	{
		SlotRarityAccentWidgets[SlotIndex] = EnsureQuickSlotRarityAccentWidget(
			SlotIconImages[SlotIndex],
			FName(*FString::Printf(TEXT("QuickSlot%dRarityAccent"), SlotNumber)));
	}

	if (SlotRarityAccentWidgets.IsValidIndex(SlotIndex) && SlotRarityAccentWidgets[SlotIndex])
	{
		SlotRarityAccentWidgets[SlotIndex]->SetItemGrade(ItemGrade, bVisible);
	}
}

void UTunaSweeperHudQuickSlotBarWidget::SetMeleeQuickSlotItemGrade(
	ETunaSweeperItemGrade ItemGrade,
	bool bVisible)
{
	CacheNamedWidgets();

	if (!QuickSlotMeleeIcon)
	{
		return;
	}

	if (!QuickSlotMeleeRarityAccentWidget)
	{
		QuickSlotMeleeRarityAccentWidget = EnsureQuickSlotRarityAccentWidget(
			QuickSlotMeleeIcon,
			FName(TEXT("QuickSlotMeleeRarityAccent")));
	}

	if (QuickSlotMeleeRarityAccentWidget)
	{
		QuickSlotMeleeRarityAccentWidget->SetItemGrade(ItemGrade, bVisible);
	}
}

void UTunaSweeperHudQuickSlotBarWidget::SetSelectedQuickSlot(int32 SlotNumber)
{
	CacheNamedWidgets();

	const int32 SelectedIndex = GetSlotIndex(SlotNumber);
	if (QuickSlotMeleeSelectionFrame)
	{
		QuickSlotMeleeSelectionFrame->SetVisibility(ESlateVisibility::Hidden);
	}

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

void UTunaSweeperHudQuickSlotBarWidget::SetSelectedMeleeQuickSlot()
{
	CacheNamedWidgets();

	for (const TObjectPtr<UWidget>& SelectionFrame : SlotSelectionFrames)
	{
		if (SelectionFrame)
		{
			SelectionFrame->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	for (const TObjectPtr<UWidget>& AmmoKeyBackground : SlotAmmoKeyBackgrounds)
	{
		if (AmmoKeyBackground)
		{
			AmmoKeyBackground->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	for (const TObjectPtr<UTextBlock>& AmmoKeyText : SlotAmmoKeyTexts)
	{
		if (AmmoKeyText)
		{
			AmmoKeyText->SetVisibility(ESlateVisibility::Collapsed);
			AmmoKeyText->SetText(FText::GetEmpty());
		}
	}

	if (QuickSlotMeleeSelectionFrame)
	{
		QuickSlotMeleeSelectionFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
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

void UTunaSweeperHudQuickSlotBarWidget::SetCancelableActionProgress(float Progress, bool bVisible)
{
	CacheNamedWidgets();

	const ESlateVisibility TargetVisibility = bVisible
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;

	if (CancelableActionProgressPanel)
	{
		CancelableActionProgressPanel->SetVisibility(TargetVisibility);
	}

	if (CancelableActionProgressBar)
	{
		CancelableActionProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	}

	if (CancelableActionPromptRoot)
	{
		CancelableActionPromptRoot->SetVisibility(TargetVisibility);
	}
	if (CancelableActionCancelKeyBackground)
	{
		CancelableActionCancelKeyBackground->SetVisibility(TargetVisibility);
	}
	if (CancelableActionCancelKeyText)
	{
		CancelableActionCancelKeyText->SetVisibility(TargetVisibility);
		CancelableActionCancelKeyText->SetText(bVisible ? FText::FromString(TEXT("X")) : FText::GetEmpty());
	}
	if (CancelableActionCancelText)
	{
		CancelableActionCancelText->SetVisibility(TargetVisibility);
		CancelableActionCancelText->SetText(bVisible ? FText::FromString(TEXT("\uB3D9\uC791 \uCDE8\uC18C")) : FText::GetEmpty());
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
	SlotRarityAccentWidgets.SetNum(8);
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
	CancelableActionProgressPanel = WidgetTree->FindWidget(FName(TEXT("CancelableActionProgressPanel")));
	if (!CancelableActionProgressPanel)
	{
		CancelableActionProgressPanel = WidgetTree->FindWidget(FName(TEXT("ReloadProgressPanel")));
	}
	CancelableActionProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(FName(TEXT("CancelableActionProgressBar"))));
	if (!CancelableActionProgressBar)
	{
		CancelableActionProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(FName(TEXT("ReloadProgressBar"))));
	}
	CancelableActionPromptRoot = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("CancelableActionPromptRoot"))));
	CancelableActionCancelKeyBackground = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("CancelableActionCancelKeyBackground"))));
	CancelableActionCancelKeyText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CancelableActionCancelKeyText"))));
	CancelableActionCancelText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CancelableActionCancelText"))));
	QuickSlotMeleeIcon = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("QuickSlotMeleeIcon"))));
	QuickSlotMeleeSelectionFrame = WidgetTree->FindWidget(FName(TEXT("QuickSlotMeleeSelectionFrame")));

	if (!IsDesignTime())
	{
		EnsureCancelableActionPromptWidgets();
	}

	for (int32 OptionNumber = 1; OptionNumber <= AmmoSelectorOptionTexts.Num(); ++OptionNumber)
	{
		const int32 OptionIndex = OptionNumber - 1;
		AmmoSelectorOptionBackgrounds[OptionIndex] = WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("AmmoOption%dBackground"), OptionNumber)));
		AmmoSelectorOptionTexts[OptionIndex] = Cast<UTextBlock>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("AmmoOption%dText"), OptionNumber))));
	}

	ApplyCancelableActionLayout();
}

UTunaSweeperItemRaritySlotAccentWidget* UTunaSweeperHudQuickSlotBarWidget::EnsureQuickSlotRarityAccentWidget(
	UImage* IconImage,
	const FName& WidgetName)
{
	if (!WidgetTree || !IconImage)
	{
		return nullptr;
	}

	if (UTunaSweeperItemRaritySlotAccentWidget* ExistingWidget =
		Cast<UTunaSweeperItemRaritySlotAccentWidget>(WidgetTree->FindWidget(WidgetName)))
	{
		return ExistingWidget;
	}

	UOverlay* SlotOverlay = Cast<UOverlay>(IconImage->GetParent());
	if (!SlotOverlay)
	{
		return nullptr;
	}

	UTunaSweeperItemRaritySlotAccentWidget* AccentWidget =
		WidgetTree->ConstructWidget<UTunaSweeperItemRaritySlotAccentWidget>(
			UTunaSweeperItemRaritySlotAccentWidget::StaticClass(),
			WidgetName);
	if (!AccentWidget)
	{
		return nullptr;
	}

	const int32 IconIndex = SlotOverlay->GetChildIndex(IconImage);
	UOverlaySlot* AccentSlot = Cast<UOverlaySlot>(SlotOverlay->InsertChildAt(FMath::Max(0, IconIndex), AccentWidget));
	if (!AccentSlot)
	{
		AccentSlot = SlotOverlay->AddChildToOverlay(AccentWidget);
	}
	if (AccentSlot)
	{
		AccentSlot->SetHorizontalAlignment(HAlign_Fill);
		AccentSlot->SetVerticalAlignment(VAlign_Fill);
		AccentSlot->SetPadding(FMargin(0.0f));
	}

	return AccentWidget;
}

void UTunaSweeperHudQuickSlotBarWidget::EnsureCancelableActionPromptWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(FName(TEXT("RootCanvas"))));
	const bool bCreatedPromptRoot = !CancelableActionPromptRoot;
	if (!CancelableActionPromptRoot)
	{
		CancelableActionPromptRoot = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			FName(TEXT("CancelableActionPromptRoot")));
		if (CancelableActionPromptRoot && RootCanvas)
		{
			UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(CancelableActionPromptRoot);
			if (PromptSlot)
			{
				PromptSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
				PromptSlot->SetAlignment(FVector2D(0.5f, 0.0f));
				PromptSlot->SetPosition(FVector2D(0.0f, CancelableActionPromptOffsetY));
				PromptSlot->SetAutoSize(true);
				PromptSlot->SetZOrder(12);
			}
		}
	}

	const bool bCreatedCancelKeyBackground = !CancelableActionCancelKeyBackground;
	if (!CancelableActionCancelKeyBackground)
	{
		CancelableActionCancelKeyBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(TEXT("CancelableActionCancelKeyBackground")));
	}
	const bool bCreatedCancelKeyText = !CancelableActionCancelKeyText;
	if (!CancelableActionCancelKeyText)
	{
		CancelableActionCancelKeyText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(TEXT("CancelableActionCancelKeyText")));
	}
	const bool bCreatedCancelText = !CancelableActionCancelText;
	if (!CancelableActionCancelText)
	{
		CancelableActionCancelText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(TEXT("CancelableActionCancelText")));
	}

	if (bCreatedPromptRoot && CancelableActionPromptRoot)
	{
		CancelableActionPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CancelableActionCancelKeyBackground)
	{
		CancelableActionCancelKeyBackground->SetPadding(FMargin(8.0f, 2.0f));
		if (bCreatedCancelKeyBackground)
		{
			CancelableActionCancelKeyBackground->SetVisibility(ESlateVisibility::Collapsed);
		}
		CancelableActionCancelKeyBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(28.0f, 24.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			0.0f,
			7.0f));
		if (CancelableActionCancelKeyText)
		{
			CancelableActionCancelKeyBackground->SetContent(CancelableActionCancelKeyText);
		}
		if (CancelableActionPromptRoot && !CancelableActionCancelKeyBackground->Slot)
		{
			UHorizontalBoxSlot* KeySlot = CancelableActionPromptRoot->AddChildToHorizontalBox(CancelableActionCancelKeyBackground);
			if (KeySlot)
			{
				KeySlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (CancelableActionCancelKeyText)
	{
		CancelableActionCancelKeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));
		CancelableActionCancelKeyText->SetJustification(ETextJustify::Center);
		if (bCreatedCancelKeyText)
		{
			CancelableActionCancelKeyText->SetVisibility(ESlateVisibility::Collapsed);
		}
		TunaSweeperUIFont::ApplyFont(CancelableActionCancelKeyText, 12.0f, ETunaSweeperUIFontWeight::Bold);
	}

	if (CancelableActionCancelText)
	{
		CancelableActionCancelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 1.0f, 1.0f)));
		CancelableActionCancelText->SetShadowOffset(FVector2D(0.0f, 1.0f));
		CancelableActionCancelText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		CancelableActionCancelText->SetJustification(ETextJustify::Center);
		if (bCreatedCancelText)
		{
			CancelableActionCancelText->SetVisibility(ESlateVisibility::Collapsed);
		}
		TunaSweeperUIFont::ApplyFont(CancelableActionCancelText, 14.0f, ETunaSweeperUIFontWeight::Bold);
		if (CancelableActionPromptRoot && !CancelableActionCancelText->Slot)
		{
			UHorizontalBoxSlot* TextSlot = CancelableActionPromptRoot->AddChildToHorizontalBox(CancelableActionCancelText);
			if (TextSlot)
			{
				TextSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
				TextSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}
}

void UTunaSweeperHudQuickSlotBarWidget::ApplyCancelableActionLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (USizeBox* RootSizeBox = Cast<USizeBox>(WidgetTree->FindWidget(FName(TEXT("RootSizeBox")))))
	{
		RootSizeBox->SetWidthOverride(QuickSlotRootWidth);
		RootSizeBox->SetHeightOverride(QuickSlotRootHeight);
	}

	if (UCanvasPanelSlot* ParentCanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		ParentCanvasSlot->SetSize(FVector2D(QuickSlotRootWidth, QuickSlotRootHeight));
	}

	if (CancelableActionPromptRoot)
	{
		if (UCanvasPanelSlot* PromptSlot = Cast<UCanvasPanelSlot>(CancelableActionPromptRoot->Slot))
		{
			PromptSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			PromptSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			PromptSlot->SetPosition(FVector2D(0.0f, CancelableActionPromptOffsetY));
			PromptSlot->SetAutoSize(true);
			PromptSlot->SetZOrder(12);
		}
	}

	if (CancelableActionCancelKeyBackground)
	{
		CancelableActionCancelKeyBackground->SetPadding(FMargin(8.0f, 2.0f));
		CancelableActionCancelKeyBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(28.0f, 24.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			0.0f,
			7.0f));
		if (UHorizontalBoxSlot* KeySlot = Cast<UHorizontalBoxSlot>(CancelableActionCancelKeyBackground->Slot))
		{
			KeySlot->SetPadding(FMargin(0.0f));
			KeySlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (CancelableActionCancelKeyText)
	{
		CancelableActionCancelKeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));
		CancelableActionCancelKeyText->SetJustification(ETextJustify::Center);
		TunaSweeperUIFont::ApplyFont(CancelableActionCancelKeyText, 12.0f, ETunaSweeperUIFontWeight::Bold);
	}

	if (CancelableActionCancelText)
	{
		CancelableActionCancelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 1.0f, 1.0f)));
		CancelableActionCancelText->SetShadowOffset(FVector2D(0.0f, 1.0f));
		CancelableActionCancelText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		CancelableActionCancelText->SetJustification(ETextJustify::Center);
		TunaSweeperUIFont::ApplyFont(CancelableActionCancelText, 14.0f, ETunaSweeperUIFontWeight::Bold);
		if (UHorizontalBoxSlot* TextSlot = Cast<UHorizontalBoxSlot>(CancelableActionCancelText->Slot))
		{
			TextSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (USizeBox* ProgressSizeBox = Cast<USizeBox>(CancelableActionProgressPanel))
	{
		ProgressSizeBox->SetWidthOverride(CancelableActionProgressWidth);
		ProgressSizeBox->SetHeightOverride(CancelableActionProgressHeight);
	}

	if (CancelableActionProgressPanel)
	{
		if (UCanvasPanelSlot* ProgressSlot = Cast<UCanvasPanelSlot>(CancelableActionProgressPanel->Slot))
		{
			ProgressSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			ProgressSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			ProgressSlot->SetPosition(FVector2D(0.0f, CancelableActionProgressOffsetY));
			ProgressSlot->SetSize(FVector2D(CancelableActionProgressWidth, CancelableActionProgressHeight));
			ProgressSlot->SetZOrder(11);
		}
	}

	if (CancelableActionProgressBar)
	{
		CancelableActionProgressBar->SetWidgetStyle(MakeCancelableActionProgressBarStyle());
		CancelableActionProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
		CancelableActionProgressBar->SetFillColorAndOpacity(FLinearColor::White);
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
