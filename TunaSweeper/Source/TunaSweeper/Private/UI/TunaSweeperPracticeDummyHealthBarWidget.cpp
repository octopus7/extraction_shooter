#include "UI/TunaSweeperPracticeDummyHealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Styling/SlateBrush.h"

TSharedRef<SWidget> UTunaSweeperPracticeDummyHealthBarWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UTunaSweeperPracticeDummyHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshHealthBar();
}

void UTunaSweeperPracticeDummyHealthBarWidget::SetHealthFraction(float InHealthFraction)
{
	HealthFraction = FMath::Clamp(InHealthFraction, 0.0f, 1.0f);
	RefreshHealthBar();
}

void UTunaSweeperPracticeDummyHealthBarWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PracticeDummyHealthRoot"));
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PracticeDummyHealthBackground"));
	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PracticeDummyHealthBar"));

	WidgetTree->RootWidget = RootSizeBox;
	if (!RootSizeBox || !BackgroundBorder || !HealthProgressBar)
	{
		return;
	}

	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.86f));
	BackgroundBrush.OutlineSettings = FSlateBrushOutlineSettings(
		3.0f,
		FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f)),
		1.0f);
	BackgroundBrush.OutlineSettings.bUseBrushTransparency = false;

	RootSizeBox->SetWidthOverride(138.0f);
	RootSizeBox->SetHeightOverride(13.0f);
	RootSizeBox->SetContent(BackgroundBorder);

	BackgroundBorder->SetBrush(BackgroundBrush);
	BackgroundBorder->SetPadding(FMargin(2.0f));
	BackgroundBorder->SetContent(HealthProgressBar);

	HealthProgressBar->SetFillColorAndOpacity(FLinearColor(0.24f, 1.0f, 0.42f, 1.0f));
	HealthProgressBar->SetPercent(HealthFraction);
}

void UTunaSweeperPracticeDummyHealthBarWidget::RefreshHealthBar()
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(HealthFraction);
	}
}
