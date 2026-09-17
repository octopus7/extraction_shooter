#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/TunaSweeperDebugDisplaySettings.h"

void UTunaSweeperIntroMenuWidget::RefreshMainMenu()
{
	const bool bIsDemo = TunaSweeperBuildFlavor::IsDemo();
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(TunaGameInstance->GetActiveSaveSlotIndex());
	}

	if (CurrentSaveSlotBox)
	{
		CurrentSaveSlotBox->SetVisibility(bIsDemo
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}
	if (CurrentSaveSlotText)
	{
		CurrentSaveSlotText->SetVisibility(bIsDemo
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
		CurrentSaveSlotText->SetText(BuildCurrentSaveSlotText(Summary.SaveSlotIndex));
	}
	if (SlotSelectButton)
	{
		SlotSelectButton->SetVisibility(bIsDemo
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}

	if (SlotSelectButtonBox)
	{
		SlotSelectButtonBox->SetVisibility(bIsDemo
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}

	if (StartButtonText)
	{
		if (!Summary.bHasData)
		{
			StartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.new_game")),
				FText::FromString(TEXT("\uC0C8\uAC8C\uC784 \uC2DC\uC791"))));
		}
		else if (!Summary.bDifficultySelected && !bIsDemo)
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

	EnsureLanguageOptionRows();
	if (InterfaceLanguageOptionRow)
	{
		InterfaceLanguageOptionRow->Configure(ResolveUiText(
			FName(TEXT("ui.settings.language")),
			FText::GetEmpty()));
		InterfaceLanguageOptionRow->SetValue(BuildLanguageNameText(PendingInterfaceLanguage));
		InterfaceLanguageOptionRow->SetStepEnabled(
			PendingInterfaceLanguage != ETunaSweeperItemTextLanguage::English,
			PendingInterfaceLanguage != ETunaSweeperItemTextLanguage::Japanese);
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
	int32 ActiveSaveSlotIndex = 1;
	bool bHasCurrentSaveData = false;
	if (const UTunaSweeperGameInstance* TunaGameInstance =
		Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		ActiveSaveSlotIndex = TunaGameInstance->GetActiveSaveSlotIndex();
		bHasCurrentSaveData =
			TunaGameInstance->GetSaveSlotSummary(ActiveSaveSlotIndex).bHasData;
	}

	if (SettingsStatusText)
	{
		SettingsStatusText->SetText(FText::GetEmpty());
		SettingsStatusText->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetNamedText(
		FName(TEXT("EnemyCombatDebugToggleButtonText")),
		ResolveUiText(
			FName(TEXT("ui.settings.development.enemy_combat_debug")),
			FText::GetEmpty()));
	EnsureDevelopmentToggleButtonContent(
		EnemyCombatDebugToggleButton,
		FName(TEXT("EnemyCombatDebugToggleButtonText")),
		NAME_None);
	TunaSweeperUIStyle::SetCheckButton(
		WidgetTree,
		EnemyCombatDebugToggleButton,
		Cast<UTextBlock>(FindIntroWidget(TEXT("EnemyCombatDebugToggleButtonText"))),
		bEnemyCombatDebugEnabled);

	if (EnemyCombatDebugToggleButton)
	{
		EnemyCombatDebugToggleButton->SetIsEnabled(true);
	}
	EnsureLanguageOptionRows();
	if (DebugDisplayLanguageOptionRow)
	{
		DebugDisplayLanguageOptionRow->Configure(ResolveUiText(
			FName(TEXT("ui.settings.development.debug_display_language")),
			FText::GetEmpty()));
		DebugDisplayLanguageOptionRow->SetValue(ResolveUiText(
			DebugDisplayLanguage == ETunaSweeperDebugDisplayLanguage::Korean
				? FName(TEXT("ui.language.korean"))
				: FName(TEXT("ui.language.english")),
			FText::GetEmpty()));
		DebugDisplayLanguageOptionRow->SetStepEnabled(
			DebugDisplayLanguage == ETunaSweeperDebugDisplayLanguage::Korean,
			DebugDisplayLanguage == ETunaSweeperDebugDisplayLanguage::English);
	}
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
		ResolveUiText(
			FName(TEXT("ui.settings.development.piggy_bank")),
			FText::GetEmpty()));
	TunaSweeperUIStyle::SetCheckButton(
		WidgetTree,
		PiggyBankToggleButton,
		Cast<UTextBlock>(FindIntroWidget(TEXT("PiggyBankToggleButtonText"))),
		bPiggyBankEnabled);

	if (PiggyBankToggleButton)
	{
		PiggyBankToggleButton->SetIsEnabled(true);
	}
	SetNamedText(
		FName(TEXT("AlwaysSlowPresentationToggleButtonText")),
		ResolveUiText(
			FName(TEXT("ui.settings.development.always_slow_presentation")),
			FText::GetEmpty()));
	TunaSweeperUIStyle::SetCheckButton(
		WidgetTree,
		AlwaysSlowPresentationToggleButton,
		Cast<UTextBlock>(FindIntroWidget(TEXT("AlwaysSlowPresentationToggleButtonText"))),
		bAlwaysSlowPresentationEnabled);

	if (AlwaysSlowPresentationToggleButton)
	{
		AlwaysSlowPresentationToggleButton->SetIsEnabled(true);
	}

	SetNamedText(
		FName(TEXT("SaveDataManagementTitleText")),
		ResolveUiText(
			FName(TEXT("ui.settings.development.save_data_title")),
			FText::GetEmpty()));
	if (SaveDataManagementStatusText)
	{
		SaveDataManagementStatusText->SetText(FText::Format(
			ResolveUiText(
				bHasCurrentSaveData
					? FName(TEXT("ui.settings.development.save_data_exists"))
					: FName(TEXT("ui.settings.development.no_save_data")),
				FText::GetEmpty()),
			FText::AsNumber(ActiveSaveSlotIndex)));
		SaveDataManagementStatusText->SetColorAndOpacity(FSlateColor(
			bHasCurrentSaveData
				? FLinearColor(0.88f, 0.82f, 0.78f, 1.0f)
				: FLinearColor(0.52f, 0.56f, 0.56f, 1.0f)));
	}
	SetNamedText(
		FName(TEXT("DeleteCurrentSaveDataButtonText")),
		ResolveUiText(
			FName(TEXT("ui.settings.development.delete_current_save")),
			FText::GetEmpty()));
	if (DeleteCurrentSaveDataButton)
	{
		DeleteCurrentSaveDataButton->SetIsEnabled(bHasCurrentSaveData);
	}
	if (DeleteCurrentSaveDataButtonText)
	{
		DeleteCurrentSaveDataButtonText->SetColorAndOpacity(FSlateColor(
			bHasCurrentSaveData
				? FLinearColor::White
				: FLinearColor(0.46f, 0.48f, 0.48f, 1.0f)));
	}

	RefreshDevelopmentSelectionStyles();
}

void UTunaSweeperIntroMenuWidget::RefreshSettingsSelectionStyles(
	const FIntPoint& CurrentResolution,
	EWindowMode::Type CurrentWindowMode)
{
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(214.0f, 50.0f), true);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(214.0f, 50.0f), false);
	ApplySettingsTabButtonStyle(SettingsDevelopmentTabButton, FVector2D(214.0f, 50.0f), false);

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
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(214.0f, 50.0f), false);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(214.0f, 50.0f), true);
	ApplySettingsTabButtonStyle(SettingsDevelopmentTabButton, FVector2D(214.0f, 50.0f), false);

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
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(214.0f, 50.0f), false);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(214.0f, 50.0f), false);
	ApplySettingsTabButtonStyle(SettingsDevelopmentTabButton, FVector2D(214.0f, 50.0f), true);
	(void)bEnemyCombatDebugEnabled;
	(void)bPiggyBankEnabled;
	(void)bAlwaysSlowPresentationEnabled;
	(void)DebugDisplayLanguage;

	if (DeleteCurrentSaveDataButton)
	{
		const bool bHasCurrentSaveData = [this]()
		{
			if (const UTunaSweeperGameInstance* TunaGameInstance =
				Cast<UTunaSweeperGameInstance>(GetGameInstance()))
			{
				return TunaGameInstance->GetSaveSlotSummary(
					TunaGameInstance->GetActiveSaveSlotIndex()).bHasData;
			}
			return false;
		}();

		TunaSweeperUIStyle::ApplyButton(
			DeleteCurrentSaveDataButton,
			TunaSweeperUIStyle::EButtonRole::Danger);
		DeleteCurrentSaveDataButton->SetIsEnabled(bHasCurrentSaveData);
	}
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

	(void)ButtonSize;
	TunaSweeperUIStyle::ApplyButton(
		Button,
		bSelected || bPrimary
			? TunaSweeperUIStyle::EButtonRole::Primary
			: TunaSweeperUIStyle::EButtonRole::Secondary,
		bSelected);
}

void UTunaSweeperIntroMenuWidget::ApplySettingsTabButtonStyle(
	UButton* Button, const FVector2D& ButtonSize, bool bSelected)
{
	if (!Button) return;
	(void)ButtonSize;
	TunaSweeperUIStyle::ApplyButton(Button, TunaSweeperUIStyle::EButtonRole::Tab, bSelected);
	Button->SetIsEnabled(true);
	if (!SettingsTabFadeTexture)
	{
		constexpr int32 FadeWidth = 256;
		TArray<uint8> Pixels;
		Pixels.Init(255, FadeWidth * 4);
		for (int32 X = 0; X < FadeWidth; ++X)
			Pixels[X * 4 + 3] = static_cast<uint8>(255.0f * (1.0f - FMath::SmoothStep(0.0f, 1.0f, float(X) / (FadeWidth - 1))));
		SettingsTabFadeTexture = UTexture2D::CreateTransient(FadeWidth, 1, PF_B8G8R8A8, NAME_None, Pixels);
		if (SettingsTabFadeTexture)
		{
			SettingsTabFadeTexture->LODGroup = TEXTUREGROUP_UI;
			SettingsTabFadeTexture->SRGB = true;
			SettingsTabFadeTexture->NeverStream = true;
			SettingsTabFadeTexture->Filter = TF_Bilinear;
			SettingsTabFadeTexture->AddressX = TA_Clamp;
			SettingsTabFadeTexture->AddressY = TA_Clamp;
			SettingsTabFadeTexture->UpdateResource();
		}
	}
	auto Background = [this](float Opacity)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(256.0f, 1.0f);
		Brush.SetResourceObject(SettingsTabFadeTexture);
		Brush.TintColor = FLinearColor(0.26f, 0.48f, 0.50f, Opacity);
		return Brush;
	};
	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(Background(bSelected ? 1.0f : 0.0f));
	Style.SetHovered(Background(bSelected ? 1.0f : 0.5f));
	Style.SetPressed(Background(bSelected ? 1.0f : 0.5f));
	Style.SetDisabled(Background(0.0f));
	Button->SetStyle(Style);
}

void UTunaSweeperIntroMenuWidget::RefreshLocalizedTexts()
{
	SetNamedText(
		FName(TEXT("SteamDemoWishlistButtonText")),
		ResolveUiText(FName(TEXT("ui.title.wishlist")), FText::GetEmpty()));
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
		ResolveUiText(FName(TEXT("ui.common.back")), FText::GetEmpty()));
	SetNamedText(
		FName(TEXT("SettingsGraphicsTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.graphics")), FText::FromString(TEXT("\uADF8\uB798\uD53D"))));
	SetNamedText(
		FName(TEXT("SettingsInterfaceTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.interface")), FText::FromString(TEXT("\uC778\uD130\uD398\uC774\uC2A4"))));
	SetNamedText(
		FName(TEXT("SettingsDevelopmentTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.development")), FText::GetEmpty()));
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

	if (DifficultyTitleText)
	{
		DifficultyTitleText->SetText(ResolveUiText(
			bDifficultyAdjustmentMode
				? FName(TEXT("ui.difficulty.adjust_title"))
				: FName(TEXT("ui.difficulty.select_title")),
			FText::GetEmpty()));
	}
	if (DifficultyStartButtonText)
	{
		DifficultyStartButtonText->SetText(ResolveUiText(
			bDifficultyAdjustmentMode
				? FName(TEXT("ui.common.apply"))
				: FName(TEXT("ui.difficulty.start")),
			FText::GetEmpty()));
	}
	if (DifficultyBackButtonText)
	{
		DifficultyBackButtonText->SetText(bDifficultyAdjustmentMode
			? ResolveUiText(FName(TEXT("ui.common.cancel")), FText::GetEmpty())
			: ResolveUiText(FName(TEXT("ui.common.back")), FText::GetEmpty()));
	}

	EnsureLanguageOptionRows();
	if (InterfaceLanguageOptionRow)
	{
		InterfaceLanguageOptionRow->Configure(ResolveUiText(
			FName(TEXT("ui.settings.language")),
			FText::GetEmpty()));
	}
	if (DebugDisplayLanguageOptionRow)
	{
		DebugDisplayLanguageOptionRow->Configure(ResolveUiText(
			FName(TEXT("ui.settings.development.debug_display_language")),
			FText::GetEmpty()));
	}
	ApplyUnifiedControlStyles();
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
