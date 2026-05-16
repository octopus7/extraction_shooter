#include "UI/TunaSweeperIntroMenuWidget.h"

#include "Components/Button.h"
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
		DeleteSaveSlotButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsSaveSlotSelectionVisible())
	{
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
}

void UTunaSweeperIntroMenuWidget::HandleStartClicked()
{
	if (SaveSlotPanel)
	{
		ShowSaveSlotSelection();
		return;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->ActivateSaveSlot(1, false);
	}

	if (!StartTargetLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, StartTargetLevelName);
	}
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
	if (SelectedSaveSlotIndex == INDEX_NONE || StartTargetLevelName.IsNone())
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return;
	}

	const FTunaSweeperSaveSlotSummary Summary = TunaGameInstance->GetSaveSlotSummary(SelectedSaveSlotIndex);
	TunaGameInstance->ActivateSaveSlot(SelectedSaveSlotIndex, !Summary.bHasData);
	UGameplayStatics::OpenLevel(this, StartTargetLevelName);
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked()
{
	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->DeleteSaveSlot(SelectedSaveSlotIndex);
	}

	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowSaveSlotSelection()
{
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

	SelectedSaveSlotIndex = FMath::Clamp(SaveSlotIndex, 1, 3);
	RefreshSaveSlotMenu();
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
		PrimarySaveSlotButtonText->SetText(Summary.bHasData
			? FText::FromString(TEXT("\uC774\uC5B4\uC11C\uD558\uAE30"))
			: FText::FromString(TEXT("\uC0C8\uB85C\uC2DC\uC791\uD558\uAE30")));
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->SetIsEnabled(Summary.bHasData);
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
		return FText::FromString(FString::Printf(TEXT("\uC2AC\uB86F %d\n\uBE48 \uC2AC\uB86F"), SaveSlotIndex));
	}

	return FText::FromString(FString::Printf(
		TEXT("\uC2AC\uB86F %d\n\uD50C\uB808\uC774 %s\n\uC800\uC7A5 %s"),
		SaveSlotIndex,
		*FormatPlayTime(Summary.TotalPlaySeconds),
		*FormatSaveTime(Summary.LastSavedAtTicks)));
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
