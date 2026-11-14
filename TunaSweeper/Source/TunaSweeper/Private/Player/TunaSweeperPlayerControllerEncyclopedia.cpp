#include "Player/TunaSweeperPlayerController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Encyclopedia/EncyclopediaTerminal.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Astra/EncyclopediaGPT6Astra.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"

bool ATunaSweeperPlayerController::OpenEncyclopedia(AEncyclopediaTerminal* Terminal)
{
    if (EncyclopediaPanel || !IsValid(Terminal) || !Terminal->IsWithinInteractionDistance(GetPawn())) return false;
    auto* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
    if (!IsLocalController() || !ControlledCharacter || ControlledCharacter->IsDead() || ControlledCharacter->IsMountedInVehicle() ||
        IsIntroMap() || IsOpeningScenarioMap() || bDialogueSequenceActive || IsPauseMenuOpen() ||
        IsHousingModeOpen() || WaveDefensePanel ||
        (DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport()) ||
        UGameplayStatics::IsGamePaused(this) || IsMoveInputIgnored() || IsLookInputIgnored()) return false;
    if (IsInventoryUiOpen()) return false; // Keep active inventory/container transactions owned by their UI.
    const auto Class = Terminal->ResolvePanelWidgetClass();
    if (!Class) return false;
    EncyclopediaPanel = CreateWidget<UEncyclopediaPanelGPT6Astra>(this, Class);
    if (!EncyclopediaPanel) return false;
    ActiveEncyclopediaTerminal = Terminal;
    CancelPawnGameplayActions();
    EncyclopediaPanel->AddToViewport(940);
    bEncyclopediaPausedGame = GetNetMode() == NM_Standalone && SetPause(true);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    ApplyDefaultGameInputModeInternal(false);
    EncyclopediaPanel->SetKeyboardFocus();
    return true;
}

void ATunaSweeperPlayerController::CloseEncyclopedia()
{
    if (!EncyclopediaPanel) return;
    EncyclopediaPanel->RemoveFromParent();
    EncyclopediaPanel = nullptr;
    ActiveEncyclopediaTerminal.Reset();
    if (bEncyclopediaPausedGame) SetPause(false);
    bEncyclopediaPausedGame = false;
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    ApplyDefaultGameInputModeInternal(false);
}

void ATunaSweeperPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // UE runs Actor Tick during pause but skips PlayerTick. Keep modal lifetime checks active.
    UpdateEncyclopedia();
}

void ATunaSweeperPlayerController::UpdateEncyclopedia()
{
    if (!EncyclopediaPanel) return;
    const auto* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
    if (!EncyclopediaPanel->IsInViewport() || !ControlledCharacter || ControlledCharacter->IsDead() ||
        !ActiveEncyclopediaTerminal.IsValid() || !ActiveEncyclopediaTerminal->IsWithinInteractionDistance(ControlledCharacter))
        CloseEncyclopedia();
}
