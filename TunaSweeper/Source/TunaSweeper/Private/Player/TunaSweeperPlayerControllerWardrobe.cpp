#include "Player/TunaSweeperPlayerController.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperWardrobeActor.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"

bool ATunaSweeperPlayerController::IsWardrobeInteractionValid() const
{
	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	const ATunaSweeperWardrobeActor* Wardrobe = ActiveWardrobeActor.Get();
	return IsLocalController() && IsBunkerMap() &&
		IsValid(PlayerCharacter) && PlayerCharacter->GetController() == this &&
		!PlayerCharacter->IsDead() && !PlayerCharacter->IsMountedInVehicle() &&
		IsValid(Wardrobe) && Wardrobe->GetWorld() == GetWorld() && !Wardrobe->IsHidden() &&
		Wardrobe->IsWithinInteractionDistance(PlayerCharacter) &&
		!bDialogueSequenceActive && !IsHousingModeOpen() && !IsPauseMenuOpen() &&
		!(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport());
}

bool ATunaSweeperPlayerController::OpenWardrobePanel(ATunaSweeperWardrobeActor* WardrobeActor)
{
	ActiveWardrobeActor = WardrobeActor;
	if (!IsWardrobeInteractionValid())
	{
		CloseWardrobePanel();
		return false;
	}

	EnsureGameHudWidget();
	// Creating the HUD initializes its mode and clears any previous interaction.
	ActiveWardrobeActor = WardrobeActor;
	if (!IsWardrobeInteractionValid() || !GameHudWidget || !GameHudWidget->ShowWardrobePanel())
	{
		CloseWardrobePanel();
		return false;
	}
	CancelPawnGameplayActions();
	ApplyDefaultGameInputMode();
	return true;
}

bool ATunaSweeperPlayerController::TryEquipWardrobeOutfit(FName OutfitId)
{
	if (!GameHudWidget || GameHudWidget->GetHudMode() != ETunaSweeperHudMode::Wardrobe ||
		!IsWardrobeInteractionValid())
	{
		CloseWardrobePanel();
		return false;
	}
	UTunaSweeperGameInstance* Instance = GetGameInstance<UTunaSweeperGameInstance>();
	return Instance && Instance->TryEquipOutfit(OutfitId);
}

void ATunaSweeperPlayerController::CloseWardrobePanel()
{
	ActiveWardrobeActor.Reset();
	const bool bWasWardrobeOpen = GameHudWidget && GameHudWidget->GetHudMode() == ETunaSweeperHudMode::Wardrobe;
	if (!bWasWardrobeOpen)
	{
		return;
	}
	GameHudWidget->SetHudMode(ETunaSweeperHudMode::None);
	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	// Death and other modal systems own their input locks.
	if (IsLocalController() && IsValid(PlayerCharacter) && !PlayerCharacter->IsDead() &&
		!bDialogueSequenceActive && !IsHousingModeOpen() && !IsPauseMenuOpen() &&
		!(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport()))
	{
		ApplyDefaultGameInputMode();
	}
}

void ATunaSweeperPlayerController::UpdateWardrobeInteraction()
{
	if (GameHudWidget && GameHudWidget->GetHudMode() == ETunaSweeperHudMode::Wardrobe)
	{
		if (!IsWardrobeInteractionValid())
		{
			CloseWardrobePanel();
		}
	}
	else
	{
		ActiveWardrobeActor.Reset();
	}
}
