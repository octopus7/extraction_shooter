#include "Player/TunaSweeperPlayerController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperPauseMenuWidget.h"

bool ATunaSweeperPlayerController::IsPauseMenuKey(const FKey& Key, const UWorld* World)
{
	if (Key == EKeys::Escape) return true;
#if WITH_EDITOR
	if (Key == EKeys::K && World && World->WorldType == EWorldType::PIE)
	{
		const auto* Mapping = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Player.IMC_Player"));
		return Mapping && !Mapping->GetMappings().ContainsByPredicate(
			[](const FEnhancedActionKeyMapping& Entry) { return Entry.Key == EKeys::K; });
	}
#endif
	return false;
}

void ATunaSweeperPlayerController::TogglePauseMenu()
{
	if (PauseMenuWidget)
	{
		ResumeFromPauseMenu();
		return;
	}
	const auto* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!IsLocalController() || IsIntroMap() || IsOpeningScenarioMap() ||
		bDialogueSequenceActive || (ControlledCharacter && ControlledCharacter->IsDead()) ||
		(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport())) return;
	if (IsHousingModeOpen())
	{
		HandleHousingCancel();
		return;
	}
	if (GameHudWidget && GameHudWidget->GetHudMode() != ETunaSweeperHudMode::None)
	{
		GameHudWidget->ToggleInventoryOnlyPanel();
		return;
	}
	if (!GetWorld() || UGameplayStatics::IsGamePaused(this)) return;
	PauseMenuWidget = CreateWidget<UTunaSweeperPauseMenuWidget>(this, UTunaSweeperPauseMenuWidget::StaticClass());
	if (!PauseMenuWidget) return;
	CancelPawnGameplayActions();
	if (!SetPause(true))
	{
		PauseMenuWidget = nullptr;
		return;
	}
	PauseMenuWidget->InitializePauseMenu(IsRaidMap());
	PauseMenuWidget->AddToViewport(900);
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	ApplyDefaultGameInputMode();
	PauseMenuWidget->SetKeyboardFocus();
}

void ATunaSweeperPlayerController::ResumeFromPauseMenu()
{
	if (!PauseMenuWidget) return;
	PauseMenuWidget->RemoveFromParent();
	PauseMenuWidget = nullptr;
	SetPause(false);
	ApplyDefaultGameInputMode();
}

void ATunaSweeperPlayerController::ExitFromPauseMenu(bool bQuitGame)
{
	if (!PauseMenuWidget) return;
	auto* Instance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!Instance || !Instance->SaveForGameplayExit(IsRaidMap()))
	{
		PauseMenuWidget->ShowExitFailure();
		return;
	}
	ResumeFromPauseMenu();
	if (bQuitGame)
	{
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/IntroMap"));
	}
}
