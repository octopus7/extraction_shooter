#include "TunaSweeperIntroMenuWidgetShared.h"

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
		return FText::Format(
			FText::FromString(TEXT("{0} - {1}")),
			FText::Format(
				ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
				FText::AsNumber(SaveSlotIndex)),
			ResolveUiText(FName(TEXT("ui.title.empty_slot")), FText::FromString(TEXT("\uBE48 \uC2AC\uB86F"))));
	}

	return FText::Format(
		ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
		FText::AsNumber(SaveSlotIndex));
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
		const TArray<FString> Lines = {
			FText::Format(
				ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
				FText::AsNumber(SaveSlotIndex)).ToString(),
			ResolveUiText(FName(TEXT("ui.title.empty_slot")), FText::FromString(TEXT("\uBE48 \uC2AC\uB86F"))).ToString(),
			ResolveUiText(FName(TEXT("ui.title.start_new_game")), FText::FromString(TEXT("\uC0C8 \uAC8C\uC784 \uC2DC\uC791"))).ToString()
		};
		return FText::FromString(FString::Join(Lines, LINE_TERMINATOR));
	}

	const TArray<FString> Lines = {
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
			FText::AsNumber(SaveSlotIndex)).ToString(),
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.play_time_pattern")), FText::FromString(TEXT("\uD50C\uB808\uC774\uC2DC\uAC04 : {0}"))),
			FText::FromString(FormatPlayTime(Summary.TotalPlaySeconds))).ToString(),
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.difficulty_pattern")), FText::FromString(TEXT("\uB09C\uC774\uB3C4 : {0}"))),
			BuildSaveSlotDifficultyText(Summary.DifficultyStage, Summary.bDifficultySelected)).ToString()
	};
	return FText::FromString(FString::Join(Lines, LINE_TERMINATOR));
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

FString UTunaSweeperIntroMenuWidget::BuildCreditsColumnText(int32 ColumnIndex) const
{
	TArray<FString> Lines;
	BuildCreditsRollText().ParseIntoArrayLines(Lines, false);

	if (Lines.IsEmpty())
	{
		return FString();
	}

	const int32 ClampedColumnIndex = FMath::Clamp(ColumnIndex, 0, 2);
	const int32 LinesPerColumn = FMath::Max(1, FMath::DivideAndRoundUp(Lines.Num(), 3));
	const int32 StartIndex = ClampedColumnIndex * LinesPerColumn;
	const int32 EndIndex = FMath::Min(StartIndex + LinesPerColumn, Lines.Num());

	FString ColumnText;
	for (int32 LineIndex = StartIndex; LineIndex < EndIndex; ++LineIndex)
	{
		if (!ColumnText.IsEmpty())
		{
			ColumnText += LINE_TERMINATOR;
		}
		ColumnText += Lines[LineIndex];
	}

	return ColumnText;
}

FString UTunaSweeperIntroMenuWidget::FormatSaveTime(int64 LastSavedAtTicks) const
{
	if (LastSavedAtTicks <= 0)
	{
		return FString(TEXT("--"));
	}

	return FDateTime(LastSavedAtTicks).ToString(TEXT("%Y-%m-%d %H:%M"));
}

FString UTunaSweeperIntroMenuWidget::FormatPlayTime(float TotalPlaySeconds) const
{
	const int32 TotalMinutes = FMath::FloorToInt(FMath::Max(0.0f, TotalPlaySeconds) / 60.0f);
	const int32 Hours = TotalMinutes / 60;
	const int32 Minutes = TotalMinutes % 60;
	return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}

FText UTunaSweeperIntroMenuWidget::BuildSaveSlotDifficultyText(int32 DifficultyStage, bool bDifficultySelected) const
{
	if (!bDifficultySelected)
	{
		return FText::FromString(TEXT("\uBBF8\uC120\uD0DD"));
	}

	switch (FMath::Clamp(DifficultyStage, 1, 3))
	{
	case 1:
		return ResolveUiText(FName(TEXT("ui.title.difficulty.farming")), FText::FromString(TEXT("\uD30C\uBC0D")));
	case 2:
		return FText::FromString(TEXT("\uC77C\uBC18"));
	case 3:
		return FText::FromString(TEXT("\uC5B4\uB824\uC6C0"));
	default:
		return ResolveUiText(FName(TEXT("ui.title.difficulty.farming")), FText::FromString(TEXT("\uD30C\uBC0D")));
	}
}

bool UTunaSweeperIntroMenuWidget::IsSaveSlotSelectionVisible() const
{
	return SaveSlotPanel && SaveSlotPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::IsDifficultySelectionVisible() const
{
	return DifficultySelectPanel && DifficultySelectPanel->GetVisibility() == ESlateVisibility::Visible;
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

