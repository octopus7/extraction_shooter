#include "AI/TunaSweeperEnemyAIController.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
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

bool ATunaSweeperEnemyAIController::GetCombatDebugSnapshot(FTunaSweeperEnemyCombatDebugSnapshot& OutSnapshot) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	OutSnapshot = FTunaSweeperEnemyCombatDebugSnapshot();
	OutSnapshot.bIsCombatEngaged = bIsCombatEngaged;
	OutSnapshot.TrackingRange = ResolveTrackingRange();
	OutSnapshot.VisionAngleDegrees = FMath::Clamp(CombatVisionAngleDegrees, 0.0f, 360.0f);
	OutSnapshot.FacingDirection = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	if (OutSnapshot.FacingDirection.IsNearlyZero())
	{
		OutSnapshot.FacingDirection = NonCombatFacingDirection.GetSafeNormal2D();
	}
	if (OutSnapshot.FacingDirection.IsNearlyZero())
	{
		OutSnapshot.FacingDirection = FVector::ForwardVector;
	}

	const UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	auto SetTimedState = [&OutSnapshot, CurrentTimeSeconds](const TCHAR* Label, double EndTimeSeconds, float MaxSeconds)
	{
		OutSnapshot.StateLabel = Label;
		OutSnapshot.MaxStateSeconds = FMath::Max(0.0f, MaxSeconds);
		OutSnapshot.RemainingStateSeconds = OutSnapshot.MaxStateSeconds > 0.0f
			? FMath::Clamp(static_cast<float>(EndTimeSeconds - CurrentTimeSeconds), 0.0f, OutSnapshot.MaxStateSeconds)
			: 0.0f;
	};

	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Suspicious)
	{
		SetTimedState(TEXT("Suspicious Search"), AwarenessStateEndTimeSeconds, FMath::Max(SuspicionSearchSeconds.X, SuspicionSearchSeconds.Y));
		return true;
	}

	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted)
	{
		SetTimedState(
			TEXT("Alerted"),
			AwarenessStateEndTimeSeconds,
			FMath::Max(0.0f, AlertReactionSeconds) + FMath::Max(0.0f, AlertPreCombatDelaySeconds));
		return true;
	}

	if (!bIsCombatEngaged)
	{
		if (NonCombatState == ETunaSweeperNonCombatState::Wander)
		{
			SetTimedState(TEXT("Wander"), NonCombatStateEndTimeSeconds, FMath::Max(IdleSeconds.Y, IdleSeconds.X));
		}
		else
		{
			SetTimedState(TEXT("Idle"), NonCombatStateEndTimeSeconds, FMath::Max(IdleSeconds.X, IdleSeconds.Y));
		}
		return true;
	}

	if (const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn); EnemyCharacter && EnemyCharacter->UsesMeleeAttack())
	{
		OutSnapshot.StateLabel = bIsClosingDistance ? TEXT("Advance") : TEXT("Attack");
		return true;
	}

	switch (RangedCombatState)
	{
	case ETunaSweeperRangedCombatState::AdvanceBurst:
		SetTimedState(TEXT("Advance Burst"), RangedCombatStateEndTimeSeconds, FMath::Max(RangedLongHoldSeconds.Y, RangedMediumHoldSeconds.Y));
		break;
	case ETunaSweeperRangedCombatState::HoldFire:
		SetTimedState(TEXT("Hold Fire"), RangedCombatStateEndTimeSeconds, FMath::Max(RangedPreferredHoldSeconds.X, RangedPreferredHoldSeconds.Y));
		break;
	case ETunaSweeperRangedCombatState::SeekLineOfFire:
		SetTimedState(TEXT("Seek Line"), RangedCombatStateEndTimeSeconds, FMath::Max(RangedSeekLineOfFireSeconds.X, RangedSeekLineOfFireSeconds.Y));
		break;
	case ETunaSweeperRangedCombatState::KeepDistance:
		SetTimedState(TEXT("Keep Distance"), RangedCombatStateEndTimeSeconds, FMath::Max(RangedKeepDistanceSeconds.X, RangedKeepDistanceSeconds.Y));
		break;
	default:
		OutSnapshot.StateLabel = TEXT("Combat");
		break;
	}

	return true;
}

void ATunaSweeperEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAwarenessState(DeltaSeconds);
	UpdateNonCombatState(DeltaSeconds);
	MoveRangedCombatState(DeltaSeconds);
	MoveTowardCurrentTarget(DeltaSeconds);
	DrawCombatDebug();
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

	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
		{
			NoiseSubsystem->OnNoiseReported.AddUObject(this, &ATunaSweeperEnemyAIController::HandleNoiseReported);
		}
	}
}

void ATunaSweeperEnemyAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
		{
			NoiseSubsystem->OnNoiseReported.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RandomizeCombatTuning();
	StartNonCombatIdle();
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
	if (!ControlledPawn)
	{
		ClearCombatTarget();
		return;
	}
	if (!PlayerPawn)
	{
		if (bIsCombatEngaged || CurrentTargetActor.IsValid())
		{
			ClearCombatTarget();
		}
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
	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted)
	{
		return;
	}

	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Suspicious)
	{
		if (CanAcquireCombatTarget(PlayerPawn, DistanceToPlayer))
		{
			StartAlerted(PlayerPawn);
		}
		return;
	}

	if (bIsCombatEngaged && DistanceToPlayer > ResolveCombatDisengageRange())
	{
		ClearCombatTarget();
		return;
	}

	if (!bIsCombatEngaged && !CanAcquireCombatTarget(PlayerPawn, DistanceToPlayer))
	{
		if (CurrentTargetActor.IsValid())
		{
			ClearCombatTarget();
		}
		else
		{
			ClearFocus(EAIFocusPriority::Gameplay);
		}
		return;
	}

	if (!bIsCombatEngaged)
	{
		StartAlerted(PlayerPawn);
		return;
	}

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

void ATunaSweeperEnemyAIController::NotifySuspicionAtLocation(const FVector& InSuspicionLocation)
{
	if (InSuspicionLocation.ContainsNaN() || bIsCombatEngaged || AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted)
	{
		return;
	}

	StartSuspicion(InSuspicionLocation);
}

void ATunaSweeperEnemyAIController::UpdateAwarenessState(float DeltaSeconds)
{
	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Unaware ||
		AwarenessState == ETunaSweeperEnemyAwarenessState::Combat)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		ClearCombatTarget();
		return;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted)
	{
		AActor* TargetActor = CurrentTargetActor.Get();
		if (!TargetActor)
		{
			ClearCombatTarget();
			return;
		}

		FVector DirectionToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
		DirectionToTarget.Z = 0.0f;
		if (!DirectionToTarget.IsNearlyZero())
		{
			ControlledPawn->SetActorRotation(FRotator(0.0f, DirectionToTarget.Rotation().Yaw, 0.0f));
		}

		if (CurrentTimeSeconds >= AwarenessStateEndTimeSeconds)
		{
			EnterCombat();
		}
		return;
	}

	FVector DirectionToSuspicion = SuspicionLocation - ControlledPawn->GetActorLocation();
	DirectionToSuspicion.Z = 0.0f;
	if (!DirectionToSuspicion.IsNearlyZero())
	{
		const float ElapsedSeconds = FMath::Max(0.0f, static_cast<float>(CurrentTimeSeconds - AwarenessStateStartTimeSeconds));
		const float SweepAngleDegrees = FMath::Sin(ElapsedSeconds * 4.0f) * FMath::Max(0.0f, SearchSweepHalfAngleDegrees);
		const FVector SearchDirection = DirectionToSuspicion.GetSafeNormal().RotateAngleAxis(SweepAngleDegrees, FVector::UpVector);
		ControlledPawn->SetActorRotation(FRotator(0.0f, SearchDirection.Rotation().Yaw, 0.0f));
	}

	if (CurrentTimeSeconds >= AwarenessStateEndTimeSeconds)
	{
		ClearCombatTarget();
	}
}

void ATunaSweeperEnemyAIController::UpdateNonCombatState(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || AwarenessState != ETunaSweeperEnemyAwarenessState::Unaware || bIsCombatEngaged || CurrentTargetActor.IsValid())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	if (CurrentTimeSeconds >= NonCombatStateEndTimeSeconds)
	{
		if (NonCombatState == ETunaSweeperNonCombatState::Idle)
		{
			StartNonCombatWander();
		}
		else
		{
			StartNonCombatIdle();
		}
	}

	if (NonCombatState != ETunaSweeperNonCombatState::Wander)
	{
		return;
	}

	if (NonCombatFacingDirection.IsNearlyZero())
	{
		NonCombatFacingDirection = GetRandomPlanarDirection();
	}

	ControlledPawn->SetActorRotation(FRotator(0.0f, NonCombatFacingDirection.Rotation().Yaw, 0.0f));

	float InputScale = 1.0f;
	if (const ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		if (const UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement())
		{
			const float MaxWalkSpeed = FMath::Max(1.0f, MovementComponent->MaxWalkSpeed);
			InputScale = FMath::Clamp(FMath::Max(0.0f, WanderMoveSpeed) / MaxWalkSpeed, 0.0f, 1.0f);
		}
	}

	ControlledPawn->AddMovementInput(NonCombatFacingDirection, InputScale, true);
}

void ATunaSweeperEnemyAIController::StartNonCombatIdle()
{
	UWorld* World = GetWorld();
	NonCombatState = ETunaSweeperNonCombatState::Idle;
	NonCombatStateEndTimeSeconds =
		(World ? World->GetTimeSeconds() : 0.0) + GetRandomRangeValue(IdleSeconds, 0.05f);
	StopMovement();
}

void ATunaSweeperEnemyAIController::StartNonCombatWander()
{
	UWorld* World = GetWorld();
	NonCombatState = ETunaSweeperNonCombatState::Wander;
	NonCombatFacingDirection = GetRandomPlanarDirection();
	NonCombatStateEndTimeSeconds =
		(World ? World->GetTimeSeconds() : 0.0) + GetRandomRangeValue(WanderSeconds, 0.05f);
}

void ATunaSweeperEnemyAIController::StartSuspicion(const FVector& InSuspicionLocation)
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return;
	}

	AwarenessState = ETunaSweeperEnemyAwarenessState::Suspicious;
	AwarenessStateStartTimeSeconds = World->GetTimeSeconds();
	AwarenessStateEndTimeSeconds = AwarenessStateStartTimeSeconds + GetRandomRangeValue(SuspicionSearchSeconds, 0.05f);
	SuspicionLocation = InSuspicionLocation;
	CurrentTargetActor.Reset();
	bIsClosingDistance = false;
	bIsOpeningHold = false;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	if (ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn))
	{
		EnemyCharacter->SetAlertIndicatorVisible(false);
	}
}

void ATunaSweeperEnemyAIController::StartAlerted(AActor* TargetActor)
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World || !TargetActor)
	{
		ClearCombatTarget();
		return;
	}

	AwarenessState = ETunaSweeperEnemyAwarenessState::Alerted;
	AwarenessStateStartTimeSeconds = World->GetTimeSeconds();
	AwarenessStateEndTimeSeconds = AwarenessStateStartTimeSeconds +
		FMath::Max(0.0f, AlertReactionSeconds) + FMath::Max(0.0f, AlertPreCombatDelaySeconds);
	CurrentTargetActor = TargetActor;
	bIsCombatEngaged = false;
	bIsClosingDistance = false;
	bIsOpeningHold = false;
	RangedCombatState = ETunaSweeperRangedCombatState::Idle;
	StopMovement();
	SetFocus(TargetActor, EAIFocusPriority::Gameplay);

	if (ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn))
	{
		EnemyCharacter->SetAlertIndicatorVisible(true);
	}
}

void ATunaSweeperEnemyAIController::EnterCombat()
{
	APawn* ControlledPawn = GetPawn();
	AActor* TargetActor = CurrentTargetActor.Get();
	if (!ControlledPawn || !TargetActor)
	{
		ClearCombatTarget();
		return;
	}

	AwarenessState = ETunaSweeperEnemyAwarenessState::Combat;
	bIsCombatEngaged = true;
	SetFocus(TargetActor, EAIFocusPriority::Gameplay);

	ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn);
	if (EnemyCharacter)
	{
		EnemyCharacter->SetAlertIndicatorVisible(false);
	}

	const float DistanceToTarget = FMath::Sqrt(FVector::DistSquared2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation()));
	if (EnemyCharacter && !EnemyCharacter->UsesMeleeAttack())
	{
		bIsClosingDistance = false;
		StartRangedHold(DistanceToTarget);
		return;
	}

	UpdateApproachState(DistanceToTarget, ResolveApproachStartRange(), ResolveApproachStopRange());
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
			DistanceToTarget <= SafePreferredRangeMax ||
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
		bIsOpeningHold = false;
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
	const float PreferredRangeMax = FMath::Max(PreferredRangeMin, RangedPreferredRangeMax);
	const float MaxUsefulAdvanceDistance = FMath::Max(0.0f, DistanceToTarget - PreferredRangeMax);
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
	bIsOpeningHold = false;
	RangedCombatStateEndTimeSeconds = World->GetTimeSeconds() + ResolveRangedMoveDuration(AdvanceDistance);
}

void ATunaSweeperEnemyAIController::StartRangedHold(float DistanceToTarget, bool bOpeningHold)
{
	UWorld* World = GetWorld();
	RangedCombatState = ETunaSweeperRangedCombatState::HoldFire;
	RangedMoveDirection = FVector::ZeroVector;
	RangedMoveGoal = FVector::ZeroVector;
	bIsOpeningHold = bOpeningHold;
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
	bIsOpeningHold = false;
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
	bIsOpeningHold = false;
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
		DistanceToTarget > (bIsOpeningHold
			? FMath::Max(ResolveRangedAttackRange(), ResolveTrackingRange())
			: ResolveRangedAttackRange()))
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

bool ATunaSweeperEnemyAIController::CanAcquireCombatTarget(AActor* TargetActor, float DistanceToTarget) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !TargetActor || DistanceToTarget > ResolveTrackingRange())
	{
		return false;
	}

	FVector DirectionToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
	DirectionToTarget.Z = 0.0f;
	if (DirectionToTarget.IsNearlyZero())
	{
		return true;
	}
	DirectionToTarget.Normalize();

	const float SafeVisionAngleDegrees = FMath::Clamp(CombatVisionAngleDegrees, 0.0f, 360.0f);
	if (SafeVisionAngleDegrees < 360.0f)
	{
		FVector ForwardDirection = ControlledPawn->GetActorForwardVector();
		ForwardDirection.Z = 0.0f;
		if (ForwardDirection.IsNearlyZero())
		{
			ForwardDirection = NonCombatFacingDirection.IsNearlyZero()
				? FVector::ForwardVector
				: NonCombatFacingDirection;
		}
		ForwardDirection.Normalize();

		const float MinForwardDot = FMath::Cos(FMath::DegreesToRadians(SafeVisionAngleDegrees * 0.5f));
		if (FVector::DotProduct(ForwardDirection, DirectionToTarget) < MinForwardDot)
		{
			return false;
		}
	}

	return EvaluateLineOfFire(TargetActor) != ETunaSweeperLineOfFireResult::BlockedByIndestructible;
}

void ATunaSweeperEnemyAIController::HandleNoiseReported(const FTunaSweeperNoiseEvent& NoiseEvent)
{
	if (bIsCombatEngaged || AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted ||
		NoiseEvent.Loudness <= 0.0f || NoiseEvent.MaxRange <= 0.0f)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World ||
		NoiseEvent.SourceActor == ControlledPawn || NoiseEvent.InstigatorActor == ControlledPawn ||
		(NoiseEvent.SourceActor && NoiseEvent.SourceActor->GetOwner() == ControlledPawn) ||
		(NoiseEvent.InstigatorActor && NoiseEvent.InstigatorActor->GetOwner() == ControlledPawn))
	{
		return;
	}

	UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>();
	if (!NoiseSubsystem)
	{
		return;
	}

	FTunaSweeperHeardNoiseEvent HeardNoise;
	if (!NoiseSubsystem->CalculateHeardNoiseAtLocation(
		NoiseEvent,
		ControlledPawn->GetActorLocation(),
		HearingRange,
		HearingSensitivity,
		HearingMinimumStrength,
		HeardNoise))
	{
		return;
	}

	StartSuspicion(HeardNoise.SourceLocation);
}

void ATunaSweeperEnemyAIController::DrawCombatDebug() const
{
	if (!bDrawCombatDebug)
	{
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return;
	}

	const FVector Origin = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 16.0f);
	if (!bIsCombatEngaged)
	{
		const float Range = FMath::Max(0.0f, ResolveTrackingRange());
		if (Range <= 0.0f)
		{
			return;
		}

		FVector ForwardDirection = ControlledPawn->GetActorForwardVector();
		ForwardDirection.Z = 0.0f;
		if (ForwardDirection.IsNearlyZero())
		{
			ForwardDirection = NonCombatFacingDirection.IsNearlyZero()
				? FVector::ForwardVector
				: NonCombatFacingDirection;
		}
		ForwardDirection.Normalize();

		const float HalfAngleDegrees = FMath::Clamp(CombatVisionAngleDegrees, 0.0f, 360.0f) * 0.5f;
		const FVector LeftDirection = ForwardDirection.RotateAngleAxis(-HalfAngleDegrees, FVector::UpVector);
		const FVector RightDirection = ForwardDirection.RotateAngleAxis(HalfAngleDegrees, FVector::UpVector);
		const FColor VisionColor(0, 190, 255, 80);
		DrawDebugLine(World, Origin, Origin + LeftDirection * Range, VisionColor, false, 0.0f, 0, 1.0f);
		DrawDebugLine(World, Origin, Origin + RightDirection * Range, VisionColor, false, 0.0f, 0, 1.0f);

		const int32 SegmentCount = 24;
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			if ((SegmentIndex % 2) != 0)
			{
				continue;
			}

			const float StartAlpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float EndAlpha = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float StartAngle = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, StartAlpha);
			const float EndAngle = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, EndAlpha);
			const FVector SegmentStart = Origin + ForwardDirection.RotateAngleAxis(StartAngle, FVector::UpVector) * Range;
			const FVector SegmentEnd = Origin + ForwardDirection.RotateAngleAxis(EndAngle, FVector::UpVector) * Range;
			DrawDebugLine(World, SegmentStart, SegmentEnd, VisionColor, false, 0.0f, 0, 1.0f);
		}
		return;
	}

	const AActor* TargetActor = CurrentTargetActor.Get();
	if (!TargetActor)
	{
		return;
	}

	FVector DirectionBeyondTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
	DirectionBeyondTarget.Z = 0.0f;
	const float DistanceToTarget = DirectionBeyondTarget.Size();
	const float ExtraDisengageDistance = ResolveCombatDisengageRange() - DistanceToTarget;
	if (ExtraDisengageDistance <= 0.0f || DirectionBeyondTarget.IsNearlyZero())
	{
		return;
	}

	DirectionBeyondTarget.Normalize();
	const FVector SegmentStart = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 16.0f);
	const FVector SegmentEnd = SegmentStart + DirectionBeyondTarget * ExtraDisengageDistance;
	const int32 SegmentCount = 12;
	const FColor DisengageColor(255, 160, 40, 64);
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		if ((SegmentIndex % 2) != 0)
		{
			continue;
		}

		const float StartAlpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float EndAlpha = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
		DrawDebugLine(
			World,
			FMath::Lerp(SegmentStart, SegmentEnd, StartAlpha),
			FMath::Lerp(SegmentStart, SegmentEnd, EndAlpha),
			DisengageColor,
			false,
			0.0f,
			0,
			2.0f);
	}
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

float ATunaSweeperEnemyAIController::ResolveCombatDisengageRange() const
{
	return FMath::Max(ResolveTrackingRange(), CombatDisengageRange);
}

float ATunaSweeperEnemyAIController::GetRandomRangeValue(const FVector2D& ValueRange, float MinValue)
{
	const float MinRange = FMath::Min(ValueRange.X, ValueRange.Y);
	const float MaxRange = FMath::Max(ValueRange.X, ValueRange.Y);
	return FMath::Max(MinValue, FMath::FRandRange(MinRange, MaxRange));
}

FVector ATunaSweeperEnemyAIController::GetRandomPlanarDirection()
{
	const float AngleRadians = FMath::FRandRange(0.0f, 2.0f * PI);
	return FVector(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f).GetSafeNormal();
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
	if (ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn()))
	{
		EnemyCharacter->SetAlertIndicatorVisible(false);
	}

	CurrentTargetActor.Reset();
	bIsCombatEngaged = false;
	bIsClosingDistance = false;
	bIsOpeningHold = false;
	AwarenessState = ETunaSweeperEnemyAwarenessState::Unaware;
	AwarenessStateStartTimeSeconds = 0.0;
	AwarenessStateEndTimeSeconds = 0.0;
	SuspicionLocation = FVector::ZeroVector;
	RangedCombatState = ETunaSweeperRangedCombatState::Idle;
	RangedCombatStateEndTimeSeconds = 0.0;
	RangedMoveDirection = FVector::ZeroVector;
	RangedMoveGoal = FVector::ZeroVector;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	StartNonCombatIdle();
}
