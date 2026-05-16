#include "AI/TunaSweeperEnemyAIController.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	float GetRandomizedValue(float BaseValue, const FVector2D& OffsetRange, float MinValue)
	{
		const float MinOffset = FMath::Min(OffsetRange.X, OffsetRange.Y);
		const float MaxOffset = FMath::Max(OffsetRange.X, OffsetRange.Y);
		return FMath::Max(MinValue, BaseValue + FMath::FRandRange(MinOffset, MaxOffset));
	}
}

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

	RandomizeCombatTuning();
	const float InitialUpdateDelay = FMath::FRandRange(0.0f, EffectiveUpdateInterval);
	GetWorldTimerManager().SetTimer(
		UpdateTimerHandle,
		this,
		&ATunaSweeperEnemyAIController::UpdateAttackTarget,
		EffectiveUpdateInterval,
		true,
		InitialUpdateDelay);
}

void ATunaSweeperEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RandomizeCombatTuning();
	UpdateAttackTarget();
}

void ATunaSweeperEnemyAIController::RandomizeCombatTuning()
{
	if (bHasRandomizedCombatTuning)
	{
		return;
	}

	EffectiveUpdateInterval = GetRandomizedValue(UpdateInterval, UpdateIntervalRandomOffset, 0.01f);
	EffectiveApproachStartRange = GetRandomizedValue(ApproachStartRange, ApproachStartRangeRandomOffset, 0.0f);
	const float MaxStopRange = FMath::Max(0.0f, EffectiveApproachStartRange - FMath::Max(0.0f, MinApproachRangeGap));
	EffectiveApproachStopRange = FMath::Clamp(
		GetRandomizedValue(ApproachStopRange, ApproachStopRangeRandomOffset, 0.0f),
		0.0f,
		MaxStopRange);
	EffectiveAttackRange = FMath::Max(
		GetRandomizedValue(AttackRange, AttackRangeRandomOffset, 0.0f),
		EffectiveApproachStartRange);
	EffectiveTrackingRange = FMath::Max(
		GetRandomizedValue(TrackingRange, TrackingRangeRandomOffset, 0.0f),
		EffectiveApproachStartRange);
	EffectiveAttackCooldownSeconds = GetRandomizedValue(AttackCooldownSeconds, AttackCooldownRandomOffset, 0.05f);

	UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	LastAttackTimeSeconds = CurrentTimeSeconds - FMath::FRandRange(0.0f, EffectiveAttackCooldownSeconds);
	bHasRandomizedCombatTuning = true;
}

void ATunaSweeperEnemyAIController::UpdateAttackTarget()
{
	RandomizeCombatTuning();

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
	if (DistanceToPlayer > EffectiveTrackingRange)
	{
		ClearCombatTarget();
		return;
	}

	CurrentTargetActor = PlayerPawn;
	SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);
	UpdateApproachState(DistanceToPlayer);

	if (bIsClosingDistance || DistanceToPlayer > EffectiveAttackRange)
	{
		return;
	}

	UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	if (CurrentTimeSeconds - LastAttackTimeSeconds < EffectiveAttackCooldownSeconds)
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
	if (bIsClosingDistance)
	{
		if (DistanceToTarget <= EffectiveApproachStopRange)
		{
			bIsClosingDistance = false;
			StopMovement();
		}

		return;
	}

	if (DistanceToTarget > EffectiveApproachStartRange)
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
	if (DistanceToTarget <= EffectiveApproachStopRange)
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
