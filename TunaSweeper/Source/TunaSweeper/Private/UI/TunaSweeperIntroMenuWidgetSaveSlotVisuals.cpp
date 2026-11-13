#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Materials/MaterialInstanceDynamic.h"

// Authored WBP layout; presentation work by gpt-6-astra, medium reasoning.
void UTunaSweeperIntroMenuWidget::PresentSaveSlot(int32 Index, const FTunaSweeperSaveSlotSummary& Summary, bool bSelected)
{
    auto Text = [this, Index](const TCHAR* Suffix, const FText& Value)
    {
        if (auto* Label = Cast<UTextBlock>(FindIntroWidget(*FString::Printf(TEXT("SaveSlot%d%s"), Index, Suffix))))
            Label->SetText(Value);
    };
    Text(TEXT("Text"), FText::Format(ResolveUiText(TEXT("ui.title.slot_label"), FText::GetEmpty()), FText::AsNumber(Index)));
    FNumberFormattingOptions Number; Number.MinimumIntegralDigits = 2;
    Text(TEXT("Number"), FText::AsNumber(Index, &Number));
    Text(TEXT("Detail"), Summary.bHasData
        ? FText::Format(ResolveUiText(TEXT("ui.title.play_time_pattern"), FText::GetEmpty()), FText::FromString(FormatPlayTime(Summary.TotalPlaySeconds)))
        : ResolveUiText(TEXT("ui.title.empty_slot"), FText::GetEmpty()));
    Text(TEXT("Difficulty"), Summary.bHasData
        ? FText::Format(ResolveUiText(TEXT("ui.title.difficulty_pattern"), FText::GetEmpty()), BuildSaveSlotDifficultyText(Summary.DifficultyStage, Summary.bDifficultySelected))
        : ResolveUiText(TEXT("ui.title.start_new_game"), FText::GetEmpty()));
    if (auto* Glow = Cast<UImage>(FindIntroWidget(*FString::Printf(TEXT("SaveSlot%dGlow"), Index))))
        if (auto* Material = Glow->GetDynamicMaterial())
        {
            Material->SetScalarParameterValue(TEXT("Selected"), bSelected ? 1.f : 0.f);
            Material->SetScalarParameterValue(TEXT("Empty"), Summary.bHasData ? 0.f : 1.f);
        }
    ApplySaveSlotButtonStyle(Cast<UButton>(FindIntroWidget(*FString::Printf(TEXT("SaveSlot%dButton"), Index))));
}
