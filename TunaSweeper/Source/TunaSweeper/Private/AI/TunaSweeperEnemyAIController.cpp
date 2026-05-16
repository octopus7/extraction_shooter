#include "AI/TunaSweeperEnemyAIController.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATunaSweeperEnemyAIController::ATunaSweeperEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATunaSweeperEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MoveTowardCurrentTarget(DeltaSeconds);
}

void ATunaSweeperEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		UpdateTimerHandle,
		this,
		&ATunaSweeperEnemyAIController::UpdateAttackTarget,
		FMath::Max(0.01f, UpdateInterval),
		true,
		0.0f);
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
		ClearCombatTarget();
		return;
	}

	if (const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerPawn))
	{
		if (PlayerCharacter->IsDead())
		{
			ClearCombatTarget();
			return;
		}
	}

	const float DistanceToPlayer = FMath::Sqrt(FVector::DistSquared2D(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation()));
	if (DistanceToPlayer > FMath::Max(0.0f, TrackingRange))
	{
		ClearCombatTarget();
		return;
	}

	CurrentTargetActor = PlayerPawn;
	SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);
	UpdateApproachState(DistanceToPlayer);

	if (bIsClosingDistance || DistanceToPlayer > FMath::Max(0.0f, AttackRange))
	{
		return;
	}

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

void ATunaSweeperEnemyAIController::UpdateApproachState(float DistanceToTarget)
{
	const float StartRange = FMath::Max(0.0f, ApproachStartRange);
	const float StopRange = FMath::Clamp(ApproachStopRange, 0.0f, StartRange);

	if (bIsClosingDistance)
	{
		if (DistanceToTarget <= StopRange)
		{
			bIsClosingDistance = false;
			StopMovement();
		}

		return;
	}

	if (DistanceToTarget > StartRange)
	{
		bIsClosingDistance = true;
	}
}

void ATunaSweeperEnemyAIController::MoveTowardCurrentTarget(float DeltaSeconds)
{
	if (!bIsClosingDistance || DeltaSeconds <= 0.0f)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	AActor* TargetActor = CurrentTargetActor.Get();
	if (!ControlledPawn || !TargetActor)
	{
		ClearCombatTarget();
		return;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
	ToTarget.Z = 0.0f;

	const float DistanceToTarget = ToTarget.Size();
	const float StartRange = FMath::Max(0.0f, ApproachStartRange);
	const float StopRange = FMath::Clamp(ApproachStopRange, 0.0f, StartRange);
	if (DistanceToTarget <= StopRange)
	{
		bIsClosingDistance = false;
		StopMovement();
		return;
	}

	const FVector MoveDirection = ToTarget.GetSafeNormal();
	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	ControlledPawn->SetActorRotation(FRotator(0.0f, MoveDirection.Rotation().Yaw, 0.0f));
	ControlledPawn->AddMovementInput(MoveDirection, 1.0f);
}

void ATunaSweeperEnemyAIController::ClearCombatTarget()
{
	CurrentTargetActor.Reset();
	bIsClosingDistance = false;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}
