#include "Player/TunaSweeperPlayerController.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "IconEditor/IconEditorTerminal.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Astra/IconEditorGPT6Astra.h"
#include "UI/TunaSweeperIntroMenuWidget.h"

bool ATunaSweeperPlayerController::OpenIconEditorPanel(AIconEditorTerminal* Terminal)
{
	if (IconEditorPanel)
	{
		IconEditorPanel->SetKeyboardFocus();
		return false;
	}
	const auto* ControlledPawn = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!IsLocalController() || !IsValid(Terminal) || !ControlledPawn || ControlledPawn->IsDead() ||
		ControlledPawn->IsMountedInVehicle() || !Terminal->IsWithinInteractionDistance(ControlledPawn) ||
		IsIntroMap() || IsOpeningScenarioMap() || bDialogueSequenceActive || IsPauseMenuOpen() ||
		IsInventoryUiOpen() || IsHousingModeOpen() || WaveDefensePanel ||
		(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport()) ||
		UGameplayStatics::IsGamePaused(this) || IsMoveInputIgnored() || IsLookInputIgnored()) return false;

	const TSubclassOf<UIconEditorGPT6Astra> PanelClass = Terminal->ResolveEditorWidgetClass();
	if (!PanelClass) return false;
	IconEditorPanel = CreateWidget<UIconEditorGPT6Astra>(this, PanelClass);
	if (!IconEditorPanel) return false;
	IconEditorPanel->AddToViewport(950);
	if (!IconEditorPanel->InitializeEditor())
	{
		IconEditorPanel->RemoveFromParent();
		IconEditorPanel = nullptr;
		return false;
	}
	ActiveIconEditorTerminal = Terminal;
	IconEditorPanel->OnCloseRequested.AddDynamic(this, &ATunaSweeperPlayerController::CloseIconEditorPanel);
	CancelPawnGameplayActions();
	// A local editor must never pause another player's network session.
	bIconEditorPausedGame = GetNetMode() == NM_Standalone && SetPause(true);
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	ApplyDefaultGameInputMode();
	IconEditorPanel->SetKeyboardFocus();
	return true;
}

void ATunaSweeperPlayerController::CloseIconEditorPanel()
{
	if (!IconEditorPanel) return;
	IconEditorPanel->OnCloseRequested.RemoveAll(this);
	IconEditorPanel->RemoveFromParent();
	IconEditorPanel = nullptr;
	ActiveIconEditorTerminal.Reset();
	if (bIconEditorPausedGame) SetPause(false);
	bIconEditorPausedGame = false;
	// Release exactly our own pushes, including when another modal took over.
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	ApplyDefaultGameInputModeInternal(false);
}

void ATunaSweeperPlayerController::UpdateIconEditorInteraction()
{
	if (!IconEditorPanel) return;
	const auto* ControlledPawn = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!IconEditorPanel->IsInViewport() || !ControlledPawn || ControlledPawn->IsDead() ||
		!ActiveIconEditorTerminal.IsValid() || !ActiveIconEditorTerminal->IsWithinInteractionDistance(ControlledPawn))
	{
		CloseIconEditorPanel();
	}
}
