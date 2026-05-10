#include "AI/TunaSweeperEnemyAIController.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATunaSweeperEnemyAIController::ATunaSweeperEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATunaSweeperEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &ATunaSweeperEnemyAIController::UpdateChaseTarget, UpdateInterval, true, 0.0f);
}

void ATunaSweeperEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UpdateChaseTarget();
}

void ATunaSweeperEnemyAIController::UpdateChaseTarget()
{
	APawn* ControlledPawn = GetPawn();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!ControlledPawn || !PlayerPawn)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);
	MoveToActor(PlayerPawn, AcceptanceRadius, true, true, true);
}
