#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Component/TunaSweeperScratchComponent.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/TunaSweeperDebugDisplaySettings.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"

void UTunaSweeperIntroMenuWidget::HandleStartClicked()
{
	BeginStartTravel();
}

void UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked()
{
	if (TunaSweeperBuildFlavor::IsDemo())
	{
		return;
	}
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
	if (bStartTravelPending)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (TunaSweeperBuildFlavor::IsDemo() && !bDifficultyAdjustmentMode)
	{
		if (!TunaGameInstance || !TunaGameInstance->DeleteSaveSlotAndStartNewGame(1))
		{
			if (TunaGameInstance)
			{
				if (UTunaSweeperToastSubsystem* ToastSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperToastSubsystem>())
				{
					ToastSubsystem->ShowToast(ResolveUiText(
						FName(TEXT("ui.toast.save_failed")),
						FText::FromString(TEXT("저장 데이터를 만들지 못했습니다."))));
				}
			}
			return;
		}

		TunaGameInstance->BeginScenarioBunkerEntry(NAME_None);
		BeginTravelToLevel(TunaSweeperBuildFlavor::GetBunkerLevelName());
		return;
	}

	if (SelectedDifficultyStage == INDEX_NONE)
	{
		return;
	}
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

void UTunaSweeperIntroMenuWidget::HandleAlwaysSlowPresentationToggleClicked()
{
	const bool bEnabled = !ATunaSweeperPlayerController::GetDeveloperAlwaysSlowPresentationPreference();
	ATunaSweeperPlayerController::SetDeveloperAlwaysSlowPresentationPreference(bEnabled);

	if (const ATunaSweeperPlayerController* PlayerController =
		Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
	{
		if (APawn* PlayerPawn = PlayerController->GetPawn())
		{
			if (UTunaSweeperScratchComponent* ScratchComponent =
				PlayerPawn->FindComponentByClass<UTunaSweeperScratchComponent>())
			{
				ScratchComponent->SetDeveloperAlwaysSlowPresentationEnabled(bEnabled);
			}
		}
	}

	RefreshDevelopmentSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleDeleteCurrentSaveDataClicked()
{
	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return;
	}

	const int32 ActiveSaveSlotIndex = TunaGameInstance->GetActiveSaveSlotIndex();
	if (!TunaGameInstance->GetSaveSlotSummary(ActiveSaveSlotIndex).bHasData)
	{
		RefreshDevelopmentSettingsPanel();
		return;
	}

	const bool bDeleted = TunaGameInstance->DeleteSaveSlot(ActiveSaveSlotIndex);
	if (UTunaSweeperToastSubsystem* ToastSubsystem =
		TunaGameInstance->GetSubsystem<UTunaSweeperToastSubsystem>())
	{
		ToastSubsystem->ShowToast(bDeleted
			? ResolveUiText(
				FName(TEXT("ui.toast.save_slot_deleted")),
				FText::FromString(TEXT("삭제 되었습니다")))
			: ResolveUiText(
				FName(TEXT("ui.toast.save_delete_failed")),
				FText::FromString(TEXT("저장 데이터를 삭제하지 못했습니다."))));
	}

	RefreshMainMenu();
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
	if (TitleGraphicsSettingsWidget)
	{
		TitleGraphicsSettingsWidget->DiscardPendingChanges();
	}
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

