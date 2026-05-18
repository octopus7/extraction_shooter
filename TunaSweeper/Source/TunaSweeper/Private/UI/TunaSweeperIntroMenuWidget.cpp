#include "UI/TunaSweeperIntroMenuWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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

	SelectedSaveSlotIndex = INDEX_NONE;
	ResetDeleteHoldProgress();
	HideDeleteConfirmDialog();
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

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
	UE_LOG(LogTemp, Log, TEXT("Title settings button clicked."));
}

void UTunaSweeperIntroMenuWidget::HandleCreditsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Title credits button clicked."));
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

void UTunaSweeperIntroMenuWidget::ShowMainMenu()
{
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
