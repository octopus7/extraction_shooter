#include "UI/TunaSweeperGraphicsQualityRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "UI/TunaSweeperUIStyle.h"

namespace TunaSweeperGraphicsQualityRowWidget
{
	constexpr float LabelWidth = 250.0f;
	constexpr float ValueWidth = 280.0f;
	constexpr float ArrowWidth = 44.0f;
	constexpr float RowHeight = 42.0f;
}

TSharedRef<SWidget> UTunaSweeperGraphicsQualityRowWidget::RebuildWidget()
{
	BuildRuntimeWidgetTree();
	EnsureFittedLabelColumn();
	EnsureFixedValueColumn();
	return Super::RebuildWidget();
}

void UTunaSweeperGraphicsQualityRowWidget::BuildRuntimeWidgetTree()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return;
	}
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QualityRowRoot"));
	WidgetTree->RootWidget = Root;
	OptionLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OptionLabelText"));
	PreviousButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PreviousButton"));
	PreviousButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PreviousButtonText"));
	QualityValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QualityValueText"));
	NextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextButton"));
	NextButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NextButtonText"));

	PreviousButton->SetContent(PreviousButtonText);
	NextButton->SetContent(NextButtonText);

	USizeBox* LabelColumn = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QualityLabelColumn"));
	LabelColumn->SetWidthOverride(TunaSweeperGraphicsQualityRowWidget::LabelWidth);
	LabelColumn->SetMinDesiredHeight(TunaSweeperGraphicsQualityRowWidget::RowHeight);
	LabelColumn->SetContent(OptionLabelText);
	if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(LabelColumn->GetContentSlot()))
	{
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* RowSlot = Root->AddChildToHorizontalBox(LabelColumn))
	{
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}
	auto AddArrow = [this, Root](UButton* Button, const TCHAR* Name)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetWidthOverride(TunaSweeperGraphicsQualityRowWidget::ArrowWidth);
		Box->SetHeightOverride(TunaSweeperGraphicsQualityRowWidget::RowHeight);
		Box->SetContent(Button);
		Root->AddChildToHorizontalBox(Box)->SetVerticalAlignment(VAlign_Center);
	};
	AddArrow(PreviousButton, TEXT("PreviousButtonBox"));
	USizeBox* ValueColumn = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QualityValueColumn"));
	ValueColumn->SetWidthOverride(TunaSweeperGraphicsQualityRowWidget::ValueWidth);
	ValueColumn->SetHeightOverride(TunaSweeperGraphicsQualityRowWidget::RowHeight);
	ValueColumn->SetContent(QualityValueText);
	if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(ValueColumn->GetContentSlot()))
	{
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
	Root->AddChildToHorizontalBox(ValueColumn)->SetVerticalAlignment(VAlign_Center);
	AddArrow(NextButton, TEXT("NextButtonBox"));
	EnsureFittedLabelColumn();
	ApplyPresentation();
}

void UTunaSweeperGraphicsQualityRowWidget::Configure(
	ETunaSweeperScalabilityOption InOption,
	const FText& InLabel)
{
	Option = InOption;
	Label = InLabel;
	ApplyPresentation();
}

void UTunaSweeperGraphicsQualityRowWidget::SetQualityLevel(
	int32 InQualityLevel,
	const FText& InQualityText)
{
	QualityLevel = FMath::Clamp(InQualityLevel, 0, 3);
	QualityText = InQualityText;
	ApplyPresentation();
}

void UTunaSweeperGraphicsQualityRowWidget::SetStepEnabled(bool bCanStepPrevious, bool bCanStepNext)
{
	bPreviousEnabled = bCanStepPrevious;
	bNextEnabled = bCanStepNext;
	if (PreviousButton) PreviousButton->SetIsEnabled(bPreviousEnabled);
	if (NextButton) NextButton->SetIsEnabled(bNextEnabled);
}

void UTunaSweeperGraphicsQualityRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureFittedLabelColumn();
	EnsureFixedValueColumn();

	if (PreviousButton)
	{
		PreviousButton->OnClicked.RemoveDynamic(this, &UTunaSweeperGraphicsQualityRowWidget::HandlePreviousClicked);
		PreviousButton->OnClicked.AddDynamic(this, &UTunaSweeperGraphicsQualityRowWidget::HandlePreviousClicked);
	}
	if (NextButton)
	{
		NextButton->OnClicked.RemoveDynamic(this, &UTunaSweeperGraphicsQualityRowWidget::HandleNextClicked);
		NextButton->OnClicked.AddDynamic(this, &UTunaSweeperGraphicsQualityRowWidget::HandleNextClicked);
	}
	ApplyPresentation();
}

void UTunaSweeperGraphicsQualityRowWidget::EnsureFittedLabelColumn()
{
	if (!OptionLabelText && WidgetTree)
	{
		OptionLabelText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("OptionLabelText")));
	}
	if (!WidgetTree || !OptionLabelText || Cast<UScaleBox>(OptionLabelText->GetParent()))
	{
		return;
	}
	USizeBox* LabelColumn = Cast<USizeBox>(OptionLabelText->GetParent());
	if (!LabelColumn)
	{
		return;
	}

	LabelColumn->RemoveChild(OptionLabelText);
	UHorizontalBox* FittedLabelRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QualityFittedLabelRow"));
	UScaleBox* LabelScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("QualityLabelScale"));
	LabelScale->SetStretch(EStretch::ScaleToFit);
	LabelScale->SetStretchDirection(EStretchDirection::DownOnly);
	LabelScale->SetContent(OptionLabelText);
	if (UScaleBoxSlot* ScaleSlot = Cast<UScaleBoxSlot>(LabelScale->GetContentSlot()))
	{
		ScaleSlot->SetHorizontalAlignment(HAlign_Left);
		ScaleSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* ScaleRowSlot = FittedLabelRow->AddChildToHorizontalBox(LabelScale))
	{
		ScaleRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScaleRowSlot->SetVerticalAlignment(VAlign_Center);
	}
	USpacer* RightReserve = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("QualityLabelRightReserve"));
	RightReserve->SetSize(FVector2D(8.0f, 1.0f));
	FittedLabelRow->AddChildToHorizontalBox(RightReserve);
	LabelColumn->SetContent(FittedLabelRow);
	if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(LabelColumn->GetContentSlot()))
	{
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UTunaSweeperGraphicsQualityRowWidget::EnsureFixedValueColumn()
{
	if (!QualityValueText && WidgetTree)
	{
		QualityValueText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QualityValueText")));
	}
	if (!WidgetTree || !QualityValueText || Cast<USizeBox>(QualityValueText->GetParent()))
	{
		return;
	}
	UHorizontalBox* Parent = Cast<UHorizontalBox>(QualityValueText->GetParent());
	if (!Parent)
	{
		return;
	}
	const int32 Index = Parent->GetChildIndex(QualityValueText);
	Parent->RemoveChild(QualityValueText);
	USizeBox* ValueColumn = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QualityValueColumn"));
	ValueColumn->SetWidthOverride(TunaSweeperGraphicsQualityRowWidget::ValueWidth);
	ValueColumn->SetHeightOverride(TunaSweeperGraphicsQualityRowWidget::RowHeight);
	ValueColumn->SetContent(QualityValueText);
	if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(ValueColumn->GetContentSlot()))
	{
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* AddedSlot = Cast<UHorizontalBoxSlot>(Parent->InsertChildAt(Index, ValueColumn)))
	{
		AddedSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UTunaSweeperGraphicsQualityRowWidget::ApplyPresentation()
{
	if (OptionLabelText)
	{
		OptionLabelText->SetText(Label);
		TunaSweeperUIStyle::ApplyLabel(OptionLabelText, 18);
	}
	if (QualityValueText)
	{
		QualityValueText->SetText(QualityText);
		QualityValueText->SetJustification(ETextJustify::Center);
		TunaSweeperUIStyle::ApplyLabel(QualityValueText, 18);
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

void UTunaSweeperGraphicsQualityRowWidget::HandlePreviousClicked()
{
	OnQualityStepRequested.Broadcast(Option, -1);
}

void UTunaSweeperGraphicsQualityRowWidget::HandleNextClicked()
{
	OnQualityStepRequested.Broadcast(Option, 1);
}
