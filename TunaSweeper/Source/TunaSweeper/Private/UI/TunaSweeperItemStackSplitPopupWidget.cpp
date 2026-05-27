#include "UI/TunaSweeperItemStackSplitPopupWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperStackSplitPopup
{
	constexpr float PopupWidth = 232.0f;
	constexpr float PopupHeight = 152.0f;
	const FVector2D PopupCursorOffset(14.0f, 14.0f);

	FSlateBrush MakePopupBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		float Radius,
		const FLinearColor& OutlineColor,
		float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateChildSize MakeSlateChildSize(ESlateSizeRule::Type SizeRule, float Value = 1.0f)
	{
		FSlateChildSize ChildSize;
		ChildSize.SizeRule = SizeRule;
		ChildSize.Value = Value;
		return ChildSize;
	}

	FVector2D ResolveViewportPosition(UObject* WorldContextObject, const FVector2D& ScreenSpacePosition)
	{
		if (WorldContextObject)
		{
			return UWidgetLayoutLibrary::GetMousePositionOnViewport(WorldContextObject) + PopupCursorOffset;
		}

		return ScreenSpacePosition + PopupCursorOffset;
	}

	FText ResolveUiText(const UTunaSweeperGameInstance* TunaGameInstance, const TCHAR* StringKey, const TCHAR* Fallback)
	{
		return TunaGameInstance
			? TunaGameInstance->ResolveLocalizedText(FName(StringKey), FText::FromString(Fallback))
			: FText::FromString(Fallback);
	}
}

bool UTunaSweeperItemStackSplitPopupWidget::TryOpenStackSplitPopup(
	APlayerController* OwningPlayer,
	UTunaSweeperGameInstance* InTunaGameInstance,
	const FTunaSweeperItemSlotReference& InSourceSlot,
	const FTunaSweeperItemSlotReference& InTargetSlot,
	const FVector2D& ScreenSpacePosition)
{
	if (!OwningPlayer || !InTunaGameInstance)
	{
		return false;
	}

	int32 DefaultQuantity = 0;
	int32 MaxQuantity = 0;
	if (!InTunaGameInstance->CanSplitItemStackBetweenSlots(
		InSourceSlot,
		InTargetSlot,
		DefaultQuantity,
		MaxQuantity) ||
		DefaultQuantity <= 0 ||
		MaxQuantity <= 0)
	{
		return false;
	}

	UTunaSweeperItemStackSplitPopupWidget* PopupWidget =
		CreateWidget<UTunaSweeperItemStackSplitPopupWidget>(
			OwningPlayer,
			UTunaSweeperItemStackSplitPopupWidget::StaticClass());
	if (!PopupWidget)
	{
		return false;
	}

	PopupWidget->ConfigureSplit(
		InTunaGameInstance,
		InSourceSlot,
		InTargetSlot,
		DefaultQuantity,
		MaxQuantity);
	PopupWidget->AddToViewport(95);
	PopupWidget->SetDesiredSizeInViewport(FVector2D(
		TunaSweeperStackSplitPopup::PopupWidth,
		TunaSweeperStackSplitPopup::PopupHeight));
	PopupWidget->SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
	PopupWidget->SetPositionInViewport(
		TunaSweeperStackSplitPopup::ResolveViewportPosition(OwningPlayer, ScreenSpacePosition),
		false);
	PopupWidget->SetKeyboardFocus();
	return true;
}

void UTunaSweeperItemStackSplitPopupWidget::ConfigureSplit(
	UTunaSweeperGameInstance* InTunaGameInstance,
	const FTunaSweeperItemSlotReference& InSourceSlot,
	const FTunaSweeperItemSlotReference& InTargetSlot,
	int32 InDefaultSplitQuantity,
	int32 InMaxSplitQuantity)
{
	TunaGameInstance = InTunaGameInstance;
	SourceSlot = InSourceSlot;
	TargetSlot = InTargetSlot;
	MaxSplitQuantity = FMath::Max(1, InMaxSplitQuantity);
	DefaultSplitQuantity = FMath::Clamp(InDefaultSplitQuantity, 1, MaxSplitQuantity);
	RefreshSplitText();
}

TSharedRef<SWidget> UTunaSweeperItemStackSplitPopupWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildSplitPopupWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperItemStackSplitPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildSplitPopupWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (OkButton)
	{
		OkButton->OnClicked.RemoveDynamic(this, &UTunaSweeperItemStackSplitPopupWidget::HandleOkClicked);
		OkButton->OnClicked.AddDynamic(this, &UTunaSweeperItemStackSplitPopupWidget::HandleOkClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveDynamic(this, &UTunaSweeperItemStackSplitPopupWidget::HandleCancelClicked);
		CancelButton->OnClicked.AddDynamic(this, &UTunaSweeperItemStackSplitPopupWidget::HandleCancelClicked);
	}

	if (UTunaSweeperGameInstance* PinnedGameInstance = TunaGameInstance.Get())
	{
		PinnedGameInstance->OnLanguageChanged.RemoveAll(this);
		PinnedGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperItemStackSplitPopupWidget::HandleLanguageChanged);
	}

	RefreshSplitText();
	if (QuantityTextBox)
	{
		QuantityTextBox->SetKeyboardFocus();
	}
}

void UTunaSweeperItemStackSplitPopupWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* PinnedGameInstance = TunaGameInstance.Get())
	{
		PinnedGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UTunaSweeperItemStackSplitPopupWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		ConfirmSplit();
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RemoveFromParent();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UTunaSweeperItemStackSplitPopupWidget::BuildSplitPopupWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StackSplitPopupRoot"));
	UVerticalBox* RootStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StackSplitPopupStack"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StackSplitPopupTitleText"));
	GuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StackSplitPopupGuideText"));
	QuantityTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("StackSplitPopupQuantityTextBox"));
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StackSplitPopupButtonRow"));
	OkButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StackSplitPopupOkButton"));
	OkButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StackSplitPopupOkButtonText"));
	CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StackSplitPopupCancelButton"));
	CancelButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StackSplitPopupCancelButtonText"));
	if (!RootPanel ||
		!RootStack ||
		!TitleText ||
		!GuideText ||
		!QuantityTextBox ||
		!ButtonRow ||
		!OkButton ||
		!OkButtonText ||
		!CancelButton ||
		!CancelButtonText)
	{
		return;
	}

	WidgetTree->RootWidget = RootPanel;
	RootPanel->SetPadding(FMargin(12.0f));
	RootPanel->SetBrush(TunaSweeperStackSplitPopup::MakePopupBrush(
		FVector2D(TunaSweeperStackSplitPopup::PopupWidth, TunaSweeperStackSplitPopup::PopupHeight),
		FLinearColor(0.035f, 0.04f, 0.045f, 0.96f),
		5.0f,
		FLinearColor(0.48f, 0.56f, 0.54f, 0.72f),
		1.0f));
	RootPanel->SetContent(RootStack);

	TitleText->SetText(TunaSweeperStackSplitPopup::ResolveUiText(
		TunaGameInstance.Get(),
		TEXT("ui.split.title"),
		TEXT("\uC218\uB7C9 \uB098\uB204\uAE30")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.97f, 0.95f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(TitleText, 17, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* TitleSlot = RootStack->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	GuideText->SetColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.84f, 0.82f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(GuideText, 13);
	if (UVerticalBoxSlot* GuideSlot = RootStack->AddChildToVerticalBox(GuideText))
	{
		GuideSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	QuantityTextBox->SetSelectAllTextWhenFocused(true);
	QuantityTextBox->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* QuantitySlot = RootStack->AddChildToVerticalBox(QuantityTextBox))
	{
		QuantitySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	OkButton->SetContent(OkButtonText);
	OkButton->SetBackgroundColor(FLinearColor(0.56f, 0.84f, 0.92f, 1.0f));
	OkButtonText->SetText(TunaSweeperStackSplitPopup::ResolveUiText(
		TunaGameInstance.Get(),
		TEXT("ui.common.ok"),
		TEXT("OK")));
	OkButtonText->SetJustification(ETextJustify::Center);
	OkButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.035f, 0.04f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(OkButtonText, 15, ETunaSweeperUIFontWeight::Bold);
	if (UHorizontalBoxSlot* OkSlot = ButtonRow->AddChildToHorizontalBox(OkButton))
	{
		OkSlot->SetSize(TunaSweeperStackSplitPopup::MakeSlateChildSize(ESlateSizeRule::Fill));
		OkSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}

	CancelButton->SetContent(CancelButtonText);
	CancelButton->SetBackgroundColor(FLinearColor(0.18f, 0.20f, 0.21f, 1.0f));
	CancelButtonText->SetText(TunaSweeperStackSplitPopup::ResolveUiText(
		TunaGameInstance.Get(),
		TEXT("ui.common.cancel"),
		TEXT("\uCDE8\uC18C")));
	CancelButtonText->SetJustification(ETextJustify::Center);
	CancelButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.86f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(CancelButtonText, 15, ETunaSweeperUIFontWeight::Bold);
	if (UHorizontalBoxSlot* CancelSlot = ButtonRow->AddChildToHorizontalBox(CancelButton))
	{
		CancelSlot->SetSize(TunaSweeperStackSplitPopup::MakeSlateChildSize(ESlateSizeRule::Fill));
	}

	RootStack->AddChildToVerticalBox(ButtonRow);
}

void UTunaSweeperItemStackSplitPopupWidget::RefreshSplitText()
{
	if (TitleText)
	{
		TitleText->SetText(TunaSweeperStackSplitPopup::ResolveUiText(
			TunaGameInstance.Get(),
			TEXT("ui.split.title"),
			TEXT("\uC218\uB7C9 \uB098\uB204\uAE30")));
	}

	if (GuideText)
	{
		GuideText->SetText(FText::Format(
			TunaSweeperStackSplitPopup::ResolveUiText(
				TunaGameInstance.Get(),
				TEXT("ui.split.guide"),
				TEXT("\uB098\uB20C \uC218\uB7C9\uC744 \uC785\uB825\uD558\uC138\uC694 (1-{0})")),
			FText::AsNumber(FMath::Max(1, MaxSplitQuantity))));
	}

	if (QuantityTextBox)
	{
		QuantityTextBox->SetText(FText::AsNumber(FMath::Clamp(DefaultSplitQuantity, 1, FMath::Max(1, MaxSplitQuantity))));
	}

	if (OkButtonText)
	{
		OkButtonText->SetText(TunaSweeperStackSplitPopup::ResolveUiText(
			TunaGameInstance.Get(),
			TEXT("ui.common.ok"),
			TEXT("OK")));
	}

	if (CancelButtonText)
	{
		CancelButtonText->SetText(TunaSweeperStackSplitPopup::ResolveUiText(
			TunaGameInstance.Get(),
			TEXT("ui.common.cancel"),
			TEXT("\uCDE8\uC18C")));
	}
}

void UTunaSweeperItemStackSplitPopupWidget::HandleLanguageChanged()
{
	RefreshSplitText();
}

void UTunaSweeperItemStackSplitPopupWidget::ConfirmSplit()
{
	UTunaSweeperGameInstance* PinnedGameInstance = TunaGameInstance.Get();
	if (PinnedGameInstance)
	{
		PinnedGameInstance->SplitItemStackBetweenSlots(SourceSlot, TargetSlot, GetRequestedSplitQuantity());
	}

	RemoveFromParent();
}

int32 UTunaSweeperItemStackSplitPopupWidget::GetRequestedSplitQuantity() const
{
	if (!QuantityTextBox)
	{
		return DefaultSplitQuantity;
	}

	const FString RawText = QuantityTextBox->GetText().ToString().TrimStartAndEnd();
	const int32 RequestedQuantity = FCString::Atoi(*RawText);
	return FMath::Clamp(RequestedQuantity, 1, FMath::Max(1, MaxSplitQuantity));
}

void UTunaSweeperItemStackSplitPopupWidget::HandleOkClicked()
{
	ConfirmSplit();
}

void UTunaSweeperItemStackSplitPopupWidget::HandleCancelClicked()
{
	RemoveFromParent();
}
