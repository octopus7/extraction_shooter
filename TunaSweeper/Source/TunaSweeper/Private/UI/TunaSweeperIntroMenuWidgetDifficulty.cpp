#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Settings/TunaSweeperBuildFlavor.h"

void UTunaSweeperIntroMenuWidget::EnsureDifficultySelectionPanel()
{
	if (DifficultySelectPanel || !WidgetTree)
	{
		return;
	}

	LoadDifficultyDefinitions();

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	UOverlay* Panel = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(),
		TEXT("DifficultySelectPanel"));
	if (!Panel)
	{
		return;
	}
	DifficultySelectPanel = Panel;
	Panel->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PanelSlot->SetOffsets(FMargin(0.0f));
		PanelSlot->SetAlignment(FVector2D::ZeroVector);
		PanelSlot->SetZOrder(12);
	}

	DifficultyBackgroundImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("DifficultyBackgroundImage"));
	if (DifficultyBackgroundImage)
	{
		FSlateBrush BackgroundBrush;
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
		BackgroundBrush.TintColor = FSlateColor(FLinearColor::White);
		BackgroundBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));
		if (UTexture2D* BackgroundTexture = LoadDifficultyTexture(
			DifficultyBackgroundTexture,
			TunaSweeperDifficultySelect::BackgroundTexturePath))
		{
			BackgroundBrush.SetResourceObject(BackgroundTexture);
		}
		DifficultyBackgroundImage->SetBrush(BackgroundBrush);
		DifficultyBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* BackgroundSlot = Panel->AddChildToOverlay(DifficultyBackgroundImage))
		{
			BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
			BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	UImage* ReadabilityWash = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("DifficultyReadabilityWash"));
	if (ReadabilityWash)
	{
		FSlateBrush WashBrush;
		WashBrush.DrawAs = ESlateBrushDrawType::Box;
		WashBrush.TintColor = FSlateColor(FLinearColor(1.0f, 0.985f, 0.93f, 0.38f));
		ReadabilityWash->SetBrush(WashBrush);
		ReadabilityWash->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* WashSlot = Panel->AddChildToOverlay(ReadabilityWash))
		{
			WashSlot->SetHorizontalAlignment(HAlign_Fill);
			WashSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	USizeBox* ContentBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("DifficultyContentBox"));
	UVerticalBox* ContentStack = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("DifficultyContentStack"));
	if (!ContentBox || !ContentStack)
	{
		return;
	}
	ContentBox->SetWidthOverride(1120.0f);
	ContentBox->SetHeightOverride(690.0f);
	ContentBox->SetContent(ContentStack);

	if (UOverlaySlot* ContentSlot = Panel->AddChildToOverlay(ContentBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	}

	DifficultyTitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DifficultyTitleText"));
	if (DifficultyTitleText)
	{
		DifficultyTitleText->SetText(FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
		DifficultyTitleText->SetJustification(ETextJustify::Center);
		DifficultyTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.10f, 0.18f, 0.20f, 1.0f)));
		TunaSweeperUIFont::ApplyFont(DifficultyTitleText, 42.0f);
		if (UVerticalBoxSlot* TitleSlot = ContentStack->AddChildToVerticalBox(DifficultyTitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Center);
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	DifficultyDemoNoticeText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DifficultyDemoNoticeText"));
	if (DifficultyDemoNoticeText)
	{
		DifficultyDemoNoticeText->SetAutoWrapText(true);
		DifficultyDemoNoticeText->SetWrapTextAt(1080.0f);
		DifficultyDemoNoticeText->SetJustification(ETextJustify::Center);
		DifficultyDemoNoticeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.16f, 0.26f, 0.28f, 1.0f)));
		DifficultyDemoNoticeText->SetVisibility(ESlateVisibility::Collapsed);
		TunaSweeperUIFont::ApplyFont(DifficultyDemoNoticeText, 28.0f);
		if (UVerticalBoxSlot* NoticeSlot = ContentStack->AddChildToVerticalBox(DifficultyDemoNoticeText))
		{
			NoticeSlot->SetHorizontalAlignment(HAlign_Fill);
			NoticeSlot->SetVerticalAlignment(VAlign_Center);
			NoticeSlot->SetPadding(FMargin(60.0f, 76.0f, 60.0f, 76.0f));
			NoticeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	auto BuildOptionButton = [this](
		const TCHAR* ButtonName,
		const TCHAR* OverlayName,
		const TCHAR* BackgroundName,
		const TCHAR* BorderName,
		const TCHAR* IconName,
		const TCHAR* TitleName,
		const TCHAR* DescriptionName,
		int32 DifficultyStage,
		TObjectPtr<UButton>& OutButton,
		TObjectPtr<UImage>& OutBackgroundImage,
		TObjectPtr<UBorder>& OutSelectionBorder,
		TObjectPtr<UImage>& OutIconImage,
		TObjectPtr<UTextBlock>& OutTitleText,
		TObjectPtr<UTextBlock>& OutDescriptionText) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		UOverlay* ButtonOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), OverlayName);
		if (!Button || !ButtonOverlay)
		{
			return nullptr;
		}

		OutButton = Button;
		ApplyDifficultyButtonStyle(Button);

		OutBackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), BackgroundName);
		if (OutBackgroundImage)
		{
			ConfigureDifficultyCardBackground(OutBackgroundImage, false);
			if (UOverlaySlot* BackgroundSlot = ButtonOverlay->AddChildToOverlay(OutBackgroundImage))
			{
				BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
				BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}

		OutSelectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), BorderName);
		if (OutSelectionBorder)
		{
			ConfigureDifficultySelectionBorder(OutSelectionBorder, false);
			if (UOverlaySlot* BorderSlot = ButtonOverlay->AddChildToOverlay(OutSelectionBorder))
			{
				BorderSlot->SetHorizontalAlignment(HAlign_Fill);
				BorderSlot->SetVerticalAlignment(VAlign_Fill);
				BorderSlot->SetPadding(FMargin(16.0f));
			}
		}

		UVerticalBox* CardStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		OutIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		UVerticalBox* TextStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		OutTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TitleName);
		OutDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), DescriptionName);
		if (CardStack && IconBox && OutIconImage && TextStack && OutTitleText && OutDescriptionText)
		{
			IconBox->SetWidthOverride(184.0f);
			IconBox->SetHeightOverride(156.0f);
			ConfigureDifficultyIcon(OutIconImage, DifficultyStage);
			IconBox->SetContent(OutIconImage);

			if (UVerticalBoxSlot* IconSlot = CardStack->AddChildToVerticalBox(IconBox))
			{
				IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin(0.0f, 50.0f, 0.0f, 16.0f));
			}

			OutTitleText->SetText(BuildDifficultyTitleText(DifficultyStage));
			OutTitleText->SetJustification(ETextJustify::Center);
			OutTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.11f, 0.20f, 0.22f, 1.0f)));
			TunaSweeperUIFont::ApplyFont(OutTitleText, 32.0f);
			if (UVerticalBoxSlot* OptionTitleSlot = TextStack->AddChildToVerticalBox(OutTitleText))
			{
				OptionTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				OptionTitleSlot->SetHorizontalAlignment(HAlign_Fill);
				OptionTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
			}

			OutDescriptionText->SetText(BuildDifficultyDescriptionText(DifficultyStage));
			OutDescriptionText->SetAutoWrapText(true);
			OutDescriptionText->SetWrapTextAt(230.0f);
			OutDescriptionText->SetJustification(ETextJustify::Center);
			OutDescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.21f, 0.30f, 0.32f, 0.95f)));
			TunaSweeperUIFont::ApplyFont(OutDescriptionText, 18.0f);
			if (UVerticalBoxSlot* DescriptionSlot = TextStack->AddChildToVerticalBox(OutDescriptionText))
			{
				DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
				DescriptionSlot->SetVerticalAlignment(VAlign_Top);
			}

			if (UVerticalBoxSlot* TextSlot = CardStack->AddChildToVerticalBox(TextStack))
			{
				TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				TextSlot->SetHorizontalAlignment(HAlign_Fill);
				TextSlot->SetVerticalAlignment(VAlign_Fill);
				TextSlot->SetPadding(FMargin(30.0f, 0.0f, 30.0f, 34.0f));
			}

			if (UOverlaySlot* CardSlot = ButtonOverlay->AddChildToOverlay(CardStack))
			{
				CardSlot->SetHorizontalAlignment(HAlign_Fill);
				CardSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}

		Button->SetContent(ButtonOverlay);
		return Button;
	};

	UHorizontalBox* OptionRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("DifficultyOptionRow"));
	if (OptionRow)
	{
		if (UVerticalBoxSlot* OptionRowSlot = ContentStack->AddChildToVerticalBox(OptionRow))
		{
			OptionRowSlot->SetHorizontalAlignment(HAlign_Center);
			OptionRowSlot->SetVerticalAlignment(VAlign_Center);
			OptionRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
			OptionRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	auto AddOptionButton = [this, OptionRow](UButton* Button, float LeftPadding)
	{
		if (!Button || !OptionRow)
		{
			return;
		}

		USizeBox* ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (!ButtonBox)
		{
			return;
		}
		ButtonBox->SetWidthOverride(320.0f);
		ButtonBox->SetHeightOverride(455.0f);
		ButtonBox->SetContent(Button);
		if (UHorizontalBoxSlot* OptionSlot = OptionRow->AddChildToHorizontalBox(ButtonBox))
		{
			OptionSlot->SetHorizontalAlignment(HAlign_Center);
			OptionSlot->SetVerticalAlignment(VAlign_Center);
			OptionSlot->SetPadding(FMargin(LeftPadding, 0.0f, 0.0f, 0.0f));
			OptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	};

	AddOptionButton(BuildOptionButton(
		TEXT("DifficultyFarmingButton"),
		TEXT("DifficultyFarmingButtonOverlay"),
		TEXT("DifficultyFarmingBackgroundImage"),
		TEXT("DifficultyFarmingSelectionBorder"),
		TEXT("DifficultyFarmingIconImage"),
		TEXT("DifficultyFarmingTitleText"),
		TEXT("DifficultyFarmingDescriptionText"),
		1,
		DifficultyFarmingButton,
		DifficultyFarmingBackgroundImage,
		DifficultyFarmingSelectionBorder,
		DifficultyFarmingIconImage,
		DifficultyFarmingTitleText,
		DifficultyFarmingDescriptionText), 0.0f);

	AddOptionButton(BuildOptionButton(
		TEXT("DifficultyNormalButton"),
		TEXT("DifficultyNormalButtonOverlay"),
		TEXT("DifficultyNormalBackgroundImage"),
		TEXT("DifficultyNormalSelectionBorder"),
		TEXT("DifficultyNormalIconImage"),
		TEXT("DifficultyNormalTitleText"),
		TEXT("DifficultyNormalDescriptionText"),
		2,
		DifficultyNormalButton,
		DifficultyNormalBackgroundImage,
		DifficultyNormalSelectionBorder,
		DifficultyNormalIconImage,
		DifficultyNormalTitleText,
		DifficultyNormalDescriptionText), 22.0f);

	AddOptionButton(BuildOptionButton(
		TEXT("DifficultyHardButton"),
		TEXT("DifficultyHardButtonOverlay"),
		TEXT("DifficultyHardBackgroundImage"),
		TEXT("DifficultyHardSelectionBorder"),
		TEXT("DifficultyHardIconImage"),
		TEXT("DifficultyHardTitleText"),
		TEXT("DifficultyHardDescriptionText"),
		3,
		DifficultyHardButton,
		DifficultyHardBackgroundImage,
		DifficultyHardSelectionBorder,
		DifficultyHardIconImage,
		DifficultyHardTitleText,
		DifficultyHardDescriptionText), 22.0f);

	USpacer* ActionSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (ActionSpacer)
	{
		ActionSpacer->SetSize(FVector2D(1.0f, 22.0f));
		ContentStack->AddChildToVerticalBox(ActionSpacer);
	}

	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("DifficultyActionRow"));
	if (ActionRow)
	{
		auto BuildActionButton = [this](
			const TCHAR* ButtonName,
			const TCHAR* TextName,
			const FText& Label,
			TObjectPtr<UTextBlock>& OutText) -> UButton*
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
			UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
			if (!Button || !Overlay || !Background || !OutText)
			{
				return nullptr;
			}

			ApplyDifficultyButtonStyle(Button);
			ConfigureDifficultyActionButtonBackground(Background, false);
			Background->SetRenderOpacity(0.92f);
			if (UOverlaySlot* BackgroundSlot = Overlay->AddChildToOverlay(Background))
			{
				BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
				BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
			}

			OutText->SetText(Label);
			OutText->SetJustification(ETextJustify::Center);
			OutText->SetColorAndOpacity(FSlateColor(FLinearColor(0.10f, 0.19f, 0.21f, 1.0f)));
			TunaSweeperUIFont::ApplyFont(OutText, 24.0f);
			if (UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(OutText))
			{
				TextSlot->SetHorizontalAlignment(HAlign_Fill);
				TextSlot->SetVerticalAlignment(VAlign_Center);
				TextSlot->SetPadding(FMargin(22.0f, 0.0f));
			}

			Button->SetContent(Overlay);
			return Button;
		};

		DifficultyBackButton = BuildActionButton(
			TEXT("DifficultyBackButton"),
			TEXT("DifficultyBackButtonText"),
			FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30")),
			DifficultyBackButtonText);
		DifficultyStartButton = BuildActionButton(
			TEXT("DifficultyStartButton"),
			TEXT("DifficultyStartButtonText"),
			FText::FromString(TEXT("\uAC8C\uC784 \uC2DC\uC791")),
			DifficultyStartButtonText);

		if (DifficultyBackButton)
		{
			USizeBox* BackBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			if (BackBox)
			{
				BackBox->SetWidthOverride(260.0f);
				BackBox->SetHeightOverride(82.0f);
				BackBox->SetContent(DifficultyBackButton);
				if (UHorizontalBoxSlot* BackSlot = ActionRow->AddChildToHorizontalBox(BackBox))
				{
					BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
					BackSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
				}
			}
		}

		if (DifficultyStartButton)
		{
			USizeBox* StartBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			if (StartBox)
			{
				StartBox->SetWidthOverride(430.0f);
				StartBox->SetHeightOverride(82.0f);
				StartBox->SetContent(DifficultyStartButton);
				if (UHorizontalBoxSlot* StartSlot = ActionRow->AddChildToHorizontalBox(StartBox))
				{
					StartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				}
			}
		}

		if (UVerticalBoxSlot* ActionSlot = ContentStack->AddChildToVerticalBox(ActionRow))
		{
			ActionSlot->SetHorizontalAlignment(HAlign_Center);
			ActionSlot->SetVerticalAlignment(VAlign_Center);
			ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::SelectDifficultyStage(int32 DifficultyStage)
{
	SelectedDifficultyStage = FMath::Clamp(DifficultyStage, 1, 3);
	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::RefreshDifficultySelectionPanel()
{
	LoadDifficultyDefinitions();
	const bool bDemoNotice = TunaSweeperBuildFlavor::IsDemo() && !bDifficultyAdjustmentMode;
	if (bDemoNotice)
	{
		SelectedDifficultyStage = 2;
	}

	if (USizeBox* ContentBox = Cast<USizeBox>(WidgetTree
		? WidgetTree->FindWidget(TEXT("DifficultyContentBox"))
		: nullptr))
	{
		ContentBox->SetWidthOverride(bDemoNotice ? 1440.0f : 1120.0f);
		ContentBox->SetHeightOverride(bDemoNotice ? 520.0f : 690.0f);
	}
	if (UWidget* OptionRow = WidgetTree
		? WidgetTree->FindWidget(TEXT("DifficultyOptionRow"))
		: nullptr)
	{
		OptionRow->SetVisibility(bDemoNotice
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}
	if (DifficultyDemoNoticeText)
	{
		DifficultyDemoNoticeText->SetVisibility(bDemoNotice
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
		DifficultyDemoNoticeText->SetText(ResolveUiText(
			FName(TEXT("ui.title.demo_notice_message")),
			FText::FromString(TEXT("데모 저장 데이터는 본편과 연동되지 않습니다."))));
	}

	if (DifficultyTitleText)
	{
		DifficultyTitleText->SetText(bDemoNotice
			? ResolveUiText(FName(TEXT("ui.title.demo_notice_title")), FText::FromString(TEXT("데모 안내")))
			: FText::FromString(
			bDifficultyAdjustmentMode
				? TEXT("\uB09C\uC774\uB3C4 \uC870\uC815")
				: TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
	}
	if (DifficultyStartButtonText)
	{
		DifficultyStartButtonText->SetText(bDemoNotice
			? ResolveUiText(FName(TEXT("ui.common.confirm")), FText::FromString(TEXT("확인")))
			: FText::FromString(
			bDifficultyAdjustmentMode
				? TEXT("\uC801\uC6A9")
				: TEXT("\uAC8C\uC784 \uC2DC\uC791")));
	}
	if (DifficultyBackButtonText)
	{
		DifficultyBackButtonText->SetText(bDifficultyAdjustmentMode
			? ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("\uCDE8\uC18C")))
			: ResolveUiText(FName(TEXT("ui.common.back")), FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));
	}

	RefreshDifficultyOption(
		1,
		DifficultyFarmingButton,
		DifficultyFarmingBackgroundImage,
		DifficultyFarmingSelectionBorder,
		DifficultyFarmingTitleText,
		DifficultyFarmingDescriptionText);
	RefreshDifficultyOption(
		2,
		DifficultyNormalButton,
		DifficultyNormalBackgroundImage,
		DifficultyNormalSelectionBorder,
		DifficultyNormalTitleText,
		DifficultyNormalDescriptionText);
	RefreshDifficultyOption(
		3,
		DifficultyHardButton,
		DifficultyHardBackgroundImage,
		DifficultyHardSelectionBorder,
		DifficultyHardTitleText,
		DifficultyHardDescriptionText);

	if (DifficultyFarmingButton)
	{
		DifficultyFarmingButton->SetVisibility(bDemoNotice ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (DifficultyHardButton)
	{
		DifficultyHardButton->SetVisibility(bDemoNotice ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (DifficultyNormalButton)
	{
		DifficultyNormalButton->SetVisibility(bDemoNotice ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible);
	}
	if (bDemoNotice)
	{
		if (DifficultyNormalTitleText)
		{
			DifficultyNormalTitleText->SetText(ResolveUiText(
				FName(TEXT("ui.title.demo_label")), FText::FromString(TEXT("데모"))));
		}
		if (DifficultyNormalDescriptionText)
		{
			DifficultyNormalDescriptionText->SetText(ResolveUiText(
				FName(TEXT("ui.title.demo_notice_message")),
				FText::FromString(TEXT("이 데모의 저장 데이터는 본편과 연동되지 않습니다."))));
		}
	}

	if (DifficultyStartButton)
	{
		DifficultyStartButton->SetIsEnabled((bDemoNotice || SelectedDifficultyStage != INDEX_NONE) && !bStartTravelPending);
	}
}

void UTunaSweeperIntroMenuWidget::RefreshDifficultyOption(
	int32 DifficultyStage,
	UButton* Button,
	UImage* BackgroundImage,
	UBorder* SelectionBorder,
	UTextBlock* TitleText,
	UTextBlock* DescriptionText)
{
	const bool bSelected = SelectedDifficultyStage == DifficultyStage;
	if (Button)
	{
		Button->SetIsEnabled(!bStartTravelPending);
	}
	ConfigureDifficultyCardBackground(BackgroundImage, bSelected);
	ConfigureDifficultySelectionBorder(SelectionBorder, bSelected);
	if (TitleText)
	{
		TitleText->SetText(BuildDifficultyTitleText(DifficultyStage));
		TitleText->SetColorAndOpacity(FSlateColor(bSelected
			? FLinearColor(0.06f, 0.16f, 0.18f, 1.0f)
			: FLinearColor(0.11f, 0.20f, 0.22f, 1.0f)));
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(BuildDifficultyDescriptionText(DifficultyStage));
		DescriptionText->SetColorAndOpacity(FSlateColor(bSelected
			? FLinearColor(0.12f, 0.24f, 0.26f, 1.0f)
			: FLinearColor(0.21f, 0.30f, 0.32f, 0.95f)));
	}
}

void UTunaSweeperIntroMenuWidget::ApplyDifficultyButtonStyle(UButton* Button) const
{
	if (!Button)
	{
		return;
	}

	FSlateBrush EmptyBrush;
	EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	EmptyBrush.TintColor = FSlateColor(FLinearColor::Transparent);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(EmptyBrush);
	ButtonStyle.SetHovered(EmptyBrush);
	ButtonStyle.SetPressed(EmptyBrush);
	ButtonStyle.SetDisabled(EmptyBrush);
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
	Button->SetStyle(ButtonStyle);
	Button->SetClickMethod(EButtonClickMethod::DownAndUp);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultyCardBackground(UImage* BackgroundImage, bool bSelected)
{
	if (!BackgroundImage)
	{
		return;
	}

	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
	BackgroundBrush.Margin = FMargin(0.18f);
	BackgroundBrush.SetImageSize(FVector2D(320.0f, 455.0f));
	BackgroundBrush.TintColor = FSlateColor(bSelected
		? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
		: FLinearColor(0.94f, 0.98f, 0.98f, 0.92f));
	if (UTexture2D* ButtonTexture = LoadDifficultyTexture(
		DifficultyCardFrameTexture,
		TunaSweeperDifficultySelect::CardFrameTexturePath))
	{
		BackgroundBrush.SetResourceObject(ButtonTexture);
	}
	BackgroundImage->SetBrush(BackgroundBrush);
	BackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	BackgroundImage->SetRenderOpacity(bSelected ? 1.0f : 0.90f);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultyActionButtonBackground(UImage* BackgroundImage, bool bSelected)
{
	if (!BackgroundImage)
	{
		return;
	}

	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
	BackgroundBrush.Margin = FMargin(0.25f, 0.36f);
	BackgroundBrush.SetImageSize(FVector2D(420.0f, 82.0f));
	BackgroundBrush.TintColor = FSlateColor(bSelected
		? FLinearColor::White
		: FLinearColor(0.96f, 0.99f, 0.99f, 0.93f));
	if (UTexture2D* ButtonTexture = LoadDifficultyTexture(
		DifficultyActionButtonTexture,
		TunaSweeperDifficultySelect::ActionButtonTexturePath))
	{
		BackgroundBrush.SetResourceObject(ButtonTexture);
	}
	BackgroundImage->SetBrush(BackgroundBrush);
	BackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	BackgroundImage->SetRenderOpacity(bSelected ? 1.0f : 0.92f);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultySelectionBorder(UBorder* SelectionBorder, bool bSelected)
{
	if (!SelectionBorder)
	{
		return;
	}

	FSlateBrush BorderBrush;
	BorderBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	BorderBrush.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	BorderBrush.OutlineSettings = FSlateBrushOutlineSettings(
		32.0f,
		FSlateColor(FLinearColor(1.0f, 0.76f, 0.22f, 1.0f)),
		bSelected ? 5.0f : 0.0f);
	BorderBrush.OutlineSettings.bUseBrushTransparency = false;
	SelectionBorder->SetBrush(BorderBrush);
	SelectionBorder->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultyIcon(UImage* IconImage, int32 DifficultyStage)
{
	if (!IconImage)
	{
		return;
	}

	UTexture2D* IconTexture = nullptr;
	switch (FMath::Clamp(DifficultyStage, 1, 3))
	{
	case 1:
		IconTexture = LoadDifficultyTexture(
			DifficultyFarmingIconTexture,
			TunaSweeperDifficultySelect::FarmingIconTexturePath);
		break;
	case 2:
		IconTexture = LoadDifficultyTexture(
			DifficultyNormalIconTexture,
			TunaSweeperDifficultySelect::NormalIconTexturePath);
		break;
	case 3:
	default:
		IconTexture = LoadDifficultyTexture(
			DifficultyHardIconTexture,
			TunaSweeperDifficultySelect::HardIconTexturePath);
		break;
	}

	FSlateBrush IconBrush;
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.SetImageSize(FVector2D(150.0f, 150.0f));
	IconBrush.TintColor = FSlateColor(FLinearColor::White);
	if (IconTexture)
	{
		IconBrush.SetResourceObject(IconTexture);
	}
	IconImage->SetBrush(IconBrush);
	IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperIntroMenuWidget::LoadDifficultyDefinitions()
{
	if (bDifficultyDefinitionsLoaded)
	{
		return;
	}

	DifficultyOptionTexts.Reset();
	for (int32 DifficultyStage = 1; DifficultyStage <= 3; ++DifficultyStage)
	{
		FDifficultyOptionText OptionText;
		OptionText.DifficultyStage = DifficultyStage;
		OptionText.Title = TunaSweeperDifficultySelect::MakeFallbackTitle(DifficultyStage);
		OptionText.Description = TunaSweeperDifficultySelect::MakeFallbackDescription(DifficultyStage);
		DifficultyOptionTexts.Add(OptionText);
	}

	FString JsonContent;
	const FString JsonPath = TunaSweeperDifficultySelect::GetDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		bDifficultyDefinitionsLoaded = true;
		return;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, RootValue) || !RootValue.IsValid())
	{
		bDifficultyDefinitionsLoaded = true;
		return;
	}

	TArray<TSharedPtr<FJsonValue>> RootArrayValues;
	const TArray<TSharedPtr<FJsonValue>>* DifficultyValues = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		RootArrayValues = RootValue->AsArray();
		DifficultyValues = &RootArrayValues;
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		if (RootObject.IsValid())
		{
			RootObject->TryGetArrayField(TEXT("difficulties"), DifficultyValues) ||
				RootObject->TryGetArrayField(TEXT("difficulty_options"), DifficultyValues);
		}
	}

	if (!DifficultyValues)
	{
		bDifficultyDefinitionsLoaded = true;
		return;
	}

	for (const TSharedPtr<FJsonValue>& DifficultyValue : *DifficultyValues)
	{
		const TSharedPtr<FJsonObject>* DifficultyObjectPtr = nullptr;
		if (!DifficultyValue.IsValid() ||
			!DifficultyValue->TryGetObject(DifficultyObjectPtr) ||
			!DifficultyObjectPtr ||
			!DifficultyObjectPtr->IsValid())
		{
			continue;
		}

		const TSharedPtr<FJsonObject>& DifficultyObject = *DifficultyObjectPtr;
		double NumericStage = 0.0;
		if (!DifficultyObject->TryGetNumberField(TEXT("difficulty_stage"), NumericStage) &&
			!DifficultyObject->TryGetNumberField(TEXT("stage"), NumericStage) &&
			!DifficultyObject->TryGetNumberField(TEXT("id"), NumericStage))
		{
			continue;
		}

		const int32 DifficultyStage = FMath::Clamp(FMath::RoundToInt(NumericStage), 1, 3);
		FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
			[DifficultyStage](const FDifficultyOptionText& Candidate)
			{
				return Candidate.DifficultyStage == DifficultyStage;
			});
		if (!OptionText)
		{
			continue;
		}

		FString StringValue;
		if (DifficultyObject->TryGetStringField(TEXT("title"), StringValue) ||
			DifficultyObject->TryGetStringField(TEXT("name"), StringValue) ||
			DifficultyObject->TryGetStringField(TEXT("label"), StringValue))
		{
			OptionText->Title = FText::FromString(StringValue);
		}
		if (DifficultyObject->TryGetStringField(TEXT("description"), StringValue) ||
			DifficultyObject->TryGetStringField(TEXT("desc"), StringValue))
		{
			OptionText->Description = FText::FromString(StringValue);
		}
	}

	bDifficultyDefinitionsLoaded = true;
}

FText UTunaSweeperIntroMenuWidget::BuildDifficultyTitleText(int32 DifficultyStage) const
{
	if (const FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
		[DifficultyStage](const FDifficultyOptionText& Candidate)
		{
			return Candidate.DifficultyStage == DifficultyStage;
		}))
	{
		return OptionText->Title;
	}

	return TunaSweeperDifficultySelect::MakeFallbackTitle(DifficultyStage);
}

FText UTunaSweeperIntroMenuWidget::BuildDifficultyDescriptionText(int32 DifficultyStage) const
{
	if (const FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
		[DifficultyStage](const FDifficultyOptionText& Candidate)
		{
			return Candidate.DifficultyStage == DifficultyStage;
		}))
	{
		return OptionText->Description;
	}

	return TunaSweeperDifficultySelect::MakeFallbackDescription(DifficultyStage);
}

UTexture2D* UTunaSweeperIntroMenuWidget::LoadDifficultyTexture(
	TObjectPtr<UTexture2D>& TextureCache,
	const TCHAR* TexturePath)
{
	if (!TextureCache && TexturePath)
	{
		TextureCache = LoadObject<UTexture2D>(nullptr, TexturePath);
	}

	return TextureCache;
}
