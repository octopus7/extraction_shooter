#include "Player/TunaSweeperPlayerController.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperQuestRewardLockerActor.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"

bool ATunaSweeperPlayerController::IsQuestRewardLockerInteractionValid() const
{
	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	const ATunaSweeperQuestRewardLockerActor* QuestRewardLocker = ActiveQuestRewardLockerActor.Get();
	return IsLocalController() && IsBunkerMap() &&
		IsValid(PlayerCharacter) && PlayerCharacter->GetController() == this &&
		!PlayerCharacter->IsDead() && !PlayerCharacter->IsMountedInVehicle() &&
		IsValid(QuestRewardLocker) && QuestRewardLocker->GetWorld() == GetWorld() && !QuestRewardLocker->IsHidden() &&
		QuestRewardLocker->IsWithinInteractionDistance(PlayerCharacter) &&
		!bDialogueSequenceActive && !IsHousingModeOpen() && !IsPauseMenuOpen() &&
		!(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport());
}

bool ATunaSweeperPlayerController::OpenQuestRewardLockerPanel(ATunaSweeperQuestRewardLockerActor* QuestRewardLockerActor)
{
	ActiveQuestRewardLockerActor = QuestRewardLockerActor;
	if (!IsQuestRewardLockerInteractionValid())
	{
		CloseQuestRewardLockerPanel();
		return false;
	}

	EnsureGameHudWidget();
	// Creating the HUD initializes its mode and clears any previous interaction.
	ActiveQuestRewardLockerActor = QuestRewardLockerActor;
	if (!IsQuestRewardLockerInteractionValid() || !GameHudWidget || !GameHudWidget->ShowQuestRewardLockerPanel())
	{
		CloseQuestRewardLockerPanel();
		return false;
	}
	CancelPawnGameplayActions();
	ApplyDefaultGameInputMode();
	return true;
}

bool ATunaSweeperPlayerController::TryReceiveQuestReward(int32 Index)
{
	if (!GameHudWidget || GameHudWidget->GetHudMode() != ETunaSweeperHudMode::QuestRewardLocker ||
		!IsQuestRewardLockerInteractionValid())
	{
		CloseQuestRewardLockerPanel();
		return false;
	}
	UTunaSweeperGameInstance* Instance = GetGameInstance<UTunaSweeperGameInstance>();
	return Instance && Instance->ReceivePendingQuestReward(Index);
}

void ATunaSweeperPlayerController::CloseQuestRewardLockerPanel()
{
	ActiveQuestRewardLockerActor.Reset();
	const bool bWasQuestRewardLockerOpen = GameHudWidget && GameHudWidget->GetHudMode() == ETunaSweeperHudMode::QuestRewardLocker;
	if (!bWasQuestRewardLockerOpen)
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

void ATunaSweeperPlayerController::UpdateQuestRewardLockerInteraction()
{
	if (GameHudWidget && GameHudWidget->GetHudMode() == ETunaSweeperHudMode::QuestRewardLocker)
	{
		if (!IsQuestRewardLockerInteractionValid())
		{
			CloseQuestRewardLockerPanel();
		}
	}
	else
	{
		ActiveQuestRewardLockerActor.Reset();
	}
}
