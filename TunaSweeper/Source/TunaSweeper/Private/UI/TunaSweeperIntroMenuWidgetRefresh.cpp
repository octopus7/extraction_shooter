#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperDebugDisplaySettings.h"

void UTunaSweeperIntroMenuWidget::RefreshMainMenu()
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(TunaGameInstance->GetActiveSaveSlotIndex());
	}

	if (CurrentSaveSlotText)
	{
		CurrentSaveSlotText->SetText(BuildCurrentSaveSlotText(Summary.SaveSlotIndex));
	}

	if (StartButtonText)
	{
		if (!Summary.bHasData)
		{
			StartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.new_game")),
				FText::FromString(TEXT("\uC0C8\uAC8C\uC784 \uC2DC\uC791"))));
		}
		else if (!Summary.bDifficultySelected)
		{
			StartButtonText->SetText(FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
		}
		else
		{
			StartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.continue")),
				FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30"))));
		}
	}
}

void UTunaSweeperIntroMenuWidget::RefreshSaveSlotMenu()
{
	RefreshSaveSlotButton(1, SaveSlot1Button, SaveSlot1Text);
	RefreshSaveSlotButton(2, SaveSlot2Button, SaveSlot2Text);
	RefreshSaveSlotButton(3, SaveSlot3Button, SaveSlot3Text);

	if (SaveSlotActionRow)
	{
		SaveSlotActionRow->SetVisibility(SelectedSaveSlotIndex == INDEX_NONE
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}

	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return;
	}

	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SelectedSaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SelectedSaveSlotIndex;
	}

	if (PrimarySaveSlotButtonText)
	{
		PrimarySaveSlotButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.primary_save_slot")),
			FText::FromString(TEXT("\uC138\uC774\uBE0C \uC2AC\uB86F \uC120\uD0DD"))));
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->SetIsEnabled(Summary.bHasData);
	}
	if (DeleteSaveSlotButtonBox)
	{
		DeleteSaveSlotButtonBox->SetVisibility(Summary.bHasData
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden);
	}
	else if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->SetVisibility(Summary.bHasData
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden);
	}

	if (DeleteSaveSlotButtonText)
	{
		DeleteSaveSlotButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.delete_hold")),
			FText::FromString(TEXT("\uAE38\uAC8C \uB20C\uB7EC \uC0AD\uC81C\uD558\uAE30"))));
		DeleteSaveSlotButtonText->SetJustification(ETextJustify::Center);
		DeleteSaveSlotButtonText->SetMargin(FMargin(0.0f));
		DeleteSaveSlotButtonText->SetColorAndOpacity(FSlateColor(Summary.bHasData
			? FLinearColor::White
			: FLinearColor(0.55f, 0.60f, 0.62f, 1.0f)));
	}

	if (!Summary.bHasData)
	{
		ResetDeleteHoldProgress();
	}
}

void UTunaSweeperIntroMenuWidget::RefreshSettingsPanel()
{
	if (bShowingDevelopmentSettingsTab)
	{
		RefreshDevelopmentSettingsPanel();
		return;
	}

	if (bShowingInterfaceSettingsTab)
	{
		RefreshInterfaceSettingsPanel();
		return;
	}

	FIntPoint CurrentResolution(0, 0);
	EWindowMode::Type CurrentWindowMode = EWindowMode::Windowed;
	if (GEngine)
	{
		if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
		{
			CurrentResolution = GameUserSettings->GetScreenResolution();
			CurrentWindowMode = GameUserSettings->GetFullscreenMode();
		}
	}

	if (SettingsStatusText)
	{
		const bool bDLSSSupported = UDLSSLibrary::IsDLSSSupported();
		const FText DLSSStatusText = bDLSSSupported
			? BuildDLSSModeText(PreferredDLSSMode)
			: ResolveUiText(
				FName(TEXT("ui.settings.dlss.unavailable")),
				FText::FromString(TEXT("\uC0AC\uC6A9 \uBD88\uAC00")));
		SettingsStatusText->SetText(FText::Format(
			ResolveUiText(
				FName(TEXT("ui.settings.current_graphics")),
				FText::FromString(TEXT("\uD604\uC7AC: {0} / {1}x{2} / DLSS {3}"))),
			BuildWindowModeText(CurrentWindowMode),
			FText::AsNumber(CurrentResolution.X),
			FText::AsNumber(CurrentResolution.Y),
			DLSSStatusText));
	}

	SetNamedText(
		FName(TEXT("Resolution1280ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(1280, 720) ? TEXT("\u2713 1280 x 720") : TEXT("1280 x 720")));
	SetNamedText(
		FName(TEXT("Resolution1600ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(1600, 900) ? TEXT("\u2713 1600 x 900") : TEXT("1600 x 900")));
	SetNamedText(
		FName(TEXT("Resolution1920ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(1920, 1080) ? TEXT("\u2713 1920 x 1080") : TEXT("1920 x 1080")));
	SetNamedText(
		FName(TEXT("Resolution2560ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(2560, 1440) ? TEXT("\u2713 2560 x 1440") : TEXT("2560 x 1440")));
	SetNamedText(
		FName(TEXT("Resolution3840ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(3840, 2160) ? TEXT("\u2713 3840 x 2160") : TEXT("3840 x 2160")));

	if (DLSSOffButton)
	{
		DLSSOffButton->SetIsEnabled(true);
	}
	if (DLSSQualityButton)
	{
		DLSSQualityButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Quality));
	}
	if (DLSSBalancedButton)
	{
		DLSSBalancedButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Balanced));
	}
	if (DLSSPerformanceButton)
	{
		DLSSPerformanceButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Performance));
	}

	RefreshSettingsSelectionStyles(CurrentResolution, CurrentWindowMode);
}

void UTunaSweeperIntroMenuWidget::RefreshInterfaceSettingsPanel()
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	const ETunaSweeperItemTextLanguage CurrentLanguage = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	if (SettingsStatusText)
	{
		SettingsStatusText->SetText(FText::Format(
			ResolveUiText(
				FName(TEXT("ui.settings.current_language")),
				FText::FromString(TEXT("\uD604\uC7AC \uC5B8\uC5B4: {0}"))),
			BuildLanguageNameText(CurrentLanguage)));
	}

	if (LanguageEnglishButtonText)
	{
		LanguageEnglishButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::English,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::English));
	}
	if (LanguageKoreanButtonText)
	{
		LanguageKoreanButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::Korean,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Korean));
	}
	if (LanguageJapaneseButtonText)
	{
		LanguageJapaneseButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::Japanese,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Japanese));
	}

	if (LanguageEnglishButton)
	{
		LanguageEnglishButton->SetIsEnabled(true);
	}
	if (LanguageKoreanButton)
	{
		LanguageKoreanButton->SetIsEnabled(true);
	}
	if (LanguageJapaneseButton)
	{
		LanguageJapaneseButton->SetIsEnabled(true);
	}
	if (ConfirmInterfaceSettingsButton)
	{
		ConfirmInterfaceSettingsButton->SetIsEnabled(true);
	}
	if (CancelInterfaceSettingsButton)
	{
		CancelInterfaceSettingsButton->SetIsEnabled(true);
	}

	RefreshInterfaceSelectionStyles();
}

void UTunaSweeperIntroMenuWidget::RefreshDevelopmentSettingsPanel()
{
	const bool bEnemyCombatDebugEnabled = ATunaSweeperPlayerController::GetEnemyCombatDebugPreference();
	const bool bPiggyBankEnabled = ATunaSweeperPlayerController::GetDeveloperPiggyBankPreference();
	const bool bAlwaysSlowPresentationEnabled =
		ATunaSweeperPlayerController::GetDeveloperAlwaysSlowPresentationPreference();
	const ETunaSweeperDebugDisplayLanguage DebugDisplayLanguage =
		TunaSweeperDebugDisplaySettings::GetDebugDisplayLanguage();

	if (SettingsStatusText)
	{
		SettingsStatusText->SetText(FText::Format(
			FText::FromString(TEXT("{0}\n\uB514\uBC84\uADF8 \uD45C\uAE30 \uC5B8\uC5B4: {1}\n\uB3FC\uC9C0\uC800\uAE08\uD1B5: {2} (\uB2E4\uC74C \uBC99\uCEE4 \uC785\uC7A5\uBD80\uD130)\n\uC0C1\uC2DC \uC2AC\uB85C\uC6B0 \uC5F0\uCD9C: {3}")),
			bEnemyCombatDebugEnabled
				? FText::FromString(TEXT("\uC804\uD22C \uB514\uBC84\uADF8: \uCF1C\uC9D0 (F8)"))
				: FText::FromString(TEXT("\uC804\uD22C \uB514\uBC84\uADF8: \uAEBC\uC9D0 (F8)")),
			DebugDisplayLanguage == ETunaSweeperDebugDisplayLanguage::Korean
				? FText::FromString(TEXT("\uD55C\uAD6D\uC5B4"))
				: FText::FromString(TEXT("English")),
			bPiggyBankEnabled
				? FText::FromString(TEXT("\uCF1C\uC9D0"))
				: FText::FromString(TEXT("\uAEBC\uC9D0")),
			bAlwaysSlowPresentationEnabled
				? FText::FromString(TEXT("\uCF1C\uC9D0"))
				: FText::FromString(TEXT("\uAEBC\uC9D0"))));
	}

	auto SetToggleIndicatorState = [this](FName IndicatorWidgetName, bool bChecked)
	{
		if (WidgetTree)
		{
			if (UCheckBox* Indicator = Cast<UCheckBox>(WidgetTree->FindWidget(IndicatorWidgetName)))
			{
				Indicator->SetIsChecked(bChecked);
			}
		}
	};

	SetNamedText(
		FName(TEXT("EnemyCombatDebugToggleButtonText")),
		FText::FromString(TEXT("\uC801 \uC804\uD22C \uB514\uBC84\uADF8 \uD45C\uC2DC")));
	SetToggleIndicatorState(FName(TEXT("EnemyCombatDebugToggleIndicator")), bEnemyCombatDebugEnabled);

	if (EnemyCombatDebugToggleButton)
	{
		EnemyCombatDebugToggleButton->SetIsEnabled(true);
	}
	SetNamedText(
		FName(TEXT("DebugDisplayLanguageLabelText")),
		FText::FromString(TEXT("\uB514\uBC84\uADF8 \uD45C\uAE30 \uC5B8\uC5B4")));
	SetNamedText(
		FName(TEXT("DebugDisplayLanguageKoreanButtonText")),
		FText::FromString(TEXT("\uD55C\uAD6D\uC5B4")));
	SetNamedText(
		FName(TEXT("DebugDisplayLanguageEnglishButtonText")),
		FText::FromString(TEXT("English")));
	if (DebugDisplayLanguageKoreanButton)
	{
		DebugDisplayLanguageKoreanButton->SetIsEnabled(true);
	}
	if (DebugDisplayLanguageEnglishButton)
	{
		DebugDisplayLanguageEnglishButton->SetIsEnabled(true);
	}
	SetNamedText(
		FName(TEXT("PiggyBankToggleButtonText")),
		FText::FromString(TEXT("\uB3FC\uC9C0\uC800\uAE08\uD1B5")));
	SetToggleIndicatorState(FName(TEXT("PiggyBankToggleIndicator")), bPiggyBankEnabled);

	if (PiggyBankToggleButton)
	{
		PiggyBankToggleButton->SetIsEnabled(true);
	}
	SetNamedText(
		FName(TEXT("AlwaysSlowPresentationToggleButtonText")),
		FText::FromString(TEXT("\uC0C1\uC2DC \uC2AC\uB85C\uC6B0 \uC5F0\uCD9C")));
	SetToggleIndicatorState(
		FName(TEXT("AlwaysSlowPresentationToggleIndicator")),
		bAlwaysSlowPresentationEnabled);

	if (AlwaysSlowPresentationToggleButton)
	{
		AlwaysSlowPresentationToggleButton->SetIsEnabled(true);
	}

	RefreshDevelopmentSelectionStyles();
}

void UTunaSweeperIntroMenuWidget::RefreshSettingsSelectionStyles(
	const FIntPoint& CurrentResolution,
	EWindowMode::Type CurrentWindowMode)
{
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(142.0f, 38.0f), true);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(158.0f, 38.0f), false);
	ApplySettingsTabButtonStyle(SettingsDevelopmentTabButton, FVector2D(102.0f, 38.0f), false);

	ApplySettingsChoiceButtonStyle(
		WindowedModeButton,
		FVector2D(160.0f, 44.0f),
		CurrentWindowMode == EWindowMode::Windowed);
	ApplySettingsChoiceButtonStyle(
		BorderlessWindowModeButton,
		FVector2D(236.0f, 44.0f),
		CurrentWindowMode == EWindowMode::WindowedFullscreen);
	ApplySettingsChoiceButtonStyle(
		FullscreenModeButton,
		FVector2D(184.0f, 44.0f),
		CurrentWindowMode == EWindowMode::Fullscreen);

	ApplySettingsChoiceButtonStyle(
		Resolution1280Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(1280, 720));
	ApplySettingsChoiceButtonStyle(
		Resolution1600Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(1600, 900));
	ApplySettingsChoiceButtonStyle(
		Resolution1920Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(1920, 1080));
	ApplySettingsChoiceButtonStyle(
		Resolution2560Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(2560, 1440));
	ApplySettingsChoiceButtonStyle(
		Resolution3840Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(3840, 2160));

	ApplySettingsChoiceButtonStyle(
		DLSSOffButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Off);
	ApplySettingsChoiceButtonStyle(
		DLSSQualityButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Quality);
	ApplySettingsChoiceButtonStyle(
		DLSSBalancedButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Balanced);
	ApplySettingsChoiceButtonStyle(
		DLSSPerformanceButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Performance);
}

void UTunaSweeperIntroMenuWidget::RefreshInterfaceSelectionStyles()
{
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(142.0f, 38.0f), false);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(158.0f, 38.0f), true);
	ApplySettingsTabButtonStyle(SettingsDevelopmentTabButton, FVector2D(102.0f, 38.0f), false);

	ApplySettingsChoiceButtonStyle(
		LanguageEnglishButton,
		FVector2D(660.0f, 46.0f),
		PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::English);
	ApplySettingsChoiceButtonStyle(
		LanguageKoreanButton,
		FVector2D(660.0f, 46.0f),
		PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Korean);
	ApplySettingsChoiceButtonStyle(
		LanguageJapaneseButton,
		FVector2D(660.0f, 46.0f),
		PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Japanese);
	ApplySettingsChoiceButtonStyle(
		CancelInterfaceSettingsButton,
		FVector2D(160.0f, 46.0f),
		false);
	ApplySettingsChoiceButtonStyle(
		ConfirmInterfaceSettingsButton,
		FVector2D(160.0f, 46.0f),
		false,
		true);
}

void UTunaSweeperIntroMenuWidget::RefreshDevelopmentSelectionStyles()
{
	const bool bEnemyCombatDebugEnabled = ATunaSweeperPlayerController::GetEnemyCombatDebugPreference();
	const bool bPiggyBankEnabled = ATunaSweeperPlayerController::GetDeveloperPiggyBankPreference();
	const bool bAlwaysSlowPresentationEnabled =
		ATunaSweeperPlayerController::GetDeveloperAlwaysSlowPresentationPreference();
	const ETunaSweeperDebugDisplayLanguage DebugDisplayLanguage =
		TunaSweeperDebugDisplaySettings::GetDebugDisplayLanguage();
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(142.0f, 38.0f), false);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(158.0f, 38.0f), false);
	ApplySettingsTabButtonStyle(SettingsDevelopmentTabButton, FVector2D(102.0f, 38.0f), true);
	ApplySettingsChoiceButtonStyle(
		EnemyCombatDebugToggleButton,
		FVector2D(660.0f, 46.0f),
		bEnemyCombatDebugEnabled);
	ApplySettingsChoiceButtonStyle(
		DebugDisplayLanguageKoreanButton,
		FVector2D(660.0f, 46.0f),
		DebugDisplayLanguage == ETunaSweeperDebugDisplayLanguage::Korean);
	ApplySettingsChoiceButtonStyle(
		DebugDisplayLanguageEnglishButton,
		FVector2D(660.0f, 46.0f),
		DebugDisplayLanguage == ETunaSweeperDebugDisplayLanguage::English);
	ApplySettingsChoiceButtonStyle(
		PiggyBankToggleButton,
		FVector2D(660.0f, 46.0f),
		bPiggyBankEnabled);
	ApplySettingsChoiceButtonStyle(
		AlwaysSlowPresentationToggleButton,
		FVector2D(660.0f, 46.0f),
		bAlwaysSlowPresentationEnabled);
}

void UTunaSweeperIntroMenuWidget::ApplySettingsChoiceButtonStyle(
	UButton* Button,
	const FVector2D& ButtonSize,
	bool bSelected,
	bool bPrimary) const
{
	if (!Button)
	{
		return;
	}

	using namespace TunaSweeperSettingsUi;

	const FLinearColor NormalFill = bSelected
		? FLinearColor(0.04f, 0.25f, 0.28f, 0.92f)
		: (bPrimary ? FLinearColor(0.05f, 0.34f, 0.38f, 0.92f) : FLinearColor(0.022f, 0.034f, 0.040f, 0.80f));
	const FLinearColor HoveredFill = bSelected
		? FLinearColor(0.06f, 0.36f, 0.40f, 0.98f)
		: (bPrimary ? FLinearColor(0.07f, 0.44f, 0.48f, 0.98f) : FLinearColor(0.045f, 0.075f, 0.085f, 0.92f));
	const FLinearColor PressedFill = NormalFill * 0.78f;
	const FLinearColor Outline = bSelected || bPrimary
		? Accent
		: FLinearColor(0.56f, 0.66f, 0.66f, 0.70f);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		NormalFill,
		Outline,
		bSelected || bPrimary ? 1.8f : 1.0f,
		ButtonCornerRadius));
	ButtonStyle.SetHovered(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		HoveredFill,
		FLinearColor(0.82f, 0.98f, 1.0f, 1.0f),
		bSelected || bPrimary ? 2.2f : 1.4f,
		ButtonCornerRadius));
	ButtonStyle.SetPressed(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		PressedFill,
		Outline * 0.84f,
		1.0f,
		ButtonCornerRadius));
	ButtonStyle.SetDisabled(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		bSelected ? NormalFill : FLinearColor(0.018f, 0.024f, 0.028f, 0.58f),
		bSelected ? Outline : FLinearColor(0.30f, 0.36f, 0.36f, 0.42f),
		bSelected ? 1.6f : 0.8f,
		ButtonCornerRadius));
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));

	Button->SetStyle(ButtonStyle);
	Button->SetClickMethod(EButtonClickMethod::DownAndUp);
}

void UTunaSweeperIntroMenuWidget::ApplySettingsTabButtonStyle(
	UButton* Button,
	const FVector2D& ButtonSize,
	bool bSelected) const
{
	if (!Button)
	{
		return;
	}

	using namespace TunaSweeperSettingsUi;

	const FLinearColor Fill = bSelected
		? FLinearColor(0.035f, 0.19f, 0.21f, 0.92f)
		: FLinearColor(0.018f, 0.030f, 0.036f, 0.78f);
	const FLinearColor HoveredFill = bSelected
		? FLinearColor(0.05f, 0.28f, 0.31f, 0.98f)
		: FLinearColor(0.035f, 0.070f, 0.080f, 0.94f);
	const FLinearColor Outline = bSelected
		? Accent
		: FLinearColor(0.46f, 0.56f, 0.56f, 0.68f);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		Fill,
		Outline,
		bSelected ? 1.8f : 1.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetHovered(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		HoveredFill,
		FLinearColor(0.78f, 0.98f, 1.0f, 1.0f),
		2.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetPressed(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		Fill * 0.78f,
		Outline,
		1.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetDisabled(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		ButtonSize,
		Fill,
		Outline,
		bSelected ? 1.8f : 1.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));

	Button->SetStyle(ButtonStyle);
	Button->SetClickMethod(EButtonClickMethod::DownAndUp);
}

void UTunaSweeperIntroMenuWidget::RefreshLocalizedTexts()
{
	SetNamedText(
		FName(TEXT("SaveSlotPanelTitleText")),
		ResolveUiText(FName(TEXT("ui.title.slot_select")), FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD"))));
	SetNamedText(
		FName(TEXT("BackToMainMenuButtonText")),
		ResolveUiText(FName(TEXT("ui.common.back")), FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));
	SetNamedText(
		FName(TEXT("DeleteConfirmTitleText")),
		ResolveUiText(FName(TEXT("ui.title.delete_confirm_title")), FText::FromString(TEXT("\uC2AC\uB86F \uC0AD\uC81C"))));
	SetNamedText(
		FName(TEXT("DeleteConfirmMessageText")),
		ResolveUiText(FName(TEXT("ui.title.delete_confirm_message")), FText::FromString(TEXT("\uC120\uD0DD\uD55C \uC800\uC7A5 \uB370\uC774\uD130\uB97C \uC0AD\uC81C\uD560\uAE4C\uC694?"))));
	SetNamedText(
		FName(TEXT("ConfirmDeleteButtonText")),
		ResolveUiText(FName(TEXT("ui.common.delete")), FText::FromString(TEXT("\uC0AD\uC81C\uD558\uAE30"))));
	SetNamedText(
		FName(TEXT("CancelDeleteButtonText")),
		ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("\uCDE8\uC18C"))));
	SetNamedText(
		FName(TEXT("SettingsTitleText")),
		ResolveUiText(FName(TEXT("ui.title.settings")), FText::FromString(TEXT("\uC124\uC815"))));
	SetNamedText(
		FName(TEXT("SettingsGraphicsTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.graphics")), FText::FromString(TEXT("\uADF8\uB798\uD53D"))));
	SetNamedText(
		FName(TEXT("SettingsInterfaceTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.interface")), FText::FromString(TEXT("\uC778\uD130\uD398\uC774\uC2A4"))));
	SetNamedText(
		FName(TEXT("WindowModeLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.window_mode")), FText::FromString(TEXT("\uD654\uBA74 \uBAA8\uB4DC"))));
	SetNamedText(
		FName(TEXT("WindowedModeButtonText")),
		BuildWindowModeText(EWindowMode::Windowed));
	SetNamedText(
		FName(TEXT("BorderlessWindowModeButtonText")),
		BuildWindowModeText(EWindowMode::WindowedFullscreen));
	SetNamedText(
		FName(TEXT("FullscreenModeButtonText")),
		BuildWindowModeText(EWindowMode::Fullscreen));
	SetNamedText(
		FName(TEXT("ResolutionLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.resolution")), FText::FromString(TEXT("\uD574\uC0C1\uB3C4"))));
	SetNamedText(
		FName(TEXT("DLSSLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.dlss")), FText::FromString(TEXT("DLSS"))));
	SetNamedText(
		FName(TEXT("DLSSOffButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Off));
	SetNamedText(
		FName(TEXT("DLSSQualityButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Quality));
	SetNamedText(
		FName(TEXT("DLSSBalancedButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Balanced));
	SetNamedText(
		FName(TEXT("DLSSPerformanceButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Performance));
	SetNamedText(
		FName(TEXT("LanguageLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.language")), FText::FromString(TEXT("\uC5B8\uC5B4"))));
	SetNamedText(
		FName(TEXT("ConfirmInterfaceSettingsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.confirm")), FText::FromString(TEXT("\uACB0\uC815"))));
	SetNamedText(
		FName(TEXT("CancelInterfaceSettingsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("\uCDE8\uC18C"))));
	SetNamedText(
		FName(TEXT("CreditsTitleText")),
		ResolveUiText(FName(TEXT("ui.title.credits")), FText::FromString(TEXT("\uD06C\uB808\uB527"))));
	SetNamedText(
		FName(TEXT("BackFromCreditsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.back")), FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));

	if (AlwaysNewStartButtonText)
	{
		AlwaysNewStartButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.always_new_start")),
			FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
	}
	if (DifficultyTitleText)
	{
		DifficultyTitleText->SetText(FText::FromString(
			bDifficultyAdjustmentMode
				? TEXT("\uB09C\uC774\uB3C4 \uC870\uC815")
				: TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
	}
	if (DifficultyStartButtonText)
	{
		DifficultyStartButtonText->SetText(FText::FromString(
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
}

void UTunaSweeperIntroMenuWidget::RefreshSaveSlotButton(int32 SaveSlotIndex, UButton* SlotButton, UTextBlock* SlotText)
{
	const bool bSelected = SaveSlotIndex == SelectedSaveSlotIndex;

	if (SlotText)
	{
		SlotText->SetText(BuildSaveSlotButtonText(SaveSlotIndex));
		SlotText->SetJustification(ETextJustify::Center);
		SlotText->SetMargin(FMargin(0.0f));
		SlotText->SetColorAndOpacity(FSlateColor(bSelected
			? FLinearColor::White
			: FLinearColor(0.74f, 0.80f, 0.84f, 1.0f)));
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(true);
		ApplySaveSlotButtonStyle(SlotButton, bSelected);
	}

	UImage* RingImage = nullptr;
	switch (SaveSlotIndex)
	{
	case 1:
		RingImage = GeneratedSaveSlot1SelectionRingImage;
		break;
	case 2:
		RingImage = GeneratedSaveSlot2SelectionRingImage;
		break;
	case 3:
		RingImage = GeneratedSaveSlot3SelectionRingImage;
		break;
	default:
		break;
	}

	SetSaveSlotSelectionRingSelected(RingImage, bSelected);
}

void UTunaSweeperIntroMenuWidget::ApplySaveSlotButtonStyle(UButton* SlotButton, bool bSelected)
{
	if (!SlotButton)
	{
		return;
	}

	const FVector2D ButtonSize(700.0f, 112.0f);
	const float CornerRadius = 11.0f;
	const FLinearColor NormalFill(0.025f, 0.045f, 0.050f, 0.56f);
	const FLinearColor HoveredFill(0.055f, 0.095f, 0.105f, 0.76f);
	const FLinearColor PressedFill = NormalFill * 0.75f;
	const FLinearColor NormalOutline = bSelected
		? FLinearColor(0.98f, 1.0f, 0.92f, 1.0f)
		: FLinearColor(0.78f, 0.84f, 0.82f, 0.88f);
	const FLinearColor HoveredOutline = bSelected
		? FLinearColor(1.0f, 1.0f, 0.96f, 1.0f)
		: FLinearColor(0.96f, 0.98f, 0.95f, 1.0f);
	const FLinearColor PressedOutline = bSelected
		? FLinearColor(0.94f, 0.98f, 0.88f, 1.0f)
		: FLinearColor(0.60f, 0.68f, 0.68f, 0.90f);

	auto MakeSlotBrush = [ButtonSize, CornerRadius](const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ButtonSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(CornerRadius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	};

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(MakeSlotBrush(NormalFill, NormalOutline, bSelected ? 2.8f : 1.3f));
	ButtonStyle.SetHovered(MakeSlotBrush(HoveredFill, HoveredOutline, bSelected ? 3.2f : 1.7f));
	ButtonStyle.SetPressed(MakeSlotBrush(PressedFill, PressedOutline, bSelected ? 2.2f : 1.0f));
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
	SlotButton->SetStyle(ButtonStyle);
	SlotButton->SetClickMethod(EButtonClickMethod::DownAndUp);
}
