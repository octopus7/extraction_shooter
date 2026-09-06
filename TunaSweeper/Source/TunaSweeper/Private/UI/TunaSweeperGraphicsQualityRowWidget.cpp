#include "UI/TunaSweeperGraphicsQualityRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/TunaSweeperUIFont.h"

TSharedRef<SWidget> UTunaSweeperGraphicsQualityRowWidget::RebuildWidget()
{
	BuildRuntimeWidgetTree();
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

	TunaSweeperUIFont::ApplyFont(OptionLabelText, 14);
	TunaSweeperUIFont::ApplyFont(PreviousButtonText, 14);
	TunaSweeperUIFont::ApplyFont(QualityValueText, 14);
	TunaSweeperUIFont::ApplyFont(NextButtonText, 14);
	OptionLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.91f, 0.92f, 1.0f)));
	QualityValueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	QualityValueText->SetJustification(ETextJustify::Center);
	PreviousButtonText->SetText(FText::FromString(TEXT("<")));
	NextButtonText->SetText(FText::FromString(TEXT(">")));
	PreviousButton->SetContent(PreviousButtonText);
	NextButton->SetContent(NextButtonText);
	// Quiet hit areas: the arrow is primary; fill only becomes apparent on interaction.
	for (UButton* Button : { PreviousButton.Get(), NextButton.Get() })
	{
		auto Fill = [](FLinearColor Color) {
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = Color;
			return Brush;
		};
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(Fill(FLinearColor(0.005f, 0.015f, 0.017f, 0.16f)));
		Style.SetHovered(Fill(FLinearColor(0.24f, 0.40f, 0.34f, 0.38f)));
		Style.SetPressed(Fill(FLinearColor(0.36f, 0.52f, 0.42f, 0.50f)));
		Style.SetDisabled(Fill(FLinearColor::Transparent));
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::White);
	}

	if (UHorizontalBoxSlot* RowSlot = Root->AddChildToHorizontalBox(OptionLabelText))
	{
		RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}
	Root->AddChildToHorizontalBox(PreviousButton)->SetPadding(FMargin(8.0f, 2.0f));
	if (UHorizontalBoxSlot* RowSlot = Root->AddChildToHorizontalBox(QualityValueText))
	{
		RowSlot->SetPadding(FMargin(12.0f, 2.0f));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}
	Root->AddChildToHorizontalBox(NextButton)->SetPadding(FMargin(8.0f, 2.0f));
}

void UTunaSweeperGraphicsQualityRowWidget::Configure(
	ETunaSweeperScalabilityOption InOption,
	const FText& InLabel)
{
	Option = InOption;
	Label = InLabel;
	if (OptionLabelText)
	{
		OptionLabelText->SetText(Label);
	}
}

void UTunaSweeperGraphicsQualityRowWidget::SetQualityLevel(
	int32 InQualityLevel,
	const FText& InQualityText)
{
	QualityLevel = FMath::Clamp(InQualityLevel, 0, 3);
	if (QualityValueText)
	{
		QualityValueText->SetText(InQualityText);
	}
}

void UTunaSweeperGraphicsQualityRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
	if (PreviousButtonText)
	{
		PreviousButtonText->SetText(FText::FromString(TEXT("<")));
	}
	if (NextButtonText)
	{
		NextButtonText->SetText(FText::FromString(TEXT(">")));
	}
	if (OptionLabelText)
	{
		OptionLabelText->SetText(Label);
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
