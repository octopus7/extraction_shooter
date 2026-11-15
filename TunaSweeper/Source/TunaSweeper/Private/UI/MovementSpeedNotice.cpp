// Authored by gpt-6-astra. Reasoning effort is not exposed in this session.
#include "UI/MovementSpeedNotice.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/TunaSweeperUIFont.h"

void UMovementSpeedNotice::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    TunaSweeperUIFont::ApplyFontToWidgetTree(this);
}

void UMovementSpeedNotice::UpdateSpeed(float Multiplier)
{
    SetVisibility(Multiplier > 1.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    const auto* Instance = GetGameInstance<UTunaSweeperGameInstance>();
    if (!SpeedText || !Instance || Multiplier <= 1.0f) return;
    const int32 Language = static_cast<int32>(Instance->GetCurrentTextLanguage());
    if (DisplayedMultiplier == Multiplier && DisplayedLanguage == Language) return;
    DisplayedMultiplier = Multiplier;
    DisplayedLanguage = Language;
    FFormatNamedArguments Args;
    Args.Add(TEXT("Multiplier"), FText::AsNumber(FMath::RoundToInt(Multiplier)));
    SpeedText->SetText(FText::Format(Instance->ResolveLocalizedText(
        TEXT("ui.movement_cheat.speed_notice"), FText::GetEmpty()), Args));
}
