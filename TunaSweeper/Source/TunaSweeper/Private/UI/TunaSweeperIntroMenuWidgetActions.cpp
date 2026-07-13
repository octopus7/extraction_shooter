#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperDebugDisplaySettings.h"

void UTunaSweeperIntroMenuWidget::HandleStartClicked()
{
	BeginStartTravel(false);
}

void UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked()
{
	ShowSaveSlotSelection();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsClicked()
{
	ShowSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleCreditsClicked()
{
	ShowCreditsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UTunaSweeperIntroMenuWidget::HandleAlwaysNewStartClicked()
{
	BeginStartTravel(true);
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyFarmingClicked()
{
	SelectDifficultyStage(1);
	if (DifficultyFarmingButton)
	{
		DifficultyFarmingButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyNormalClicked()
{
	SelectDifficultyStage(2);
	if (DifficultyNormalButton)
	{
		DifficultyNormalButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyHardClicked()
{
	SelectDifficultyStage(3);
	if (DifficultyHardButton)
	{
		DifficultyHardButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked()
{
	if (bStartTravelPending || SelectedDifficultyStage == INDEX_NONE)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance ||
		!TunaGameInstance->SetActiveSaveSlotDifficultyStage(SelectedDifficultyStage, true))
	{
		return;
	}

	if (bDifficultyAdjustmentMode)
	{
		CloseDifficultyAdjustment();
		return;
	}

	BeginTravelToLevel(TunaGameInstance->ResolveInitialGameplayLevelName());
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked()
{
	if (bDifficultyAdjustmentMode)
	{
		CloseDifficultyAdjustment();
		return;
	}

	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused()
{
	SelectSaveSlot(1);
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused()
{
	SelectSaveSlot(2);
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused()
{
	SelectSaveSlot(3);
}

void UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked()
{
	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return;
	}

	TunaGameInstance->SetActiveSaveSlotIndex(SelectedSaveSlotIndex);
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked()
{
	HandleDeleteSaveSlotPressed();
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed()
{
	if (!CanDeleteSelectedSaveSlot())
	{
		ResetDeleteHoldProgress();
		return;
	}

	bDeleteHoldActive = true;
	DeleteHoldElapsedSeconds = 0.0f;
	SetDeleteHoldProgress(0.0f);
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased()
{
	ResetDeleteHoldProgress();
}

void UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked()
{
	ExecuteSelectedSaveSlotDelete();
}

void UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked()
{
	ShowGraphicsSettingsTab();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked()
{
	ShowInterfaceSettingsTab();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsDevelopmentTabClicked()
{
	ShowDevelopmentSettingsTab();
}

void UTunaSweeperIntroMenuWidget::HandleEnemyCombatDebugToggleClicked()
{
	const bool bEnabled = !ATunaSweeperPlayerController::GetEnemyCombatDebugPreference();
	ATunaSweeperPlayerController::SetEnemyCombatDebugPreference(bEnabled);

	if (ATunaSweeperPlayerController* PlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
	{
		PlayerController->SetEnemyCombatDebugEnabled(bEnabled);
	}

	RefreshDevelopmentSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleDebugDisplayLanguageKoreanClicked()
{
	TunaSweeperDebugDisplaySettings::SetDebugDisplayLanguage(ETunaSweeperDebugDisplayLanguage::Korean);
	RefreshDevelopmentSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleDebugDisplayLanguageEnglishClicked()
{
	TunaSweeperDebugDisplaySettings::SetDebugDisplayLanguage(ETunaSweeperDebugDisplayLanguage::English);
	RefreshDevelopmentSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandlePiggyBankToggleClicked()
{
	const bool bEnabled = !ATunaSweeperPlayerController::GetDeveloperPiggyBankPreference();
	ATunaSweeperPlayerController::SetDeveloperPiggyBankPreference(bEnabled);
	RefreshDevelopmentSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked()
{
	ApplyDisplaySettings(EWindowMode::Windowed);
}

void UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked()
{
	ApplyDisplaySettings(EWindowMode::WindowedFullscreen);
}

void UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked()
{
	ApplyDisplaySettings(EWindowMode::Fullscreen);
}

void UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked()
{
	ApplyResolutionSetting(FIntPoint(1280, 720));
}

void UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked()
{
	ApplyResolutionSetting(FIntPoint(1600, 900));
}

void UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked()
{
	ApplyResolutionSetting(FIntPoint(1920, 1080));
}

void UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked()
{
	ApplyResolutionSetting(FIntPoint(2560, 1440));
}

void UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked()
{
	ApplyResolutionSetting(FIntPoint(3840, 2160));
}

void UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Off);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Quality);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Balanced);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Performance);
}

void UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::English;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::Korean;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::Japanese;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->SetCurrentTextLanguage(PendingInterfaceLanguage, true);
	}

	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageChanged()
{
	bTitleMenuButtonContentLayoutApplied = false;
	ApplyTitleMenuButtonContentLayout();
	RefreshLocalizedTexts();
	RefreshMainMenu();
	if (IsSaveSlotSelectionVisible())
	{
		RefreshSaveSlotMenu();
	}
	if (SettingsPanel && SettingsPanel->GetVisibility() == ESlateVisibility::Visible)
	{
		RefreshSettingsPanel();
	}
	if (IsDifficultySelectionVisible())
	{
		RefreshDifficultySelectionPanel();
	}
}

