#include "AI/TunaSweeperEnemyAIController.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATunaSweeperEnemyAIController::ATunaSweeperEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATunaSweeperEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(UpdateTimerHandle, this, &ATunaSweeperEnemyAIController::UpdateAttackTarget, UpdateInterval, true, 0.0f);
}

void ATunaSweeperEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UpdateAttackTarget();
}

void ATunaSweeperEnemyAIController::UpdateAttackTarget()
{
	APawn* ControlledPawn = GetPawn();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!ControlledPawn || !PlayerPawn)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	if (const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerPawn))
	{
		if (PlayerCharacter->IsDead())
		{
			ClearFocus(EAIFocusPriority::Gameplay);
			return;
		}
	}

	const float AttackRangeSquared = FMath::Square(FMath::Max(0.0f, AttackRange));
	if (FVector::DistSquared2D(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation()) > AttackRangeSquared)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);

	UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	if (CurrentTimeSeconds - LastAttackTimeSeconds < AttackCooldownSeconds)
	{
		return;
	}

	ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn);
	if (EnemyCharacter && EnemyCharacter->FireProjectileAt(PlayerPawn))
	{
		LastAttackTimeSeconds = CurrentTimeSeconds;
	}
}
