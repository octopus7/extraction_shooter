#include "Player/TunaSweeperPlayerController.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Interaction/TunaSweeperDebugArmoryActor.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"

bool ATunaSweeperPlayerController::IsDebugArmoryInteractionValid() const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	const ATunaSweeperDebugArmoryActor* Armory = ActiveDebugArmoryActor.Get();
	return ATunaSweeperDebugArmoryActor::IsArmoryEnabled() && IsLocalController() &&
		IsValid(PlayerCharacter) && PlayerCharacter->GetController() == this &&
		!PlayerCharacter->IsDead() && !PlayerCharacter->IsMountedInVehicle() &&
		IsValid(Armory) && Armory->GetWorld() == GetWorld() && !Armory->IsHidden() &&
		Armory->CanUseArmory(GetPawn()) && !bDialogueSequenceActive &&
		!IsHousingModeOpen() && !IsPauseMenuOpen() &&
		!(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport());
#endif
}

bool ATunaSweeperPlayerController::GetDebugArmoryCatalog(TArray<FTunaSweeperItemDefinition>& OutItems) const
{
	OutItems.Reset();
#if UE_BUILD_SHIPPING
	return false;
#else
	return IsDebugArmoryInteractionValid() && ActiveDebugArmoryActor->GetSupplyCatalog(OutItems);
#endif
}

bool ATunaSweeperPlayerController::OpenDebugArmoryPanel(ATunaSweeperDebugArmoryActor* ArmoryActor)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	ActiveDebugArmoryActor = ArmoryActor;
	if (!IsDebugArmoryInteractionValid())
	{
		CloseDebugArmoryPanel();
		return false;
	}
	EnsureGameHudWidget();
	// HUD initialization resets stale interactions along with the initial mode.
	ActiveDebugArmoryActor = ArmoryActor;
	if (!IsDebugArmoryInteractionValid() || !GameHudWidget || !GameHudWidget->ShowDebugArmoryPanel())
	{
		CloseDebugArmoryPanel();
		return false;
	}
	CancelPawnGameplayActions();
	ApplyDefaultGameInputMode();
	return true;
#endif
}

bool ATunaSweeperPlayerController::TrySupplyDebugArmoryItem(int32 ItemId, int32 Quantity)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (!GameHudWidget || GameHudWidget->GetHudMode() != ETunaSweeperHudMode::DebugArmory ||
		!IsDebugArmoryInteractionValid())
	{
		CloseDebugArmoryPanel();
		return false;
	}
	return ActiveDebugArmoryActor->TrySupplyItem(GetPawn(), ItemId, Quantity);
#endif
}

void ATunaSweeperPlayerController::CloseDebugArmoryPanel()
{
	ActiveDebugArmoryActor.Reset();
	if (!GameHudWidget || GameHudWidget->GetHudMode() != ETunaSweeperHudMode::DebugArmory) return;
	GameHudWidget->SetHudMode(ETunaSweeperHudMode::None);
	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (IsLocalController() && IsValid(PlayerCharacter) && !PlayerCharacter->IsDead() &&
		!bDialogueSequenceActive && !IsHousingModeOpen() && !IsPauseMenuOpen() &&
		!(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport()))
	{
		ApplyDefaultGameInputMode();
	}
}

void ATunaSweeperPlayerController::UpdateDebugArmoryInteraction()
{
	if (GameHudWidget && GameHudWidget->GetHudMode() == ETunaSweeperHudMode::DebugArmory)
	{
		if (!IsDebugArmoryInteractionValid()) CloseDebugArmoryPanel();
	}
	else
	{
		ActiveDebugArmoryActor.Reset();
	}
}

void ATunaSweeperPlayerController::OnUnPossess()
{
	CloseDebugArmoryPanel();
	Super::OnUnPossess();
}
