#include "UI/TunaSweeperIntroMenuWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UTunaSweeperIntroMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

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

	if (WindowedModeButton)
	{
		WindowedModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
		WindowedModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
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

	if (BackFromSettingsButton)
	{
		BackFromSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
		BackFromSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
	}

	if (BackFromCreditsButton)
	{
		BackFromCreditsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
		BackFromCreditsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
	}

	if (CreditsText)
	{
		CreditsText->SetText(FText::FromString(BuildCreditsRollText()));
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	ResetDeleteHoldProgress();
	HideDeleteConfirmDialog();
	HideOverlayPanels();
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsCreditsPanelVisible() && CreditsScrollBox)
	{
		CreditsScrollOffset += InDeltaTime * CreditsScrollSpeed;
		CreditsScrollBox->SetScrollOffset(CreditsScrollOffset);
		if (CreditsScrollOffset > 3600.0f)
		{
			CreditsScrollOffset = 0.0f;
			CreditsScrollBox->SetScrollOffset(0.0f);
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
		bDeleteHoldActive = false;
		ShowDeleteConfirmDialog();
	}
}

void UTunaSweeperIntroMenuWidget::HandleStartClicked()
{
	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (TunaGameInstance)
	{
		const int32 ActiveSaveSlotIndex = TunaGameInstance->GetActiveSaveSlotIndex();
		const FTunaSweeperSaveSlotSummary Summary = TunaGameInstance->GetSaveSlotSummary(ActiveSaveSlotIndex);
		TunaGameInstance->ActivateSaveSlot(ActiveSaveSlotIndex, !Summary.bHasData);
	}

	if (!StartTargetLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, StartTargetLevelName);
	}
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
	if (!CanDeleteSelectedSaveSlot() || bDeleteConfirmVisible)
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
	if (!bDeleteConfirmVisible)
	{
		ResetDeleteHoldProgress();
	}
	else
	{
		bDeleteHoldActive = false;
	}
}

void UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked()
{
	if (CanDeleteSelectedSaveSlot())
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			TunaGameInstance->DeleteSaveSlot(SelectedSaveSlotIndex);
		}
	}

	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	RefreshSaveSlotMenu();
	RefreshMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
}

void UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked()
{
	ApplyDisplaySettings(EWindowMode::Windowed);
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

void UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked()
{
	HideOverlayPanels();
}

void UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked()
{
	HideOverlayPanels();
}

void UTunaSweeperIntroMenuWidget::ShowMainMenu()
{
	HideOverlayPanels();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	RefreshMainMenu();
	RefreshSaveSlotMenu();
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
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
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
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowCreditsPanel()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CreditsText)
	{
		CreditsText->SetText(FText::FromString(BuildCreditsRollText()));
	}
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (CreditsScrollBox)
	{
		CreditsScrollOffset = 0.0f;
		CreditsScrollBox->SetScrollOffset(0.0f);
	}
}

void UTunaSweeperIntroMenuWidget::HideOverlayPanels()
{
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	CreditsScrollOffset = 0.0f;
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
	RefreshSaveSlotMenu();
}

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
		StartButtonText->SetText(FText::FromString(TEXT("\uC774\uC5B4\uAC00\uAE30")));
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
		PrimarySaveSlotButtonText->SetText(FText::FromString(TEXT("\uC138\uC774\uBE0C \uC2AC\uB86F \uC120\uD0DD")));
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->SetIsEnabled(Summary.bHasData);
	}

	if (DeleteSaveSlotButtonText)
	{
		DeleteSaveSlotButtonText->SetText(FText::FromString(TEXT("\uAE38\uAC8C \uB20C\uB7EC \uC0AD\uC81C\uD558\uAE30")));
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
	if (!SettingsStatusText)
	{
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

	const FString WindowModeText = CurrentWindowMode == EWindowMode::Fullscreen
		? TEXT("\uC804\uCCB4\uD654\uBA74")
		: TEXT("\uCC3D\uBAA8\uB4DC");
	SettingsStatusText->SetText(FText::FromString(FString::Printf(
		TEXT("\uD604\uC7AC: %s / %dx%d"),
		*WindowModeText,
		CurrentResolution.X,
		CurrentResolution.Y)));
}

void UTunaSweeperIntroMenuWidget::RefreshSaveSlotButton(int32 SaveSlotIndex, UButton* SlotButton, UTextBlock* SlotText)
{
	if (SlotText)
	{
		SlotText->SetText(BuildSaveSlotButtonText(SaveSlotIndex));
		SlotText->SetColorAndOpacity(FSlateColor(SaveSlotIndex == SelectedSaveSlotIndex
			? FLinearColor::White
			: FLinearColor(0.74f, 0.80f, 0.84f, 1.0f)));
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(true);
	}
}

FText UTunaSweeperIntroMenuWidget::BuildCurrentSaveSlotText(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SaveSlotIndex;
	}

	if (!Summary.bHasData)
	{
		return FText::FromString(FString::Printf(TEXT("\uC2AC\uB86F %d - \uBE48 \uC2AC\uB86F"), SaveSlotIndex));
	}

	return FText::FromString(FString::Printf(
		TEXT("\uC2AC\uB86F %d - %s"),
		SaveSlotIndex,
		*FormatPlayTime(Summary.TotalPlaySeconds)));
}

FText UTunaSweeperIntroMenuWidget::BuildSaveSlotButtonText(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SaveSlotIndex;
	}

	if (!Summary.bHasData)
	{
		return FText::FromString(FString::Printf(
			TEXT("\uC2AC\uB86F %d\n\uBE48 \uC2AC\uB86F\n\uC0C8 \uAC8C\uC784 \uC2DC\uC791"),
			SaveSlotIndex));
	}

	return FText::FromString(FString::Printf(
		TEXT("\uC2AC\uB86F %d\n\uD50C\uB808\uC774 \uC2DC\uAC04 %s\n\uC9C4\uD589 \uB370\uC774\uD130 \uC788\uC74C"),
		SaveSlotIndex,
		*FormatPlayTime(Summary.TotalPlaySeconds)));
}

FString UTunaSweeperIntroMenuWidget::BuildCreditsRollText() const
{
	const FString CreditsFilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UI"),
		TEXT("Credits"),
		TEXT("StaffRoll.txt"));

	FString CreditsTextFromFile;
	if (FFileHelper::LoadFileToString(CreditsTextFromFile, *CreditsFilePath) &&
		!CreditsTextFromFile.TrimStartAndEnd().IsEmpty())
	{
		return CreditsTextFromFile;
	}

	return FString(
		TEXT("Tuna Sweeper\n\n")
		TEXT("A Game by BlenG\n\n\n")
		TEXT("Direction\nBlenG\n\n")
		TEXT("Game Design\nBlenG\n\n")
		TEXT("Programming\nBlenG\n\n")
		TEXT("Art Direction\nBlenG\n\n")
		TEXT("UI Design\nBlenG\n\n")
		TEXT("Scenario\nBlenG\n\n")
		TEXT("Level Design\nBlenG\n\n")
		TEXT("Audio Direction\nBlenG\n\n")
		TEXT("QA\nBlenG\n\n\n")
		TEXT("Thank you for playing.\n"));
}

FString UTunaSweeperIntroMenuWidget::FormatPlayTime(float TotalSeconds) const
{
	const int32 ClampedTotalSeconds = FMath::Max(0, FMath::RoundToInt(TotalSeconds));
	const int32 Hours = ClampedTotalSeconds / 3600;
	const int32 Minutes = (ClampedTotalSeconds / 60) % 60;
	const int32 Seconds = ClampedTotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);
}

FString UTunaSweeperIntroMenuWidget::FormatSaveTime(int64 LastSavedAtTicks) const
{
	if (LastSavedAtTicks <= 0)
	{
		return FString(TEXT("--"));
	}

	return FDateTime(LastSavedAtTicks).ToString(TEXT("%Y-%m-%d %H:%M"));
}

bool UTunaSweeperIntroMenuWidget::IsSaveSlotSelectionVisible() const
{
	return SaveSlotPanel && SaveSlotPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::IsCreditsPanelVisible() const
{
	return CreditsPanel && CreditsPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::CanDeleteSelectedSaveSlot() const
{
	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		return TunaGameInstance->GetSaveSlotSummary(SelectedSaveSlotIndex).bHasData;
	}

	return false;
}

void UTunaSweeperIntroMenuWidget::ApplyDisplaySettings(EWindowMode::Type WindowMode)
{
	if (!GEngine)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
	{
		GameUserSettings->SetFullscreenMode(WindowMode);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ApplyResolutionSetting(const FIntPoint& Resolution)
{
	if (!GEngine)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
	{
		GameUserSettings->SetScreenResolution(Resolution);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ResetDeleteHoldProgress()
{
	bDeleteHoldActive = false;
	DeleteHoldElapsedSeconds = 0.0f;
	SetDeleteHoldProgress(0.0f);
}

void UTunaSweeperIntroMenuWidget::SetDeleteHoldProgress(float Progress)
{
	if (!DeleteHoldGaugeFill)
	{
		return;
	}

	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	DeleteHoldGaugeFill->SetRenderOpacity(ClampedProgress > 0.0f ? 1.0f : 0.0f);
	DeleteHoldGaugeFill->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	DeleteHoldGaugeFill->SetRenderScale(FVector2D(ClampedProgress, ClampedProgress));
}

void UTunaSweeperIntroMenuWidget::ShowDeleteConfirmDialog()
{
	bDeleteConfirmVisible = true;
	SetDeleteHoldProgress(1.0f);

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTunaSweeperIntroMenuWidget::HideDeleteConfirmDialog()
{
	bDeleteConfirmVisible = false;

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}
