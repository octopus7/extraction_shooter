#include "UI/TunaSweeperOptionRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "UI/TunaSweeperUIStyle.h"

namespace TunaSweeperOptionRowWidget
{
	constexpr float LabelWidth = 250.0f;
	constexpr float ValueWidth = 280.0f;
	constexpr float ArrowWidth = 44.0f;
	constexpr float RowHeight = 42.0f;
}

TSharedRef<SWidget> UTunaSweeperOptionRowWidget::RebuildWidget()
{
	BuildRuntimeWidgetTree();
	return Super::RebuildWidget();
}

void UTunaSweeperOptionRowWidget::BuildRuntimeWidgetTree()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return;
	}
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OptionRowRoot"));
	WidgetTree->RootWidget = Root;
	OptionLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OptionLabelText"));
	PreviousButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PreviousButton"));
	PreviousButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PreviousButtonText"));
	OptionValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OptionValueText"));
	NextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextButton"));
	NextButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NextButtonText"));

	PreviousButton->SetContent(PreviousButtonText);
	NextButton->SetContent(NextButtonText);

	USizeBox* LabelColumn = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("OptionLabelColumn"));
	LabelColumn->SetWidthOverride(TunaSweeperOptionRowWidget::LabelWidth);
	LabelColumn->SetMinDesiredHeight(TunaSweeperOptionRowWidget::RowHeight);
	UHorizontalBox* FittedLabelRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OptionFittedLabelRow"));
	UScaleBox* LabelScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("OptionLabelScale"));
	LabelScale->SetStretch(EStretch::ScaleToFit);
	LabelScale->SetStretchDirection(EStretchDirection::DownOnly);
	LabelScale->SetContent(OptionLabelText);
	if (UScaleBoxSlot* ScaleContentSlot = Cast<UScaleBoxSlot>(LabelScale->GetContentSlot()))
	{
		ScaleContentSlot->SetHorizontalAlignment(HAlign_Left);
		ScaleContentSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* ScaleRowSlot = FittedLabelRow->AddChildToHorizontalBox(LabelScale))
	{
		ScaleRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScaleRowSlot->SetVerticalAlignment(VAlign_Center);
	}
	USpacer* RightReserve = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("OptionLabelRightReserve"));
	RightReserve->SetSize(FVector2D(8.0f, 1.0f));
	FittedLabelRow->AddChildToHorizontalBox(RightReserve);
	LabelColumn->SetContent(FittedLabelRow);
	if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(LabelColumn->GetContentSlot()))
	{
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* AddedSlot = Root->AddChildToHorizontalBox(LabelColumn))
	{
		AddedSlot->SetVerticalAlignment(VAlign_Center);
	}

	auto AddArrow = [this, Root](UButton* Button, const TCHAR* Name)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetWidthOverride(TunaSweeperOptionRowWidget::ArrowWidth);
		Box->SetHeightOverride(TunaSweeperOptionRowWidget::RowHeight);
		Box->SetContent(Button);
		if (UHorizontalBoxSlot* AddedSlot = Root->AddChildToHorizontalBox(Box))
		{
			AddedSlot->SetVerticalAlignment(VAlign_Center);
		}
	};
	AddArrow(PreviousButton, TEXT("PreviousButtonBox"));

	USizeBox* ValueColumn = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("OptionValueColumn"));
	ValueColumn->SetWidthOverride(TunaSweeperOptionRowWidget::ValueWidth);
	ValueColumn->SetHeightOverride(TunaSweeperOptionRowWidget::RowHeight);
	ValueColumn->SetContent(OptionValueText);
	if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(ValueColumn->GetContentSlot()))
	{
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* AddedSlot = Root->AddChildToHorizontalBox(ValueColumn))
	{
		AddedSlot->SetVerticalAlignment(VAlign_Center);
	}

	AddArrow(NextButton, TEXT("NextButtonBox"));
	ApplyPresentation();
}

void UTunaSweeperOptionRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (PreviousButton)
	{
		PreviousButton->OnClicked.RemoveDynamic(this, &UTunaSweeperOptionRowWidget::HandlePreviousClicked);
		PreviousButton->OnClicked.AddDynamic(this, &UTunaSweeperOptionRowWidget::HandlePreviousClicked);
	}
	if (NextButton)
	{
		NextButton->OnClicked.RemoveDynamic(this, &UTunaSweeperOptionRowWidget::HandleNextClicked);
		NextButton->OnClicked.AddDynamic(this, &UTunaSweeperOptionRowWidget::HandleNextClicked);
	}
	ApplyPresentation();
}

void UTunaSweeperOptionRowWidget::Configure(const FText& InLabel)
{
	Label = InLabel;
	ApplyPresentation();
}

void UTunaSweeperOptionRowWidget::SetValue(const FText& InValue)
{
	Value = InValue;
	ApplyPresentation();
}

void UTunaSweeperOptionRowWidget::SetStepEnabled(bool bCanStepPrevious, bool bCanStepNext)
{
	bPreviousEnabled = bCanStepPrevious;
	bNextEnabled = bCanStepNext;
	if (PreviousButton)
	{
		PreviousButton->SetIsEnabled(bPreviousEnabled);
	}
	if (NextButton)
	{
		NextButton->SetIsEnabled(bNextEnabled);
	}
}

void UTunaSweeperOptionRowWidget::ApplyPresentation()
{
	if (OptionLabelText)
	{
		OptionLabelText->SetText(Label);
		TunaSweeperUIStyle::ApplyLabel(OptionLabelText, 18);
	}
	if (OptionValueText)
	{
		OptionValueText->SetText(Value);
		OptionValueText->SetJustification(ETextJustify::Center);
		TunaSweeperUIStyle::ApplyLabel(OptionValueText, 18);
	}
	if (PreviousButtonText)
	{
		PreviousButtonText->SetText(FText::FromString(TEXT("<")));
		PreviousButtonText->SetJustification(ETextJustify::Center);
		TunaSweeperUIStyle::ApplyLabel(PreviousButtonText, 20);
	}
	if (NextButtonText)
	{
		NextButtonText->SetText(FText::FromString(TEXT(">")));
		NextButtonText->SetJustification(ETextJustify::Center);
		TunaSweeperUIStyle::ApplyLabel(NextButtonText, 20);
	}
	if (PreviousButton)
	{
		TunaSweeperUIStyle::ApplyButton(PreviousButton, TunaSweeperUIStyle::EButtonRole::Icon);
		PreviousButton->SetIsEnabled(bPreviousEnabled);
	}
	if (NextButton)
	{
		TunaSweeperUIStyle::ApplyButton(NextButton, TunaSweeperUIStyle::EButtonRole::Icon);
		NextButton->SetIsEnabled(bNextEnabled);
	}
}

void UTunaSweeperOptionRowWidget::HandlePreviousClicked()
{
	OnStepRequested.Broadcast(-1);
}

void UTunaSweeperOptionRowWidget::HandleNextClicked()
{
	OnStepRequested.Broadcast(1);
}
