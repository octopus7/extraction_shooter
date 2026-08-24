#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Brushes/SlateColorBrush.h"
#include "Settings/TunaSweeperBuildFlavor.h"

void UTunaSweeperIntroMenuWidget::ResetTitleViewportLayoutState()
{
	auto ResetWidgetTransform = [](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		Widget->SetRenderTransform(FWidgetTransform());
		Widget->SetRenderTransformPivot(FVector2D::ZeroVector);
		Widget->SetRenderScale(FVector2D::UnitVector);
	};

	ResetWidgetTransform(this);
	ResetWidgetTransform(GetRootWidget());
	ResetWidgetTransform(MainMenuPanel.Get());
	ResetWidgetTransform(SaveSlotPanel.Get());
	ResetWidgetTransform(SettingsPanel.Get());
	ResetWidgetTransform(CreditsPanel.Get());

	InvalidateLayoutAndVolatility();
}

void UTunaSweeperIntroMenuWidget::ApplyTitleMenuButtonContentLayout()
{
	if (bTitleMenuButtonContentLayoutApplied || !WidgetTree)
	{
		return;
	}

	auto ApplyContent = [this](
		UButton* Button,
		const FText& Icon,
		UTextBlock* ExistingLabelText,
		const FText& Label,
		bool bPrimary)
	{
		if (!Button)
		{
			return;
		}

		UTextBlock* LabelText = ExistingLabelText;
		if (!LabelText)
		{
			LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		}
		if (!LabelText)
		{
			return;
		}

		Button->SetContent(BuildTitleMenuButtonContent(
			Icon,
			LabelText,
			Label,
			bPrimary ? 28 : 20,
			bPrimary ? 28 : 20));
	};

	ApplyContent(
		StartButton,
		FText::FromString(TEXT("\u25B6")),
		StartButtonText,
		ResolveUiText(FName(TEXT("ui.title.continue")), FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30"))),
		true);
	ApplyContent(
		SlotSelectButton,
		FText::FromString(TEXT("\u25A6")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.slot_select")), FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD"))),
		false);
	ApplyContent(
		SettingsButton,
		FText::FromString(TEXT("\u2699")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.settings")), FText::FromString(TEXT("\uC124\uC815"))),
		false);
	ApplyContent(
		CreditsButton,
		FText::FromString(TEXT("\u24D8")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.credits")), FText::FromString(TEXT("\uD06C\uB808\uB527"))),
		false);
	ApplyContent(
		QuitButton,
		FText::FromString(TEXT("\u00D7")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.quit")), FText::FromString(TEXT("\uC885\uB8CC"))),
		false);

	bTitleMenuButtonContentLayoutApplied = true;
}

void UTunaSweeperIntroMenuWidget::EnsurePiggyBankToggleButton()
{
	if (PiggyBankToggleButton || !WidgetTree || !EnemyCombatDebugToggleButton)
	{
		return;
	}

	UVerticalBox* DevelopmentSettingsStack = nullptr;
	UCanvasPanel* DevelopmentSettingsCanvas = nullptr;
	for (UPanelWidget* Parent = EnemyCombatDebugToggleButton->GetParent(); Parent; Parent = Parent->GetParent())
	{
		DevelopmentSettingsStack = Cast<UVerticalBox>(Parent);
		if (DevelopmentSettingsStack)
		{
			break;
		}

		DevelopmentSettingsCanvas = Cast<UCanvasPanel>(Parent);
		if (DevelopmentSettingsCanvas)
		{
			break;
		}
	}

	if (!DevelopmentSettingsStack && !DevelopmentSettingsCanvas)
	{
		return;
	}

	UButton* NewPiggyBankToggleButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("PiggyBankToggleButton"));
	UTextBlock* PiggyBankToggleButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("PiggyBankToggleButtonText"));
	if (!NewPiggyBankToggleButton || !PiggyBankToggleButtonText)
	{
		return;
	}

	PiggyBankToggleButtonText->SetText(FText::FromString(TEXT("\uB3FC\uC9C0\uC800\uAE08\uD1B5")));
	PiggyBankToggleButtonText->SetJustification(ETextJustify::Center);
	PiggyBankToggleButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.96f, 0.96f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(PiggyBankToggleButtonText, 17, ETunaSweeperUIFontWeight::Bold);
	NewPiggyBankToggleButton->SetContent(PiggyBankToggleButtonText);

	bool bAdded = false;
	if (DevelopmentSettingsStack)
	{
		if (UVerticalBoxSlot* AddedSlot = DevelopmentSettingsStack->AddChildToVerticalBox(NewPiggyBankToggleButton))
		{
			AddedSlot->SetHorizontalAlignment(HAlign_Fill);
			AddedSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			bAdded = true;
		}
	}
	else if (DevelopmentSettingsCanvas)
	{
		const UCanvasPanelSlot* ExistingSlot = Cast<UCanvasPanelSlot>(EnemyCombatDebugToggleButton->Slot);
		if (UCanvasPanelSlot* NewSlot = DevelopmentSettingsCanvas->AddChildToCanvas(NewPiggyBankToggleButton))
		{
			if (ExistingSlot)
			{
				NewSlot->SetAnchors(ExistingSlot->GetAnchors());
				NewSlot->SetAlignment(ExistingSlot->GetAlignment());
				NewSlot->SetSize(ExistingSlot->GetSize());
				NewSlot->SetPosition(ExistingSlot->GetPosition() + FVector2D(0.0f, ExistingSlot->GetSize().Y + 8.0f));
			}
			else
			{
				NewSlot->SetSize(FVector2D(660.0f, 46.0f));
			}
			bAdded = true;
		}
	}

	if (bAdded)
	{
		PiggyBankToggleButton = NewPiggyBankToggleButton;
	}
}

void UTunaSweeperIntroMenuWidget::EnsureAlwaysSlowPresentationToggleButton()
{
	UButton* AnchorButton = PiggyBankToggleButton ? PiggyBankToggleButton.Get() : EnemyCombatDebugToggleButton.Get();
	if (AlwaysSlowPresentationToggleButton || !WidgetTree || !AnchorButton)
	{
		return;
	}

	UVerticalBox* DevelopmentSettingsStack = nullptr;
	UCanvasPanel* DevelopmentSettingsCanvas = nullptr;
	for (UPanelWidget* Parent = AnchorButton->GetParent(); Parent; Parent = Parent->GetParent())
	{
		DevelopmentSettingsStack = Cast<UVerticalBox>(Parent);
		if (DevelopmentSettingsStack)
		{
			break;
		}

		DevelopmentSettingsCanvas = Cast<UCanvasPanel>(Parent);
		if (DevelopmentSettingsCanvas)
		{
			break;
		}
	}

	if (!DevelopmentSettingsStack && !DevelopmentSettingsCanvas)
	{
		return;
	}

	UButton* NewToggleButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("AlwaysSlowPresentationToggleButton"));
	UTextBlock* NewToggleButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("AlwaysSlowPresentationToggleButtonText"));
	if (!NewToggleButton || !NewToggleButtonText)
	{
		return;
	}

	NewToggleButtonText->SetText(FText::FromString(TEXT("\uC0C1\uC2DC \uC2AC\uB85C\uC6B0 \uC5F0\uCD9C")));
	NewToggleButtonText->SetJustification(ETextJustify::Center);
	NewToggleButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.96f, 0.96f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(NewToggleButtonText, 17, ETunaSweeperUIFontWeight::Bold);
	NewToggleButton->SetContent(NewToggleButtonText);

	bool bAdded = false;
	if (DevelopmentSettingsStack)
	{
		if (UVerticalBoxSlot* AddedSlot = DevelopmentSettingsStack->AddChildToVerticalBox(NewToggleButton))
		{
			AddedSlot->SetHorizontalAlignment(HAlign_Fill);
			AddedSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			bAdded = true;
		}
	}
	else if (DevelopmentSettingsCanvas)
	{
		const UCanvasPanelSlot* ExistingSlot = Cast<UCanvasPanelSlot>(AnchorButton->Slot);
		if (UCanvasPanelSlot* NewSlot = DevelopmentSettingsCanvas->AddChildToCanvas(NewToggleButton))
		{
			if (ExistingSlot)
			{
				NewSlot->SetAnchors(ExistingSlot->GetAnchors());
				NewSlot->SetAlignment(ExistingSlot->GetAlignment());
				NewSlot->SetSize(ExistingSlot->GetSize());
				NewSlot->SetPosition(ExistingSlot->GetPosition() + FVector2D(0.0f, ExistingSlot->GetSize().Y + 8.0f));
			}
			else
			{
				NewSlot->SetSize(FVector2D(660.0f, 46.0f));
			}
			bAdded = true;
		}
	}

	if (!bAdded)
	{
		return;
	}

	AlwaysSlowPresentationToggleButton = NewToggleButton;
	for (UPanelWidget* Parent = NewToggleButton->GetParent(); Parent; Parent = Parent->GetParent())
	{
		if (USizeBox* SectionBox = Cast<USizeBox>(Parent);
			SectionBox && SectionBox->GetFName() == FName(TEXT("EnemyCombatDebugSection")))
		{
			SectionBox->SetHeightOverride(212.0f);
			break;
		}
	}
}

void UTunaSweeperIntroMenuWidget::EnsureDevelopmentToggleButtonContent(
	UButton* ToggleButton,
	FName LabelWidgetName,
	FName IndicatorWidgetName)
{
	if (!WidgetTree || !ToggleButton || WidgetTree->FindWidget(IndicatorWidgetName))
	{
		return;
	}

	UTextBlock* LabelText = Cast<UTextBlock>(WidgetTree->FindWidget(LabelWidgetName));
	constexpr float CheckBoxLaneWidth = 76.0f;
	constexpr float CheckBoxLeftPadding = 26.0f;

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* IndicatorLane = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UHorizontalBox* IndicatorLaneContent = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* IndicatorBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UCheckBox* Indicator = WidgetTree->ConstructWidget<UCheckBox>(
		UCheckBox::StaticClass(),
		IndicatorWidgetName);
	if (!LabelText || !Content || !IndicatorLane || !IndicatorLaneContent || !IndicatorBox || !Indicator)
	{
		return;
	}

	LabelText->RemoveFromParent();
	LabelText->SetJustification(ETextJustify::Left);
	IndicatorLane->SetWidthOverride(CheckBoxLaneWidth);
	IndicatorBox->SetWidthOverride(26.0f);
	IndicatorBox->SetHeightOverride(26.0f);
	Indicator->SetIsChecked(false);
	Indicator->SetVisibility(ESlateVisibility::HitTestInvisible);
	IndicatorBox->SetContent(Indicator);
	if (UHorizontalBoxSlot* IndicatorSlot = IndicatorLaneContent->AddChildToHorizontalBox(IndicatorBox))
	{
		IndicatorSlot->SetHorizontalAlignment(HAlign_Left);
		IndicatorSlot->SetVerticalAlignment(VAlign_Center);
		IndicatorSlot->SetPadding(FMargin(CheckBoxLeftPadding, 0.0f, 0.0f, 0.0f));
		IndicatorSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	IndicatorLane->SetContent(IndicatorLaneContent);

	if (UHorizontalBoxSlot* IndicatorLaneSlot = Content->AddChildToHorizontalBox(IndicatorLane))
	{
		IndicatorLaneSlot->SetHorizontalAlignment(HAlign_Fill);
		IndicatorLaneSlot->SetVerticalAlignment(VAlign_Center);
		IndicatorLaneSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Left);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	ToggleButton->SetContent(Content);
}

UWidget* UTunaSweeperIntroMenuWidget::BuildTitleMenuButtonContent(
	const FText& Icon,
	UTextBlock* LabelText,
	const FText& Label,
	int32 LabelFontSize,
	int32 IconFontSize)
{
	if (!WidgetTree || !LabelText)
	{
		return LabelText;
	}

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	USizeBox* BalanceBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	if (!Content || !IconBox || !IconText || !BalanceBox)
	{
		return LabelText;
	}

	const FLinearColor MenuTextColor(0.94f, 0.92f, 0.84f, 1.0f);
	IconText->SetText(Icon);
	TunaSweeperUIFont::ApplyFont(IconText, IconFontSize);
	IconText->SetColorAndOpacity(FSlateColor(MenuTextColor));
	IconText->SetJustification(ETextJustify::Center);

	LabelText->RemoveFromParent();
	LabelText->SetText(Label);
	TunaSweeperUIFont::ApplyFont(LabelText, LabelFontSize);
	LabelText->SetColorAndOpacity(FSlateColor(MenuTextColor));
	LabelText->SetJustification(ETextJustify::Center);

	const float IconLaneWidth = IconFontSize >= 28 ? 58.0f : 46.0f;
	IconBox->SetWidthOverride(IconLaneWidth);
	IconBox->SetHeightOverride(IconFontSize + 8.0f);
	IconBox->SetContent(IconText);
	BalanceBox->SetWidthOverride(IconLaneWidth);
	BalanceBox->SetHeightOverride(IconFontSize + 8.0f);

	if (UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBoxSlot* BalanceSlot = Content->AddChildToHorizontalBox(BalanceBox))
	{
		BalanceSlot->SetHorizontalAlignment(HAlign_Center);
		BalanceSlot->SetVerticalAlignment(VAlign_Center);
		BalanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	return Content;
}

void UTunaSweeperIntroMenuWidget::EnsureDemoBuildImage()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!DemoBuildImage)
	{
		DemoBuildImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("DemoBuildImage")));
	}

	if (!TunaSweeperBuildFlavor::IsDemo())
	{
		if (DemoBuildImage)
		{
			DemoBuildImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (!DemoBuildImage)
	{
		UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!RootCanvas)
		{
			return;
		}

		DemoBuildImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DemoBuildImage"));
		if (!DemoBuildImage)
		{
			return;
		}

		const FSlateColorBrush BadgeBrush(FLinearColor(0.95f, 0.55f, 0.08f, 0.92f));
		DemoBuildImage->SetBrush(BadgeBrush);
		if (UCanvasPanelSlot* DemoSlot = RootCanvas->AddChildToCanvas(DemoBuildImage))
		{
			DemoSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			DemoSlot->SetPosition(FVector2D(302.0f, 282.0f));
			DemoSlot->SetSize(FVector2D(156.0f, 10.0f));
			DemoSlot->SetZOrder(4);
		}
	}

	DemoBuildImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperIntroMenuWidget::EnsureTitleWindParticleOverlay()
{
	EnsureDemoBuildImage();
	if (TitleWindParticleOverlay || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	auto SetCanvasZOrder = [](UWidget* Widget, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetZOrder(ZOrder);
		}
	};

	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("LeftScrim")), 2);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("LogoImage")), 3);
	SetCanvasZOrder(MainMenuPanel, 4);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("VersionText")), 4);
	SetCanvasZOrder(SaveSlotPanel, 10);
	SetCanvasZOrder(SettingsPanel, 10);
	SetCanvasZOrder(CreditsPanel, 10);

	TitleWindParticleOverlay = WidgetTree->ConstructWidget<UTunaSweeperTitleWindParticleWidget>(
		UTunaSweeperTitleWindParticleWidget::StaticClass(),
		TEXT("TitleWindParticleOverlay"));
	if (!TitleWindParticleOverlay)
	{
		return;
	}

	TitleWindParticleOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* ParticleSlot = RootCanvas->AddChildToCanvas(TitleWindParticleOverlay);
	if (!ParticleSlot)
	{
		TitleWindParticleOverlay = nullptr;
		return;
	}

	ParticleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ParticleSlot->SetOffsets(FMargin(0.0f));
	ParticleSlot->SetAlignment(FVector2D::ZeroVector);
	ParticleSlot->SetZOrder(1);
}

