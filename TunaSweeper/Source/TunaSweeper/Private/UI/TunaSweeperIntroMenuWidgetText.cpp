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
    return ResolveUiText(bDifficultySelected
        ? FName(*FString::Printf(TEXT("difficulty.stage.%d.title"), FMath::Clamp(DifficultyStage, 1, 3)))
        : FName(TEXT("ui.title.difficulty.unselected")), FText::GetEmpty());
}

bool UTunaSweeperIntroMenuWidget::IsSaveSlotSelectionVisible() const
{
	return SaveSlotPanel && SaveSlotPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::IsDifficultySelectionVisible() const
{
	return (DifficultySelectPanel && DifficultySelectPanel->GetVisibility() == ESlateVisibility::Visible) ||
		(DemoNoticePanel && DemoNoticePanel->GetVisibility() == ESlateVisibility::Visible);
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

