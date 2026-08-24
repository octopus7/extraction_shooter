#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Settings/TunaSweeperBuildFlavor.h"

void UTunaSweeperIntroMenuWidget::EnsureAlwaysNewStartButton()
{
	if (AlwaysNewStartButton)
	{
		if (!AlwaysNewStartButtonContainer)
		{
			AlwaysNewStartButtonContainer = AlwaysNewStartButton;
		}
		if (AlwaysNewStartButtonText)
		{
			AlwaysNewStartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.always_new_start")),
				FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
		}
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	USizeBox* AlwaysNewStartButtonBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("AlwaysNewStartButtonBox"));
	AlwaysNewStartButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("AlwaysNewStartButton"));
	AlwaysNewStartButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("AlwaysNewStartButtonText"));

	if (!AlwaysNewStartButtonBox || !AlwaysNewStartButton || !AlwaysNewStartButtonText)
	{
		return;
	}

	auto MakeBoxBrush = [](const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Color);
		return Brush;
	};

	FButtonStyle DebugButtonStyle;
	DebugButtonStyle.SetNormal(MakeBoxBrush(FLinearColor(0.34f, 0.03f, 0.02f, 0.76f)));
	DebugButtonStyle.SetHovered(MakeBoxBrush(FLinearColor(0.58f, 0.06f, 0.04f, 0.90f)));
	DebugButtonStyle.SetPressed(MakeBoxBrush(FLinearColor(0.22f, 0.02f, 0.015f, 0.95f)));
	DebugButtonStyle.SetNormalPadding(FMargin(0.0f));
	DebugButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
	AlwaysNewStartButton->SetStyle(DebugButtonStyle);
	AlwaysNewStartButton->SetClickMethod(EButtonClickMethod::DownAndUp);

	AlwaysNewStartButtonText->SetText(ResolveUiText(
		FName(TEXT("ui.title.always_new_start")),
		FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
	AlwaysNewStartButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.92f, 0.88f, 1.0f)));
	AlwaysNewStartButtonText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(AlwaysNewStartButtonText, 16, ETunaSweeperUIFontWeight::Bold);

	AlwaysNewStartButtonBox->SetWidthOverride(180.0f);
	AlwaysNewStartButtonBox->SetHeightOverride(42.0f);
	AlwaysNewStartButton->SetContent(AlwaysNewStartButtonText);
	AlwaysNewStartButtonBox->SetContent(AlwaysNewStartButton);
	AlwaysNewStartButtonContainer = AlwaysNewStartButtonBox;

	UCanvasPanelSlot* DebugButtonSlot = RootCanvas->AddChildToCanvas(AlwaysNewStartButtonBox);
	if (DebugButtonSlot)
	{
		DebugButtonSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		DebugButtonSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		DebugButtonSlot->SetPosition(FVector2D(-36.0f, 36.0f));
		DebugButtonSlot->SetSize(FVector2D(180.0f, 42.0f));
		DebugButtonSlot->SetZOrder(200);
	}
}

void UTunaSweeperIntroMenuWidget::SetAlwaysNewStartButtonVisible(bool bVisible)
{
	bVisible = bVisible && !TunaSweeperBuildFlavor::IsDemo();
	EnsureAlwaysNewStartButton();
	if (AlwaysNewStartButtonContainer)
	{
		AlwaysNewStartButtonContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (AlwaysNewStartButton)
	{
		AlwaysNewStartButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
