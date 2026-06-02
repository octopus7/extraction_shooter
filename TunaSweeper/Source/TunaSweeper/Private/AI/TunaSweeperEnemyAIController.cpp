#include "AI/TunaSweeperEnemyAIController.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Kismet/GameplayStatics.h"
#include "TunaSweeperCollisionChannels.h"
#include "TimerManager.h"

namespace
{
	float GetRandomizedValue(float BaseValue, const FVector2D& OffsetRange, float MinValue)
	{
		const float MinOffset = FMath::Min(OffsetRange.X, OffsetRange.Y);
		const float MaxOffset = FMath::Max(OffsetRange.X, OffsetRange.Y);
		return FMath::Max(MinValue, BaseValue + FMath::FRandRange(MinOffset, MaxOffset));
	}

	FVector GetPlanarDirectionToTarget(const AActor* SourceActor, const AActor* TargetActor)
	{
		if (!SourceActor || !TargetActor)
		{
			return FVector::ZeroVector;
		}

		FVector ToTarget = TargetActor->GetActorLocation() - SourceActor->GetActorLocation();
		ToTarget.Z = 0.0f;
		return ToTarget.GetSafeNormal();
	}
}

ATunaSweeperEnemyAIController::ATunaSweeperEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATunaSweeperEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MoveRangedCombatState(DeltaSeconds);
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
	if (DistanceToPlayer > ResolveTrackingRange())
	{
		ClearCombatTarget();
		return;
	}

	CurrentTargetActor = PlayerPawn;
	SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);

	ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn);
	if (EnemyCharacter && !EnemyCharacter->UsesMeleeAttack())
	{
		bIsClosingDistance = false;
		UpdateRangedCombatState(DistanceToPlayer, PlayerPawn, EnemyCharacter);
		return;
	}

	UpdateApproachState(DistanceToPlayer, ResolveApproachStartRange(), ResolveApproachStopRange());

	if (bIsClosingDistance || DistanceToPlayer > ResolveAttackRange())
	{
		return;
	}

	UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	const float ResolvedAttackCooldownSeconds = ResolveAttackCooldownSeconds();
	if (CurrentTimeSeconds - LastAttackTimeSeconds < ResolvedAttackCooldownSeconds)
	{
		return;
	}

	if (EnemyCharacter && EnemyCharacter->AttackTarget(PlayerPawn))
	{
		LastAttackTimeSeconds = CurrentTimeSeconds;
	}
}

void ATunaSweeperEnemyAIController::UpdateApproachState(
	float DistanceToTarget,
	float InApproachStartRange,
	float InApproachStopRange)
{
	if (bIsClosingDistance)
	{
		if (DistanceToTarget <= InApproachStopRange)
		{
			bIsClosingDistance = false;
			StopMovement();
		}

		return;
	}

	if (DistanceToTarget > InApproachStartRange)
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
	if (DistanceToTarget <= ResolveApproachStopRange())
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

void ATunaSweeperEnemyAIController::UpdateRangedCombatState(
	float DistanceToTarget,
	AActor* TargetActor,
	ATunaSweeperEnemyCharacter* EnemyCharacter)
{
	if (!TargetActor || !EnemyCharacter)
	{
		ClearCombatTarget();
		return;
	}

	UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	const FVector DirectionToTarget = GetPlanarDirectionToTarget(EnemyCharacter, TargetActor);
	if (DirectionToTarget.IsNearlyZero())
	{
		StartRangedHold(DistanceToTarget);
		return;
	}

	const float SafeDangerCloseRange = FMath::Max(0.0f, RangedDangerCloseRange);
	const float SafePreferredRangeMin = FMath::Max(SafeDangerCloseRange, RangedPreferredRangeMin);
	const float SafePreferredRangeMax = FMath::Max(SafePreferredRangeMin, RangedPreferredRangeMax);
	const ETunaSweeperLineOfFireResult LineOfFireResult = EvaluateLineOfFire(TargetActor);

	if (DistanceToTarget <= SafeDangerCloseRange)
	{
		if (RangedCombatState != ETunaSweeperRangedCombatState::KeepDistance ||
			CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds)
		{
			StartRangedKeepDistance(DirectionToTarget);
		}
		return;
	}

	switch (RangedCombatState)
	{
	case ETunaSweeperRangedCombatState::AdvanceBurst:
		if (LineOfFireResult == ETunaSweeperLineOfFireResult::BlockedByIndestructible)
		{
			StartRangedSeekLineOfFire(DistanceToTarget, DirectionToTarget);
			return;
		}
		if (CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds ||
			DistanceToTarget <= SafePreferredRangeMin ||
			FVector::DistSquared2D(EnemyCharacter->GetActorLocation(), RangedMoveGoal) <=
				FMath::Square(FMath::Max(1.0f, RangedMoveGoalAcceptanceRadius)))
		{
			StartRangedHold(DistanceToTarget);
			return;
		}
		return;

	case ETunaSweeperRangedCombatState::HoldFire:
		if (LineOfFireResult == ETunaSweeperLineOfFireResult::BlockedByIndestructible)
		{
			StartRangedSeekLineOfFire(DistanceToTarget, DirectionToTarget);
			return;
		}
		TryRangedAttack(DistanceToTarget, TargetActor, EnemyCharacter, LineOfFireResult);
		if (CurrentTimeSeconds < RangedCombatStateEndTimeSeconds)
		{
			return;
		}
		RangedCombatState = ETunaSweeperRangedCombatState::Idle;
		break;

	case ETunaSweeperRangedCombatState::SeekLineOfFire:
		if (LineOfFireResult != ETunaSweeperLineOfFireResult::BlockedByIndestructible)
		{
			StartRangedHold(DistanceToTarget);
			return;
		}
		if (CurrentTimeSeconds < RangedCombatStateEndTimeSeconds)
		{
			return;
		}
		RangedCombatState = ETunaSweeperRangedCombatState::Idle;
		break;

	case ETunaSweeperRangedCombatState::KeepDistance:
		if (DistanceToTarget >= SafePreferredRangeMin || CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds)
		{
			StartRangedHold(DistanceToTarget);
		}
		return;

	case ETunaSweeperRangedCombatState::Idle:
	default:
		break;
	}

	if (LineOfFireResult == ETunaSweeperLineOfFireResult::BlockedByIndestructible)
	{
		StartRangedSeekLineOfFire(DistanceToTarget, DirectionToTarget);
		return;
	}

	if (DistanceToTarget > SafePreferredRangeMax)
	{
		StartRangedAdvance(DistanceToTarget, DirectionToTarget);
		return;
	}

	StartRangedHold(DistanceToTarget);
	TryRangedAttack(DistanceToTarget, TargetActor, EnemyCharacter, LineOfFireResult);
}

void ATunaSweeperEnemyAIController::MoveRangedCombatState(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f ||
		(RangedCombatState != ETunaSweeperRangedCombatState::AdvanceBurst &&
			RangedCombatState != ETunaSweeperRangedCombatState::SeekLineOfFire &&
			RangedCombatState != ETunaSweeperRangedCombatState::KeepDistance))
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

	UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	if (CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds)
	{
		StopMovement();
		return;
	}

	if (RangedCombatState == ETunaSweeperRangedCombatState::AdvanceBurst &&
		FVector::DistSquared2D(ControlledPawn->GetActorLocation(), RangedMoveGoal) <=
			FMath::Square(FMath::Max(1.0f, RangedMoveGoalAcceptanceRadius)))
	{
		StopMovement();
		return;
	}

	const FVector DirectionToTarget = GetPlanarDirectionToTarget(ControlledPawn, TargetActor);
	if (!DirectionToTarget.IsNearlyZero())
	{
		ControlledPawn->SetActorRotation(FRotator(0.0f, DirectionToTarget.Rotation().Yaw, 0.0f));
	}

	if (!RangedMoveDirection.IsNearlyZero())
	{
		ControlledPawn->AddMovementInput(RangedMoveDirection, 1.0f, true);
	}
}

void ATunaSweeperEnemyAIController::StartRangedAdvance(
	float DistanceToTarget,
	const FVector& DirectionToTarget)
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World || DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	const float PreferredRangeMin = FMath::Max(FMath::Max(0.0f, RangedDangerCloseRange), RangedPreferredRangeMin);
	const float MaxUsefulAdvanceDistance = FMath::Max(0.0f, DistanceToTarget - PreferredRangeMin);
	const float AdvanceDistance = FMath::Min(
		ResolveRangedAdvanceDistance(DistanceToTarget),
		MaxUsefulAdvanceDistance);
	if (AdvanceDistance <= RangedMoveGoalAcceptanceRadius)
	{
		StartRangedHold(DistanceToTarget);
		return;
	}

	const float StrafeSign = FMath::RandBool() ? 1.0f : -1.0f;
	const FVector StrafeDirection =
		FVector::CrossProduct(FVector::UpVector, DirectionToTarget).GetSafeNormal() * StrafeSign;
	RangedMoveDirection = (
		DirectionToTarget +
		StrafeDirection * FMath::Max(0.0f, RangedAdvanceStrafeWeight)).GetSafeNormal();
	if (RangedMoveDirection.IsNearlyZero())
	{
		RangedMoveDirection = DirectionToTarget;
	}

	RangedMoveGoal = ControlledPawn->GetActorLocation() + RangedMoveDirection * AdvanceDistance;
	RangedCombatState = ETunaSweeperRangedCombatState::AdvanceBurst;
	RangedCombatStateEndTimeSeconds = World->GetTimeSeconds() + ResolveRangedMoveDuration(AdvanceDistance);
}

void ATunaSweeperEnemyAIController::StartRangedHold(float DistanceToTarget)
{
	UWorld* World = GetWorld();
	RangedCombatState = ETunaSweeperRangedCombatState::HoldFire;
	RangedMoveDirection = FVector::ZeroVector;
	RangedMoveGoal = FVector::ZeroVector;
	RangedCombatStateEndTimeSeconds =
		(World ? World->GetTimeSeconds() : 0.0) + ResolveRangedHoldSeconds(DistanceToTarget);
	StopMovement();
}

void ATunaSweeperEnemyAIController::StartRangedSeekLineOfFire(
	float DistanceToTarget,
	const FVector& DirectionToTarget)
{
	UWorld* World = GetWorld();
	if (!World || DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	const float StrafeSign = FMath::RandBool() ? 1.0f : -1.0f;
	const FVector StrafeDirection =
		FVector::CrossProduct(FVector::UpVector, DirectionToTarget).GetSafeNormal() * StrafeSign;
	const float ForwardWeight = DistanceToTarget > FMath::Max(RangedPreferredRangeMin, RangedDangerCloseRange)
		? FMath::Max(0.0f, RangedSeekForwardWeight)
		: 0.0f;
	RangedMoveDirection = (StrafeDirection + DirectionToTarget * ForwardWeight).GetSafeNormal();
	RangedCombatState = ETunaSweeperRangedCombatState::SeekLineOfFire;
	RangedCombatStateEndTimeSeconds =
		World->GetTimeSeconds() + GetRandomRangeValue(RangedSeekLineOfFireSeconds, 0.05f);
}

void ATunaSweeperEnemyAIController::StartRangedKeepDistance(const FVector& DirectionToTarget)
{
	UWorld* World = GetWorld();
	if (!World || DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	const float StrafeSign = FMath::RandBool() ? 1.0f : -1.0f;
	const FVector StrafeDirection =
		FVector::CrossProduct(FVector::UpVector, DirectionToTarget).GetSafeNormal() * StrafeSign;
	RangedMoveDirection = (
		-DirectionToTarget +
		StrafeDirection * FMath::Max(0.0f, RangedKeepDistanceStrafeWeight)).GetSafeNormal();
	RangedCombatState = ETunaSweeperRangedCombatState::KeepDistance;
	RangedCombatStateEndTimeSeconds =
		World->GetTimeSeconds() + GetRandomRangeValue(RangedKeepDistanceSeconds, 0.05f);
}

void ATunaSweeperEnemyAIController::TryRangedAttack(
	float DistanceToTarget,
	AActor* TargetActor,
	ATunaSweeperEnemyCharacter* EnemyCharacter,
	ETunaSweeperLineOfFireResult LineOfFireResult)
{
	if (!TargetActor || !EnemyCharacter ||
		LineOfFireResult == ETunaSweeperLineOfFireResult::BlockedByIndestructible ||
		DistanceToTarget > ResolveRangedAttackRange())
	{
		return;
	}

	UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	const float ResolvedAttackCooldownSeconds = ResolveAttackCooldownSeconds();
	if (CurrentTimeSeconds - LastAttackTimeSeconds < ResolvedAttackCooldownSeconds)
	{
		return;
	}

	if (EnemyCharacter->AttackTarget(TargetActor))
	{
		LastAttackTimeSeconds = CurrentTimeSeconds;
	}
}

ETunaSweeperLineOfFireResult ATunaSweeperEnemyAIController::EvaluateLineOfFire(AActor* TargetActor) const
{
	const APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !TargetActor || !World)
	{
		return ETunaSweeperLineOfFireResult::BlockedByIndestructible;
	}

	const FVector TraceStart =
		ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, FMath::Max(0.0f, RangedLineOfFireTraceHeight));
	const FVector TraceEnd =
		TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, FMath::Max(0.0f, RangedTargetTraceHeight));

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperEnemyLineOfFireTrace), false, ControlledPawn);
	QueryParams.AddIgnoredActor(ControlledPawn);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		TunaSweeperCollisionChannels::Projectile,
		QueryParams))
	{
		return ETunaSweeperLineOfFireResult::Clear;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor || HitActor == TargetActor)
	{
		return ETunaSweeperLineOfFireResult::Clear;
	}

	return Cast<ATunaSweeperSandbagCoverActor>(HitActor)
		? ETunaSweeperLineOfFireResult::BlockedByDestructible
		: ETunaSweeperLineOfFireResult::BlockedByIndestructible;
}

float ATunaSweeperEnemyAIController::ResolveRangedAttackRange() const
{
	return FMath::Max(ResolveAttackRange(), FMath::Max(RangedPreferredRangeMin, RangedPreferredRangeMax));
}

float ATunaSweeperEnemyAIController::ResolveRangedAdvanceDistance(float DistanceToTarget) const
{
	if (DistanceToTarget >= FMath::Max(0.0f, RangedLongAdvanceThreshold))
	{
		return GetRandomRangeValue(RangedLongAdvanceDistance, 0.0f);
	}

	return GetRandomRangeValue(RangedMediumAdvanceDistance, 0.0f);
}

float ATunaSweeperEnemyAIController::ResolveRangedHoldSeconds(float DistanceToTarget) const
{
	if (DistanceToTarget >= FMath::Max(0.0f, RangedLongAdvanceThreshold))
	{
		return GetRandomRangeValue(RangedLongHoldSeconds, 0.05f);
	}
	if (DistanceToTarget >= FMath::Max(0.0f, RangedMediumAdvanceThreshold))
	{
		return GetRandomRangeValue(RangedMediumHoldSeconds, 0.05f);
	}

	return GetRandomRangeValue(RangedPreferredHoldSeconds, 0.05f);
}

float ATunaSweeperEnemyAIController::ResolveRangedMoveDuration(float MoveDistance) const
{
	const ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn());
	const UCharacterMovementComponent* MovementComponent = ControlledCharacter
		? ControlledCharacter->GetCharacterMovement()
		: nullptr;
	const float MoveSpeed = MovementComponent ? FMath::Max(1.0f, MovementComponent->MaxWalkSpeed) : 260.0f;
	return FMath::Max(0.25f, MoveDistance / MoveSpeed + 0.25f);
}

float ATunaSweeperEnemyAIController::GetRandomRangeValue(const FVector2D& ValueRange, float MinValue)
{
	const float MinRange = FMath::Min(ValueRange.X, ValueRange.Y);
	const float MaxRange = FMath::Max(ValueRange.X, ValueRange.Y);
	return FMath::Max(MinValue, FMath::FRandRange(MinRange, MaxRange));
}

float ATunaSweeperEnemyAIController::ResolveTrackingRange() const
{
	const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	return EnemyCharacter && EnemyCharacter->UsesMeleeAttack()
		? EnemyCharacter->GetMeleeTrackingRange()
		: EffectiveTrackingRange;
}

float ATunaSweeperEnemyAIController::ResolveAttackRange() const
{
	const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	return EnemyCharacter && EnemyCharacter->UsesMeleeAttack()
		? EnemyCharacter->GetMeleeAttackRange()
		: EffectiveAttackRange;
}

float ATunaSweeperEnemyAIController::ResolveApproachStartRange() const
{
	const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	return EnemyCharacter && EnemyCharacter->UsesMeleeAttack()
		? EnemyCharacter->GetMeleeApproachStartRange()
		: EffectiveApproachStartRange;
}

float ATunaSweeperEnemyAIController::ResolveApproachStopRange() const
{
	const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	return EnemyCharacter && EnemyCharacter->UsesMeleeAttack()
		? EnemyCharacter->GetMeleeApproachStopRange()
		: EffectiveApproachStopRange;
}

float ATunaSweeperEnemyAIController::ResolveAttackCooldownSeconds() const
{
	const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	return EnemyCharacter && EnemyCharacter->UsesMeleeAttack()
		? EnemyCharacter->GetMeleeAttackCooldownSeconds()
		: EffectiveAttackCooldownSeconds;
}

void ATunaSweeperEnemyAIController::ClearCombatTarget()
{
	CurrentTargetActor.Reset();
	bIsClosingDistance = false;
	RangedCombatState = ETunaSweeperRangedCombatState::Idle;
	RangedCombatStateEndTimeSeconds = 0.0;
	RangedMoveDirection = FVector::ZeroVector;
	RangedMoveGoal = FVector::ZeroVector;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}
