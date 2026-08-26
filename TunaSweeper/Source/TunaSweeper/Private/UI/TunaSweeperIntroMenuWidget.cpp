#include "UI/TunaSweeperIntroMenuWidget.h"
#include "TunaSweeperIntroMenuWidgetShared.h"

void UTunaSweeperIntroMenuWidget::PrepareForInitialViewport()
{
	ResetTitleViewportLayoutState();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyDemoNoticeVisualStyle();
	ApplyTitleMenuButtonContentLayout();
	EnsureDifficultySelectionPanel();
	EnsureDeleteSaveSlotHoldProgressWidget();
	HideLegacyDeleteHoldGaugeWidgets();
	EnsureTitleWindParticleOverlay();
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTunaSweeperIntroMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResetTitleViewportLayoutState();
	ApplyDemoNoticeVisualStyle();
	HideLegacyDeleteHoldGaugeWidgets();
}

void UTunaSweeperIntroMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResetTitleViewportLayoutState();
	SetIsFocusable(true);
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyDemoNoticeVisualStyle();
	EnsureTitleWindParticleOverlay();
	EnsureDeleteSaveSlotHoldProgressWidget();
	EnsureSaveSlotSelectionRingWidgets();
	EnsureDifficultySelectionPanel();
	HideLegacyDeleteHoldGaugeWidgets();
	EnsurePiggyBankToggleButton();
	EnsureAlwaysSlowPresentationToggleButton();
	EnsureDevelopmentToggleButtonContent(
		EnemyCombatDebugToggleButton,
		FName(TEXT("EnemyCombatDebugToggleButtonText")),
		FName(TEXT("EnemyCombatDebugToggleIndicator")));
	EnsureDevelopmentToggleButtonContent(
		PiggyBankToggleButton,
		FName(TEXT("PiggyBankToggleButtonText")),
		FName(TEXT("PiggyBankToggleIndicator")));
	EnsureDevelopmentToggleButtonContent(
		AlwaysSlowPresentationToggleButton,
		FName(TEXT("AlwaysSlowPresentationToggleButtonText")),
		FName(TEXT("AlwaysSlowPresentationToggleIndicator")));
	EnsureSaveDataManagementSection();

	if (StartButton)
	{
		StartButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleStartClicked);
		StartButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleStartClicked);
	}

	if (SlotSelectButton)
	{
		SlotSelectButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked);
		SlotSelectButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked);
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsClicked);
		SettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsClicked);
	}

	if (CreditsButton)
	{
		CreditsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCreditsClicked);
		CreditsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCreditsClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleQuitClicked);
		QuitButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleQuitClicked);
	}

	RefreshDistributionPresentation();

	if (DifficultyFarmingButton)
	{
		DifficultyFarmingButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyFarmingClicked);
		DifficultyFarmingButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyFarmingClicked);
	}

	if (DifficultyNormalButton)
	{
		DifficultyNormalButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyNormalClicked);
		DifficultyNormalButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyNormalClicked);
	}

	if (DifficultyHardButton)
	{
		DifficultyHardButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyHardClicked);
		DifficultyHardButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyHardClicked);
	}

	if (DifficultyStartButton)
	{
		DifficultyStartButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked);
		DifficultyStartButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked);
	}

	if (DifficultyBackButton)
	{
		DifficultyBackButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked);
		DifficultyBackButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked);
	}

	if (DemoNoticeConfirmButton)
	{
		DemoNoticeConfirmButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked);
		DemoNoticeConfirmButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked);
	}

	if (DemoNoticeBackButton)
	{
		DemoNoticeBackButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked);
		DemoNoticeBackButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked);
	}

	if (SaveSlot1Button)
	{
		SaveSlot1Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
	}

	if (SaveSlot2Button)
	{
		SaveSlot2Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
	}

	if (SaveSlot3Button)
	{
		SaveSlot3Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
	}

	if (PrimarySaveSlotButton)
	{
		PrimarySaveSlotButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked);
		PrimarySaveSlotButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked);
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked);
		DeleteSaveSlotButton->OnPressed.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed);
		DeleteSaveSlotButton->OnPressed.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed);
		DeleteSaveSlotButton->OnReleased.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased);
		DeleteSaveSlotButton->OnReleased.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased);
	}

	if (BackToMainMenuButton)
	{
		BackToMainMenuButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked);
		BackToMainMenuButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked);
	}

	if (ConfirmDeleteButton)
	{
		ConfirmDeleteButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked);
		ConfirmDeleteButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked);
	}

	if (CancelDeleteButton)
	{
		CancelDeleteButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked);
		CancelDeleteButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked);
	}

	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked);
		SettingsGraphicsTabButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked);
	}

	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked);
		SettingsInterfaceTabButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked);
	}

	if (SettingsDevelopmentTabButton)
	{
		SettingsDevelopmentTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsDevelopmentTabClicked);
		SettingsDevelopmentTabButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsDevelopmentTabClicked);
	}

	if (EnemyCombatDebugToggleButton)
	{
		EnemyCombatDebugToggleButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleEnemyCombatDebugToggleClicked);
		EnemyCombatDebugToggleButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleEnemyCombatDebugToggleClicked);
	}
	if (DebugDisplayLanguageKoreanButton)
	{
		DebugDisplayLanguageKoreanButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDebugDisplayLanguageKoreanClicked);
		DebugDisplayLanguageKoreanButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDebugDisplayLanguageKoreanClicked);
	}
	if (DebugDisplayLanguageEnglishButton)
	{
		DebugDisplayLanguageEnglishButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDebugDisplayLanguageEnglishClicked);
		DebugDisplayLanguageEnglishButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDebugDisplayLanguageEnglishClicked);
	}

	if (PiggyBankToggleButton)
	{
		PiggyBankToggleButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePiggyBankToggleClicked);
		PiggyBankToggleButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePiggyBankToggleClicked);
	}

	if (AlwaysSlowPresentationToggleButton)
	{
		AlwaysSlowPresentationToggleButton->OnClicked.RemoveDynamic(
			this,
			&UTunaSweeperIntroMenuWidget::HandleAlwaysSlowPresentationToggleClicked);
		AlwaysSlowPresentationToggleButton->OnClicked.AddDynamic(
			this,
			&UTunaSweeperIntroMenuWidget::HandleAlwaysSlowPresentationToggleClicked);
	}

	if (DeleteCurrentSaveDataButton)
	{
		DeleteCurrentSaveDataButton->OnClicked.RemoveDynamic(
			this,
			&UTunaSweeperIntroMenuWidget::HandleDeleteCurrentSaveDataClicked);
		DeleteCurrentSaveDataButton->OnClicked.AddDynamic(
			this,
			&UTunaSweeperIntroMenuWidget::HandleDeleteCurrentSaveDataClicked);
	}

	if (WindowedModeButton)
	{
		WindowedModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
		WindowedModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
	}

	if (BorderlessWindowModeButton)
	{
		BorderlessWindowModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked);
		BorderlessWindowModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked);
	}

	if (FullscreenModeButton)
	{
		FullscreenModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked);
		FullscreenModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked);
	}

	if (Resolution1280Button)
	{
		Resolution1280Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked);
		Resolution1280Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked);
	}

	if (Resolution1600Button)
	{
		Resolution1600Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked);
		Resolution1600Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked);
	}

	if (Resolution1920Button)
	{
		Resolution1920Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked);
		Resolution1920Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked);
	}

	if (Resolution2560Button)
	{
		Resolution2560Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked);
		Resolution2560Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked);
	}

	if (Resolution3840Button)
	{
		Resolution3840Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked);
		Resolution3840Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked);
	}

	if (DLSSOffButton)
	{
		DLSSOffButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked);
		DLSSOffButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked);
	}

	if (DLSSQualityButton)
	{
		DLSSQualityButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked);
		DLSSQualityButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked);
	}

	if (DLSSBalancedButton)
	{
		DLSSBalancedButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked);
		DLSSBalancedButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked);
	}

	if (DLSSPerformanceButton)
	{
		DLSSPerformanceButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked);
		DLSSPerformanceButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked);
	}

	if (BackFromSettingsButton)
	{
		BackFromSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
		BackFromSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
	}

	if (LanguageEnglishButton)
	{
		LanguageEnglishButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked);
		LanguageEnglishButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked);
	}

	if (LanguageKoreanButton)
	{
		LanguageKoreanButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked);
		LanguageKoreanButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked);
	}

	if (LanguageJapaneseButton)
	{
		LanguageJapaneseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked);
		LanguageJapaneseButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked);
	}

	if (ConfirmInterfaceSettingsButton)
	{
		ConfirmInterfaceSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked);
		ConfirmInterfaceSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked);
	}

	if (CancelInterfaceSettingsButton)
	{
		CancelInterfaceSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked);
		CancelInterfaceSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked);
	}

	if (BackFromCreditsButton)
	{
		BackFromCreditsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
		BackFromCreditsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperIntroMenuWidget::HandleLanguageChanged);
	}

	ApplyTitleMenuButtonContentLayout();
	RefreshLocalizedTexts();
	LoadTitleGraphicsSettings();
	ApplyDLSSModeToRuntime(PreferredDLSSMode);

	if (CreditsText)
	{
		CreditsText->SetText(FText::FromString(BuildCreditsColumnText(0)));
	}
	if (CreditsText2)
	{
		CreditsText2->SetText(FText::FromString(BuildCreditsColumnText(1)));
	}
	if (CreditsText3)
	{
		CreditsText3->SetText(FText::FromString(BuildCreditsColumnText(2)));
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	ResetDeleteHoldProgress();
	HideDeleteConfirmDialog();
	HideOverlayPanels();
	ShowMainMenu();
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTunaSweeperIntroMenuWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	if (bDifficultyAdjustmentMode && !bClosingDifficultyAdjustment)
	{
		bDifficultyAdjustmentMode = false;
		OnDifficultyAdjustmentClosed.Broadcast();
	}

	Super::NativeDestruct();
}

FReply UTunaSweeperIntroMenuWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (!InKeyEvent.IsRepeat() && bDifficultyAdjustmentMode && InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseDifficultyAdjustment();
		return FReply::Handled();
	}

	if (!InKeyEvent.IsRepeat() && !bDifficultyAdjustmentMode && InKeyEvent.GetKey() == EKeys::R)
	{
		ReloadIntroLevel();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UTunaSweeperIntroMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsCreditsPanelVisible() && CreditsScrollBox)
	{
		CreditsScrollOffset += InDeltaTime * CreditsScrollSpeed;
		CreditsScrollBox->SetScrollOffset(CreditsScrollOffset);
		if (CreditsScrollBox2)
		{
			CreditsScrollBox2->SetScrollOffset(CreditsScrollOffset);
		}
		if (CreditsScrollBox3)
		{
			CreditsScrollBox3->SetScrollOffset(CreditsScrollOffset);
		}
		if (CreditsScrollOffset > 3600.0f)
		{
			CreditsScrollOffset = 0.0f;
			CreditsScrollBox->SetScrollOffset(0.0f);
			if (CreditsScrollBox2)
			{
				CreditsScrollBox2->SetScrollOffset(0.0f);
			}
			if (CreditsScrollBox3)
			{
				CreditsScrollBox3->SetScrollOffset(0.0f);
			}
		}
	}

	if (!IsSaveSlotSelectionVisible())
	{
		if (bDeleteHoldActive || DeleteHoldElapsedSeconds > 0.0f)
		{
			ResetDeleteHoldProgress();
		}
		return;
	}

	if (SaveSlot1Button && SaveSlot1Button->HasKeyboardFocus())
	{
		SelectSaveSlot(1);
	}
	else if (SaveSlot2Button && SaveSlot2Button->HasKeyboardFocus())
	{
		SelectSaveSlot(2);
	}
	else if (SaveSlot3Button && SaveSlot3Button->HasKeyboardFocus())
	{
		SelectSaveSlot(3);
	}

	UpdateSaveSlotSelectionRingAnimation(InDeltaTime);

	if (!bDeleteHoldActive)
	{
		return;
	}

	if (bDeleteConfirmVisible || !CanDeleteSelectedSaveSlot())
	{
		ResetDeleteHoldProgress();
		return;
	}

	DeleteHoldElapsedSeconds += InDeltaTime;
	const float HoldProgress = FMath::Clamp(DeleteHoldElapsedSeconds / DeleteHoldDurationSeconds, 0.0f, 1.0f);
	SetDeleteHoldProgress(HoldProgress);

	if (HoldProgress >= 1.0f)
	{
		ExecuteSelectedSaveSlotDelete();
	}
}
