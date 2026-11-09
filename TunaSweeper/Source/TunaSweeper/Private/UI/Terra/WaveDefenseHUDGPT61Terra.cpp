#include "UI/Terra/WaveDefenseHUDGPT61Terra.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Components/TextBlock.h"
void UWaveDefenseHUDGPT61Terra::SetDirector(AWaveDefenseDirector* InDirector) { Director = InDirector; }
void UWaveDefenseHUDGPT61Terra::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Strings = Game ? Game->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
    if (!Director.IsValid() || !Strings) { SetVisibility(ESlateVisibility::Collapsed); return; }
    const auto S = Director->GetSnapshot();
    SetVisibility(S.Phase == EWaveDefensePhase::Idle || S.Phase == EWaveDefensePhase::Finished ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    FText StatusFormat, DetailFormat;
    Strings->TryGetTextByKey(TEXT("ui.wave_defense.status"), Game->GetCurrentTextLanguage(), StatusFormat);
    Strings->TryGetTextByKey(TEXT("ui.wave_defense.details"), Game->GetCurrentTextLanguage(), DetailFormat);
    StatusText->SetText(FText::Format(StatusFormat, FText::AsNumber(S.Wave), FText::AsNumber(S.TotalWaves), FText::AsNumber(S.CoreHealth), FText::AsNumber(S.CoreMaxHealth), FText::AsNumber(S.Alive), FText::AsNumber(S.Queued), FText::AsNumber(S.Supplies)));
    DetailText->SetText(FText::Format(DetailFormat, FText::AsNumber(Director->GetDifficultyStage()), FText::AsNumber(Director->GetRewardCoins()), FText::AsNumber(FMath::CeilToInt(S.SecondsRemaining)), FText::AsNumber(FMath::CeilToInt(S.LeaveSecondsRemaining))));
}
