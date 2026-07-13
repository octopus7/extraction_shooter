#include "TunaSweeperIntroMenuWidgetShared.h"

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

void UTunaSweeperIntroMenuWidget::EnsureTitleWindParticleOverlay()
{
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

	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("BackgroundImage")), 0);
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

