#include "TunaSweeperIntroMenuWidgetShared.h"

void UTunaSweeperIntroMenuWidget::ShowMainMenu()
{
	bDifficultyAdjustmentMode = false;
	HideOverlayPanels();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
	SetTitleLogoVisible(true);
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	SetAlwaysNewStartButtonVisible(true);
	RefreshMainMenu();
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowDifficultySelection()
{
	EnsureDifficultySelectionPanel();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	HideOverlayPanels();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(false);
	SetAlwaysNewStartButtonVisible(false);

	SelectedDifficultyStage = INDEX_NONE;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		if (TunaGameInstance->IsActiveSaveSlotDifficultySelected())
		{
			SelectedDifficultyStage = FMath::Clamp(TunaGameInstance->GetActiveSaveSlotDifficultyStage(), 1, 3);
		}
	}

	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Visible);
	}

	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::OpenForDifficultyAdjustment()
{
	bDifficultyAdjustmentMode = true;
	bClosingDifficultyAdjustment = false;
	ShowDifficultySelection();

	if (SelectedDifficultyStage == INDEX_NONE)
	{
		SelectedDifficultyStage = 1;
		if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			SelectedDifficultyStage = FMath::Clamp(TunaGameInstance->GetActiveSaveSlotDifficultyStage(), 1, 3);
		}
		RefreshDifficultySelectionPanel();
	}

	if (DifficultyStartButton)
	{
		DifficultyStartButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::CloseDifficultyAdjustment()
{
	if (!bDifficultyAdjustmentMode || bClosingDifficultyAdjustment)
	{
		return;
	}

	bClosingDifficultyAdjustment = true;
	bDifficultyAdjustmentMode = false;
	RemoveFromParent();
	OnDifficultyAdjustmentClosed.Broadcast();
	bClosingDifficultyAdjustment = false;
}

void UTunaSweeperIntroMenuWidget::ShowSaveSlotSelection()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	HideOverlayPanels();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	SaveSlotSelectionRingAngle = 0.0f;
	SetAlwaysNewStartButtonVisible(false);
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowSettingsPanel()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(false);
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SetAlwaysNewStartButtonVisible(false);
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		PendingInterfaceLanguage = TunaGameInstance->GetCurrentTextLanguage();
	}
	ShowGraphicsSettingsTab();
}

void UTunaSweeperIntroMenuWidget::ShowGraphicsSettingsTab()
{
	bShowingInterfaceSettingsTab = false;
	bShowingDevelopmentSettingsTab = false;

	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DevelopmentSettingsPanel)
	{
		DevelopmentSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(false);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(true);
	}
	if (SettingsDevelopmentTabButton)
	{
		SettingsDevelopmentTabButton->SetIsEnabled(true);
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowInterfaceSettingsTab()
{
	bShowingInterfaceSettingsTab = true;
	bShowingDevelopmentSettingsTab = false;

	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		PendingInterfaceLanguage = TunaGameInstance->GetCurrentTextLanguage();
	}
	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (DevelopmentSettingsPanel)
	{
		DevelopmentSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(true);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(false);
	}
	if (SettingsDevelopmentTabButton)
	{
		SettingsDevelopmentTabButton->SetIsEnabled(true);
	}

	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowDevelopmentSettingsTab()
{
	bShowingInterfaceSettingsTab = false;
	bShowingDevelopmentSettingsTab = true;

	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DevelopmentSettingsPanel)
	{
		DevelopmentSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(true);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(true);
	}
	if (SettingsDevelopmentTabButton)
	{
		SettingsDevelopmentTabButton->SetIsEnabled(false);
	}

	RefreshDevelopmentSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowCreditsPanel()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);
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
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	SetAlwaysNewStartButtonVisible(false);
	if (CreditsScrollBox)
	{
		CreditsScrollOffset = 0.0f;
		CreditsScrollBox->SetScrollOffset(0.0f);
	}
	if (CreditsScrollBox2)
	{
		CreditsScrollBox2->SetScrollOffset(0.0f);
	}
	if (CreditsScrollBox3)
	{
		CreditsScrollBox3->SetScrollOffset(0.0f);
	}
}

void UTunaSweeperIntroMenuWidget::HideOverlayPanels()
{
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);

	CreditsScrollOffset = 0.0f;
}

void UTunaSweeperIntroMenuWidget::SetTitleLogoVisible(bool bVisible)
{
	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* LogoWidget = WidgetTree->FindWidget(FName(TEXT("LogoImage"))))
	{
		LogoWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperIntroMenuWidget::SelectSaveSlot(int32 SaveSlotIndex)
{
	if (SelectedSaveSlotIndex == SaveSlotIndex)
	{
		return;
	}

	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	SelectedSaveSlotIndex = FMath::Clamp(SaveSlotIndex, 1, 3);
	SaveSlotSelectionRingAngle = 0.0f;
	RefreshSaveSlotMenu();
}

