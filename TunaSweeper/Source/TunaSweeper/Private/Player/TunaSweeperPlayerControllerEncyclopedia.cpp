#include "Player/TunaSweeperPlayerController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Astra/EncyclopediaGPT6Astra.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"

void ATunaSweeperPlayerController::ToggleEncyclopedia()
{
    if (EncyclopediaPanel) { CloseEncyclopedia(); return; }
    auto* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
    if (!IsLocalController() || !ControlledCharacter || ControlledCharacter->IsDead() || ControlledCharacter->IsMountedInVehicle() ||
        IsIntroMap() || IsOpeningScenarioMap() || bDialogueSequenceActive || IsPauseMenuOpen() ||
        IsHousingModeOpen() || WaveDefensePanel ||
        (DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport()) ||
        UGameplayStatics::IsGamePaused(this) || IsMoveInputIgnored() || IsLookInputIgnored()) return;
    if (IsInventoryUiOpen()) return; // Keep active inventory/container transactions owned by their UI.
    const auto Class = LoadClass<UEncyclopediaPanelGPT6Astra>(nullptr,
        TEXT("/Game/UI/Encyclopedia/WBP_EncyclopediaPanel_GPT6Astra.WBP_EncyclopediaPanel_GPT6Astra_C"));
    if (!Class) return;
    EncyclopediaPanel = CreateWidget<UEncyclopediaPanelGPT6Astra>(this, Class);
    if (!EncyclopediaPanel) return;
    CancelPawnGameplayActions();
    EncyclopediaPanel->AddToViewport(940);
    bEncyclopediaPausedGame = GetNetMode() == NM_Standalone && SetPause(true);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    ApplyDefaultGameInputModeInternal(false);
    EncyclopediaPanel->SetKeyboardFocus();
}

void ATunaSweeperPlayerController::CloseEncyclopedia()
{
    if (!EncyclopediaPanel) return;
    EncyclopediaPanel->RemoveFromParent();
    EncyclopediaPanel = nullptr;
    if (bEncyclopediaPausedGame) SetPause(false);
    bEncyclopediaPausedGame = false;
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    ApplyDefaultGameInputModeInternal(false);
}

void ATunaSweeperPlayerController::UpdateEncyclopedia()
{
    if (!EncyclopediaPanel) return;
    const auto* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
    if (!EncyclopediaPanel->IsInViewport() || !ControlledCharacter || ControlledCharacter->IsDead()) CloseEncyclopedia();
}
