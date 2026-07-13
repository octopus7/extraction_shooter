#include "AI/TunaSweeperEnemyAIController.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Subsystem/TunaSweeperEnemySquadSubsystem.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "TimerManager.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	float GetRandomizedValue(float BaseValue, const FVector2D& OffsetRange, float MinValue)
	{
		return FMath::Max(
			MinValue,
			BaseValue + FMath::FRandRange(
				FMath::Min(OffsetRange.X, OffsetRange.Y),
				FMath::Max(OffsetRange.X, OffsetRange.Y)));
	}

	double GetWorldTimeSeconds(const UObject* Context)
	{
		const UWorld* World = Context ? Context->GetWorld() : nullptr;
		return World ? World->GetTimeSeconds() : 0.0;
	}

	FVector GetPlanarDirection(const FVector& From, const FVector& To)
	{
		FVector Direction = To - From;
		Direction.Z = 0.0f;
		return Direction.GetSafeNormal();
	}

	bool IsUnavailableCombatTarget(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return true;
		}
		if (const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(Actor))
		{
			return PlayerCharacter->IsDead();
		}
		if (const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(Actor))
		{
			return EnemyCharacter->IsDead();
		}
		return false;
	}
}

ATunaSweeperEnemyAIController::ATunaSweeperEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	CachedTeamId = FGenericTeamId::NoTeam;
}

void ATunaSweeperEnemyAIController::SetGenericTeamId(const FGenericTeamId& InTeamId)
{
	CachedTeamId = InTeamId;
	Super::SetGenericTeamId(InTeamId);
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UTunaSweeperFactionComponent* FactionComponent =
			ControlledPawn->FindComponentByClass<UTunaSweeperFactionComponent>())
		{
			FactionComponent->SetFactionId(InTeamId.GetId());
		}
	}
}

FGenericTeamId ATunaSweeperEnemyAIController::GetGenericTeamId() const
{
	if (const APawn* ControlledPawn = GetPawn())
	{
		if (const UTunaSweeperFactionComponent* FactionComponent =
			ControlledPawn->FindComponentByClass<UTunaSweeperFactionComponent>())
		{
			return FGenericTeamId(FactionComponent->GetFactionId());
		}
	}
	return CachedTeamId;
}

ETeamAttitude::Type ATunaSweeperEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const UWorld* World = GetWorld();
	const UTunaSweeperFactionSubsystem* FactionSubsystem = World
		? World->GetSubsystem<UTunaSweeperFactionSubsystem>()
		: nullptr;
	if (!FactionSubsystem || !GetPawn())
	{
		return ETeamAttitude::Neutral;
	}

	switch (FactionSubsystem->GetFactionAttitude(GetPawn(), &Other))
	{
	case ETunaSweeperFactionAttitude::Friendly:
		return ETeamAttitude::Friendly;
	case ETunaSweeperFactionAttitude::Hostile:
		return ETeamAttitude::Hostile;
	default:
		return ETeamAttitude::Neutral;
	}
}

bool ATunaSweeperEnemyAIController::GetCombatDebugSnapshot(
	FTunaSweeperEnemyCombatDebugSnapshot& OutSnapshot) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	OutSnapshot = FTunaSweeperEnemyCombatDebugSnapshot();
	OutSnapshot.bIsCombatEngaged = bIsCombatEngaged;
	OutSnapshot.bHasDirectTargetSight = bHasDirectTargetSight;
	OutSnapshot.TrackingRange = ResolveTrackingRange();
	OutSnapshot.VisionAngleDegrees = FMath::Clamp(CombatVisionAngleDegrees, 0.0f, 360.0f);
	OutSnapshot.HearingRange = FMath::Max(0.0f, HearingRange);
	OutSnapshot.FacingDirection = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();

	const double CurrentTimeSeconds = GetWorldTimeSeconds(this);
	OutSnapshot.RecentEntryReasonRemainingSeconds = FMath::Max(
		0.0f,
		CombatDebugEntryReasonDisplaySeconds -
			static_cast<float>(CurrentTimeSeconds - CombatDebugEntryReasonTimeSeconds));
	if (OutSnapshot.RecentEntryReasonRemainingSeconds > 0.0f)
	{
		OutSnapshot.RecentEntryReason = CombatDebugEntryReason;
	}

	auto SetTimedState = [&OutSnapshot, CurrentTimeSeconds](const TCHAR* Label, double StartTime, double EndTime)
	{
		OutSnapshot.StateLabel = Label;
		OutSnapshot.MaxStateSeconds = FMath::Max(0.0f, static_cast<float>(EndTime - StartTime));
		OutSnapshot.RemainingStateSeconds = FMath::Clamp(
			static_cast<float>(EndTime - CurrentTimeSeconds),
			0.0f,
			OutSnapshot.MaxStateSeconds);
	};

	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Suspicious)
	{
		SetTimedState(TEXT("Suspicious"), AwarenessStateStartTimeSeconds, AwarenessStateEndTimeSeconds);
		return true;
	}
	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted)
	{
		SetTimedState(TEXT("Alerted"), AwarenessStateStartTimeSeconds, AwarenessStateEndTimeSeconds);
		return true;
	}
	if (!bIsCombatEngaged)
	{
		OutSnapshot.StateLabel = NonCombatState == ETunaSweeperNonCombatState::Wander
			? TEXT("Wander")
			: TEXT("Idle");
		return true;
	}
	if (const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn);
		Enemy && Enemy->UsesMeleeAttack())
	{
		OutSnapshot.StateLabel = bIsClosingDistance ? TEXT("Advance") : TEXT("Melee");
		return true;
	}

	const TCHAR* StateLabel = TEXT("Combat");
	switch (RangedCombatState)
	{
	case ETunaSweeperRangedCombatState::Aim: StateLabel = TEXT("Aim"); break;
	case ETunaSweeperRangedCombatState::Firing: StateLabel = TEXT("Firing"); break;
	case ETunaSweeperRangedCombatState::Recover: StateLabel = TEXT("Recover"); break;
	case ETunaSweeperRangedCombatState::Observe: StateLabel = TEXT("Observe"); break;
	case ETunaSweeperRangedCombatState::Reposition: StateLabel = TEXT("Reposition"); break;
	case ETunaSweeperRangedCombatState::Reload: StateLabel = TEXT("Reload"); break;
	case ETunaSweeperRangedCombatState::HitEvade: StateLabel = TEXT("Hit Evade"); break;
	case ETunaSweeperRangedCombatState::SeekLineOfFire: StateLabel = TEXT("Seek Line"); break;
	default: break;
	}
	SetTimedState(StateLabel, RangedCombatStateStartTimeSeconds, RangedCombatStateEndTimeSeconds);
	return true;
}

void ATunaSweeperEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
	EffectiveUpdateInterval = GetRandomizedValue(UpdateInterval, UpdateIntervalRandomOffset, 0.02f);
	GetWorldTimerManager().SetTimer(
		UpdateTimerHandle,
		this,
		&ATunaSweeperEnemyAIController::UpdateAttackTarget,
		EffectiveUpdateInterval,
		true,
		FMath::FRandRange(0.0f, EffectiveUpdateInterval));

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
	UnregisterFromSquad();
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
	InitializeFromControlledCharacter();
	StartNonCombatIdle();
	UpdateAttackTarget();
}

void ATunaSweeperEnemyAIController::OnUnPossess()
{
	UnregisterFromSquad();
	Super::OnUnPossess();
}

void ATunaSweeperEnemyAIController::InitializeFromControlledCharacter()
{
	ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	if (!EnemyCharacter)
	{
		return;
	}

	CombatProfile = EnemyCharacter->GetCombatProfile();
	if (UCharacterMovementComponent* MovementComponent = EnemyCharacter->GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = FMath::Max(0.0f, CombatProfile.MovementSpeed);
	}
	if (const UTunaSweeperFactionComponent* FactionComponent = EnemyCharacter->GetFactionComponent())
	{
		SetGenericTeamId(FGenericTeamId(FactionComponent->GetFactionId()));
	}
	PositionFiringBudget = FMath::RandRange(
		FMath::Max(1, FMath::Min(CombatProfile.PositionFiringBudgetMin, CombatProfile.PositionFiringBudgetMax)),
		FMath::Max(1, FMath::Max(CombatProfile.PositionFiringBudgetMin, CombatProfile.PositionFiringBudgetMax)));
}

void ATunaSweeperEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAwarenessState(DeltaSeconds);
	UpdateNonCombatState(DeltaSeconds);

	ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	AActor* TargetActor = CurrentTargetActor.Get();
	if (bIsCombatEngaged && EnemyCharacter && TargetActor)
	{
		const float DistanceToTarget = FVector::Dist2D(
			EnemyCharacter->GetActorLocation(),
			TargetActor->GetActorLocation());
		if (EnemyCharacter->UsesMeleeAttack())
		{
			if (bHasDirectTargetSight)
			{
				UpdateMeleeCombat(DistanceToTarget, EnemyCharacter, DeltaSeconds);
				MoveTowardCurrentTarget(DeltaSeconds);
			}
		}
		else
		{
			TickRangedCombat(DeltaSeconds, EnemyCharacter, TargetActor);
		}
	}
	DrawCombatDebug();
}

void ATunaSweeperEnemyAIController::UpdateAttackTarget()
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		ClearCombatTarget();
		return;
	}

	UTunaSweeperFactionSubsystem* FactionSubsystem = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
	AActor* TargetActor = CurrentTargetActor.Get();
	if (TargetActor)
	{
		const float Distance = FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
		if (IsUnavailableCombatTarget(TargetActor) ||
			!FactionSubsystem ||
			!FactionSubsystem->CanTargetActor(ControlledPawn, TargetActor) ||
			(bIsCombatEngaged && Distance > ResolveCombatDisengageRange()))
		{
			ClearCombatTarget();
			TargetActor = nullptr;
		}
		else
		{
			bHasDirectTargetSight = HasDirectSightTo(TargetActor);
			if (bHasDirectTargetSight)
			{
				LastKnownTargetLocation = TargetActor->GetActorLocation();
				TargetSightLostTimeSeconds = -1.0;
				ReportSquadContact(TargetActor, LastKnownTargetLocation);
			}
			else
			{
				const double CurrentTimeSeconds = GetWorldTimeSeconds(this);
				RefreshSquadState();
				const float FreshContactSeconds = FMath::Max(
					TargetSightLossGraceSeconds,
					EffectiveUpdateInterval * 2.5f);
				const bool bHasFreshSquadContact =
					IsPairedSquadMember() &&
					SquadState.SharedHostileTarget == TargetActor &&
					SquadState.bHasSharedLastKnownLocation &&
					SquadState.SharedContactAgeSeconds <= FreshContactSeconds;
				if (bHasFreshSquadContact)
				{
					LastKnownTargetLocation = SquadState.SharedLastKnownLocation;
				}
				if (TargetSightLostTimeSeconds < 0.0)
				{
					TargetSightLostTimeSeconds = CurrentTimeSeconds;
				}
				const double SightLostSeconds = CurrentTimeSeconds - TargetSightLostTimeSeconds;
				const ETunaSweeperLineOfFireResult LostSightLineOfFire = EvaluateLineOfFire(TargetActor);
				const bool bHasKnownTacticalBlock =
					LostSightLineOfFire == ETunaSweeperLineOfFireResult::BlockedByFriendly ||
					LostSightLineOfFire == ETunaSweeperLineOfFireResult::BlockedByIndestructible;
				if (IsPairedSquadMember() &&
					SquadState.Role == ETunaSweeperEnemySquadRole::Suppress &&
					bHasKnownTacticalBlock &&
					SightLostSeconds >= FMath::Max(0.05f, LineOfFireFailureSeconds))
				{
					RequestSquadLineOfFireRecovery();
				}
				const bool bIsSeekingKnownBlock =
					bHasKnownTacticalBlock &&
					(RangedCombatState == ETunaSweeperRangedCombatState::Aim ||
						RangedCombatState == ETunaSweeperRangedCombatState::SeekLineOfFire ||
						(RangedCombatState == ETunaSweeperRangedCombatState::Reposition &&
							bMoveRequestActive));
				const float EffectiveSightLossGraceSeconds =
					SquadState.bLineOfFireRecoveryRequested || bIsSeekingKnownBlock
					? FMath::Max(TargetSightLossGraceSeconds, UTunaSweeperEnemySquadSubsystem::RoleLeaseSeconds)
					: FMath::Max(0.0f, TargetSightLossGraceSeconds);
				if (!bHasFreshSquadContact && SightLostSeconds >= EffectiveSightLossGraceSeconds)
				{
					UnregisterFromSquad();
					bIsCombatEngaged = false;
					StartSuspicion(LastKnownTargetLocation, TEXT("Search: target lost"));
				}
			}
			if (AwarenessState == ETunaSweeperEnemyAwarenessState::Suspicious &&
				CanAcquireCombatTarget(TargetActor, Distance))
			{
				StartAlerted(TargetActor, TEXT("Combat: sight reacquired"));
			}
			return;
		}
	}

	AActor* BestTarget = FindBestHostileTarget();
	if (BestTarget)
	{
		StartAlerted(BestTarget, TEXT("Combat: hostile sighted"));
		return;
	}

	RefreshSquadState();
	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Unaware &&
		SquadState.bHasSharedLastKnownLocation &&
		SquadState.SharedContactAgeSeconds <= UTunaSweeperEnemySquadSubsystem::RoleLeaseSeconds)
	{
		StartSuspicion(SquadState.SharedLastKnownLocation, TEXT("Search: squad contact"));
	}
}

AActor* ATunaSweeperEnemyAIController::FindBestHostileTarget() const
{
	const APawn* ControlledPawn = GetPawn();
	const UWorld* World = GetWorld();
	const UTunaSweeperFactionSubsystem* FactionSubsystem = World
		? World->GetSubsystem<UTunaSweeperFactionSubsystem>()
		: nullptr;
	if (!ControlledPawn || !FactionSubsystem)
	{
		return nullptr;
	}

	TArray<AActor*> HostileActors;
	FactionSubsystem->GetActorsWithAttitude(
		ControlledPawn,
		ETunaSweeperFactionAttitude::Hostile,
		HostileActors);
	AActor* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (AActor* Candidate : HostileActors)
	{
		if (IsUnavailableCombatTarget(Candidate) || !FactionSubsystem->CanTargetActor(ControlledPawn, Candidate))
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared2D(
			ControlledPawn->GetActorLocation(),
			Candidate->GetActorLocation());
		if (DistanceSquared >= BestDistanceSquared ||
			!CanAcquireCombatTarget(Candidate, FMath::Sqrt(DistanceSquared)))
		{
			continue;
		}
		BestDistanceSquared = DistanceSquared;
		BestTarget = Candidate;
	}
	return BestTarget;
}

void ATunaSweeperEnemyAIController::NotifySuspicionAtLocation(const FVector& InSuspicionLocation)
{
	if (InSuspicionLocation.ContainsNaN() || bIsCombatEngaged ||
		AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted)
	{
		return;
	}
	StartSuspicion(InSuspicionLocation, TEXT("Search: external alert"));
}

void ATunaSweeperEnemyAIController::NotifyDamageTaken(AActor* SuspectedActor)
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return;
	}

	if (SuspectedActor)
	{
		if (UTunaSweeperFactionSubsystem* FactionSubsystem = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
			FactionSubsystem && !FactionSubsystem->CanApplyCombatEffect(SuspectedActor, ControlledPawn))
		{
			return;
		}
	}

	if (!bIsCombatEngaged && SuspectedActor)
	{
		StartAlerted(SuspectedActor, TEXT("Combat: damage taken"));
		return;
	}

	ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn);
	if (bIsCombatEngaged && EnemyCharacter && !EnemyCharacter->UsesMeleeAttack() &&
		RangedCombatState != ETunaSweeperRangedCombatState::Reload)
	{
		StartHitEvade(SuspectedActor);
	}
}

void ATunaSweeperEnemyAIController::UpdateAwarenessState(float DeltaSeconds)
{
	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Unaware ||
		AwarenessState == ETunaSweeperEnemyAwarenessState::Combat)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || DeltaSeconds <= 0.0f)
	{
		return;
	}
	const double CurrentTimeSeconds = GetWorldTimeSeconds(this);

	if (AwarenessState == ETunaSweeperEnemyAwarenessState::Alerted)
	{
		AActor* TargetActor = CurrentTargetActor.Get();
		if (!TargetActor)
		{
			ClearCombatTarget();
			return;
		}
		FaceTarget(DeltaSeconds, false);
		if (CurrentTimeSeconds >= AwarenessStateEndTimeSeconds)
		{
			EnterCombat();
		}
		return;
	}

	FVector DirectionToSuspicion = GetPlanarDirection(ControlledPawn->GetActorLocation(), SuspicionLocation);
	if (!DirectionToSuspicion.IsNearlyZero())
	{
		const float ElapsedSeconds = FMath::Max(
			0.0f,
			static_cast<float>(CurrentTimeSeconds - AwarenessStateStartTimeSeconds));
		const float SweepAngle = FMath::Sin(ElapsedSeconds * 4.0f) * FMath::Max(0.0f, SearchSweepHalfAngleDegrees);
		const FVector SearchDirection = DirectionToSuspicion.RotateAngleAxis(SweepAngle, FVector::UpVector);
		RotateTowardLocation(
			ControlledPawn->GetActorLocation() + SearchDirection,
			DeltaSeconds,
			0.55f);
	}
	if (CurrentTimeSeconds >= AwarenessStateEndTimeSeconds)
	{
		ClearCombatTarget();
	}
}

void ATunaSweeperEnemyAIController::StartSuspicion(
	const FVector& InSuspicionLocation,
	const TCHAR* EntryReason)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !GetWorld())
	{
		return;
	}

	const bool bEnteredFromUnaware = AwarenessState == ETunaSweeperEnemyAwarenessState::Unaware;
	AwarenessState = ETunaSweeperEnemyAwarenessState::Suspicious;
	AwarenessStateStartTimeSeconds = GetWorldTimeSeconds(this);
	AwarenessStateEndTimeSeconds = AwarenessStateStartTimeSeconds +
		GetRandomRangeValue(SuspicionSearchSeconds, 0.05f);
	SuspicionLocation = InSuspicionLocation;
	CurrentTargetActor.Reset();
	bHasDirectTargetSight = false;
	bIsClosingDistance = false;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	RecordCombatDebugEntryReason(EntryReason);

	if (bEnteredFromUnaware && !bAlertBubbleShownThisCycle)
	{
		if (ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn))
		{
			Enemy->ShowAlertSpeechBubble();
		}
		bAlertBubbleShownThisCycle = true;
	}
}

void ATunaSweeperEnemyAIController::StartAlerted(AActor* TargetActor, const TCHAR* EntryReason)
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World || !TargetActor)
	{
		return;
	}
	if (UTunaSweeperFactionSubsystem* FactionSubsystem = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
		!FactionSubsystem || !FactionSubsystem->CanTargetActor(ControlledPawn, TargetActor))
	{
		return;
	}

	AwarenessState = ETunaSweeperEnemyAwarenessState::Alerted;
	AwarenessStateStartTimeSeconds = World->GetTimeSeconds();
	AwarenessStateEndTimeSeconds = AwarenessStateStartTimeSeconds + FMath::Max(0.0f, CombatProfile.AlertSeconds);
	CurrentTargetActor = TargetActor;
	bHasDirectTargetSight = true;
	LastKnownTargetLocation = TargetActor->GetActorLocation();
	TargetSightLostTimeSeconds = -1.0;
	bIsCombatEngaged = false;
	bIsClosingDistance = false;
	StopMovement();
	SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	RecordCombatDebugEntryReason(EntryReason);

	if (!bAlertBubbleShownThisCycle)
	{
		if (ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(ControlledPawn))
		{
			Enemy->ShowAlertSpeechBubble();
		}
		bAlertBubbleShownThisCycle = true;
	}
	RegisterWithSquad();
	ReportSquadContact(TargetActor, TargetActor->GetActorLocation());
}

void ATunaSweeperEnemyAIController::EnterCombat()
{
	ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	AActor* TargetActor = CurrentTargetActor.Get();
	if (!EnemyCharacter || !TargetActor)
	{
		ClearCombatTarget();
		return;
	}

	AwarenessState = ETunaSweeperEnemyAwarenessState::Combat;
	bIsCombatEngaged = true;
	SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	RegisterWithSquad();
	RefreshSquadState();
	ReportSquadContact(TargetActor, TargetActor->GetActorLocation());
	RecordCombatDebugEntryReason(TEXT("Combat: target acquired"));

	if (EnemyCharacter->UsesMeleeAttack())
	{
		const float Distance = FVector::Dist2D(EnemyCharacter->GetActorLocation(), TargetActor->GetActorLocation());
		bIsClosingDistance = Distance > ResolveApproachStartRange();
		return;
	}

	FiringsAtCurrentPosition = 0;
	PositionFiringBudget = FMath::RandRange(
		FMath::Max(1, FMath::Min(CombatProfile.PositionFiringBudgetMin, CombatProfile.PositionFiringBudgetMax)),
		FMath::Max(1, FMath::Max(CombatProfile.PositionFiringBudgetMin, CombatProfile.PositionFiringBudgetMax)));
	bOpeningFiring = true;
	if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Reposition)
	{
		StartObserve(0.2f);
	}
	else
	{
		StartAim();
	}
}

void ATunaSweeperEnemyAIController::ClearCombatTarget()
{
	if (ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(GetPawn()))
	{
		Enemy->HideAlertSpeechBubble();
		if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = FMath::Max(0.0f, CombatProfile.MovementSpeed);
		}
	}
	UnregisterFromSquad();
	CurrentTargetActor.Reset();
	bIsCombatEngaged = false;
	bIsClosingDistance = false;
	bHasDirectTargetSight = false;
	TargetSightLostTimeSeconds = -1.0;
	LastKnownTargetLocation = FVector::ZeroVector;
	bAlertBubbleShownThisCycle = false;
	bOpeningFiring = true;
	bMoveRequestActive = false;
	bSquadMoverSettling = false;
	bPendingReloadSafeMove = false;
	AwarenessState = ETunaSweeperEnemyAwarenessState::Unaware;
	RangedCombatState = ETunaSweeperRangedCombatState::Idle;
	RangedMoveKind = ETunaSweeperRangedMoveKind::None;
	CrossRepositionWaypoints.Reset();
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	StartNonCombatIdle();
}

void ATunaSweeperEnemyAIController::UpdateNonCombatState(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || AwarenessState != ETunaSweeperEnemyAwarenessState::Unaware ||
		bIsCombatEngaged || CurrentTargetActor.IsValid())
	{
		return;
	}
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const double CurrentTimeSeconds = GetWorldTimeSeconds(this);
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

	RotateTowardLocation(
		ControlledPawn->GetActorLocation() + NonCombatFacingDirection,
		DeltaSeconds,
		0.75f);
	float InputScale = 1.0f;
	if (const ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		if (const UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
		{
			InputScale = FMath::Clamp(WanderMoveSpeed / FMath::Max(1.0f, Movement->MaxWalkSpeed), 0.0f, 1.0f);
		}
	}
	ControlledPawn->AddMovementInput(NonCombatFacingDirection, InputScale, true);
}

void ATunaSweeperEnemyAIController::StartNonCombatIdle()
{
	NonCombatState = ETunaSweeperNonCombatState::Idle;
	NonCombatStateEndTimeSeconds = GetWorldTimeSeconds(this) + GetRandomRangeValue(IdleSeconds, 0.05f);
	StopMovement();
}

void ATunaSweeperEnemyAIController::StartNonCombatWander()
{
	NonCombatState = ETunaSweeperNonCombatState::Wander;
	NonCombatFacingDirection = GetRandomPlanarDirection();
	NonCombatStateEndTimeSeconds = GetWorldTimeSeconds(this) + GetRandomRangeValue(WanderSeconds, 0.05f);
}

void ATunaSweeperEnemyAIController::UpdateMeleeCombat(
	float DistanceToTarget,
	ATunaSweeperEnemyCharacter* EnemyCharacter,
	float DeltaSeconds)
{
	if (!EnemyCharacter || !CurrentTargetActor.IsValid())
	{
		return;
	}
	RotateTowardLocation(CurrentTargetActor->GetActorLocation(), DeltaSeconds);
	if (bIsClosingDistance)
	{
		if (DistanceToTarget <= ResolveApproachStopRange())
		{
			bIsClosingDistance = false;
			StopMovement();
		}
	}
	else if (DistanceToTarget > ResolveApproachStartRange())
	{
		bIsClosingDistance = true;
	}

	const double CurrentTimeSeconds = GetWorldTimeSeconds(this);
	if (!bIsClosingDistance && DistanceToTarget <= ResolveAttackRange() &&
		IsFacingCurrentTarget() &&
		CurrentTimeSeconds - LastMeleeAttackTimeSeconds >= ResolveAttackCooldownSeconds() &&
		EnemyCharacter->AttackTarget(CurrentTargetActor.Get()))
	{
		LastMeleeAttackTimeSeconds = CurrentTimeSeconds;
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
		return;
	}
	const FVector Direction = GetPlanarDirection(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	if (!Direction.IsNearlyZero())
	{
		ControlledPawn->AddMovementInput(Direction, 1.0f, true);
	}
}

void ATunaSweeperEnemyAIController::TickRangedCombat(
	float DeltaSeconds,
	ATunaSweeperEnemyCharacter* EnemyCharacter,
	AActor* TargetActor)
{
	if (!EnemyCharacter || !TargetActor || DeltaSeconds <= 0.0f)
	{
		return;
	}
	RefreshSquadState();
	const double CurrentTimeSeconds = GetWorldTimeSeconds(this);
	const float FreshContactSeconds = FMath::Max(
		TargetSightLossGraceSeconds,
		EffectiveUpdateInterval * 2.5f);
	const bool bHasFreshSquadContact =
		IsPairedSquadMember() &&
		SquadState.SharedHostileTarget == TargetActor &&
		SquadState.bHasSharedLastKnownLocation &&
		SquadState.SharedContactAgeSeconds <= FreshContactSeconds;
	if (!bHasDirectTargetSight && bHasFreshSquadContact)
	{
		LastKnownTargetLocation = SquadState.SharedLastKnownLocation;
	}
	const bool bCanContinueSquadMoveWithoutSight =
		IsPairedSquadMember() &&
		SquadState.Role == ETunaSweeperEnemySquadRole::Reposition &&
		(SquadState.bLineOfFireRecoveryRequested ||
			(SquadState.bSuppressionStarted && bHasFreshSquadContact));
	const ETunaSweeperLineOfFireResult LostSightLineOfFire = !bHasDirectTargetSight
		? EvaluateLineOfFire(TargetActor)
		: ETunaSweeperLineOfFireResult::Clear;
	const bool bCanSeekKnownBlockWithoutSight =
		(LostSightLineOfFire == ETunaSweeperLineOfFireResult::BlockedByFriendly ||
			LostSightLineOfFire == ETunaSweeperLineOfFireResult::BlockedByIndestructible) &&
		(RangedCombatState == ETunaSweeperRangedCombatState::Aim ||
			RangedCombatState == ETunaSweeperRangedCombatState::SeekLineOfFire ||
			(RangedCombatState == ETunaSweeperRangedCombatState::Reposition &&
				bMoveRequestActive));
	if (!bHasDirectTargetSight &&
		!bCanContinueSquadMoveWithoutSight &&
		!bCanSeekKnownBlockWithoutSight &&
		RangedCombatState != ETunaSweeperRangedCombatState::Reload &&
		RangedCombatState != ETunaSweeperRangedCombatState::Recover &&
		RangedCombatState != ETunaSweeperRangedCombatState::HitEvade)
	{
		if (RangedCombatState == ETunaSweeperRangedCombatState::Firing)
		{
			FinishFiring(EnemyCharacter, LastShotTimeSeconds);
		}
		else
		{
			bMoveRequestActive = false;
			ActiveMoveRequestId = FAIRequestID::InvalidRequest;
			ActiveMoveCycleId = 0;
			StopMovement();
			StartObserve(0.15f);
		}
		return;
	}
	const FVector TacticalTargetLocation = bHasDirectTargetSight
		? TargetActor->GetActorLocation()
		: LastKnownTargetLocation;
	const float DistanceToTarget = FVector::Dist2D(
		EnemyCharacter->GetActorLocation(),
		TacticalTargetLocation);
	if (RangedMoveKind == ETunaSweeperRangedMoveKind::CrossReposition &&
		bMoveRequestActive &&
		!DoesMoveSegmentClearCurrentTarget(RangedMoveGoal, 120.0f))
	{
		bMoveRequestActive = false;
		ActiveMoveRequestId = FAIRequestID::InvalidRequest;
		ActiveMoveCycleId = 0;
		StopMovement();
		CrossRepositionWaypoints.Reset();
		CrossWaypointIndex = INDEX_NONE;
		StartReposition(false, false, false);
		return;
	}
	if (DistanceToTarget > ResolveAttackRange() &&
		RangedCombatState != ETunaSweeperRangedCombatState::Firing &&
		RangedCombatState != ETunaSweeperRangedCombatState::Recover &&
		RangedCombatState != ETunaSweeperRangedCombatState::Reload &&
		RangedCombatState != ETunaSweeperRangedCombatState::HitEvade &&
		RangedCombatState != ETunaSweeperRangedCombatState::Reposition &&
		RangedCombatState != ETunaSweeperRangedCombatState::SeekLineOfFire)
	{
		if (IsPairedSquadMember())
		{
			if (SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
			{
				RequestSquadLineOfFireRecovery();
				StartObserve(0.15f);
			}
			else if (CanSquadMoverStartReposition())
			{
				StartReposition(false, false, true);
			}
			else
			{
				StartObserve(0.15f);
			}
		}
		else
		{
			StartReposition(false, false, true);
		}
		return;
	}

	if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Reposition &&
		RangedCombatState == ETunaSweeperRangedCombatState::Firing)
	{
		FinishFiring(EnemyCharacter, LastShotTimeSeconds);
	}

	if (RangedCombatState != ETunaSweeperRangedCombatState::Firing &&
		RangedCombatState != ETunaSweeperRangedCombatState::Recover &&
		RangedCombatState != ETunaSweeperRangedCombatState::Reload &&
		RangedCombatState != ETunaSweeperRangedCombatState::HitEvade &&
		RangedCombatState != ETunaSweeperRangedCombatState::Reposition &&
		RangedCombatState != ETunaSweeperRangedCombatState::SeekLineOfFire &&
		DistanceToTarget < FMath::Max(0.0f, CombatProfile.DangerRange))
	{
		if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
		{
			RequestSquadLineOfFireRecovery();
			StartObserve(0.15f);
			return;
		}
		if (!IsPairedSquadMember() || CanSquadMoverStartReposition())
		{
			StartReposition(false, false);
		}
		else
		{
			StartObserve(0.15f);
		}
		return;
	}

	switch (RangedCombatState)
	{
	case ETunaSweeperRangedCombatState::Idle:
		if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Reposition)
		{
			if (CanSquadMoverStartReposition())
			{
				StartReposition(SquadState.bLineOfFireRecoveryRequested);
			}
			else
			{
				StartObserve(0.15f);
			}
		}
		else if (CanSquadMemberStartFiring())
		{
			StartAim();
		}
		break;

	case ETunaSweeperRangedCombatState::Aim:
	{
		FaceTarget(DeltaSeconds, false);
		const ETunaSweeperLineOfFireResult LineOfFire = EvaluateLineOfFire(TargetActor);
		if (LineOfFire == ETunaSweeperLineOfFireResult::BlockedByFriendly ||
			LineOfFire == ETunaSweeperLineOfFireResult::BlockedByIndestructible)
		{
			if (LineOfFireBlockedStartTimeSeconds < 0.0)
			{
				LineOfFireBlockedStartTimeSeconds = CurrentTimeSeconds;
			}
			if (CurrentTimeSeconds - LineOfFireBlockedStartTimeSeconds >=
				FMath::Max(0.05f, LineOfFireFailureSeconds))
			{
				if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
				{
					RequestSquadLineOfFireRecovery();
					StartObserve(0.15f);
				}
				else if (!IsPairedSquadMember() || CanSquadMoverStartReposition())
				{
					StartReposition(true, false);
				}
				else
				{
					StartObserve(0.15f);
				}
			}
			break;
		}
		LineOfFireBlockedStartTimeSeconds = -1.0;
		if (CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds)
		{
			if (bHasDirectTargetSight && CanSquadMemberStartFiring() && IsFacingCurrentTarget())
			{
				StartFiring(EnemyCharacter);
			}
			else if (!bHasDirectTargetSight || !CanSquadMemberStartFiring())
			{
				StartObserve(0.2f);
			}
		}
		break;
	}

	case ETunaSweeperRangedCombatState::Firing:
		FaceTarget(DeltaSeconds, false);
		TickFiring(EnemyCharacter, TargetActor, CurrentTimeSeconds);
		break;

	case ETunaSweeperRangedCombatState::Recover:
		if (CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds)
		{
			StartObserve();
		}
		break;

	case ETunaSweeperRangedCombatState::Observe:
		FaceTarget(DeltaSeconds, true);
		if (CurrentTimeSeconds < RangedCombatStateEndTimeSeconds)
		{
			break;
		}
		if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Reposition)
		{
			if (CanSquadMoverStartReposition())
			{
				StartReposition(SquadState.bLineOfFireRecoveryRequested);
			}
			else
			{
				StartObserve(0.15f);
			}
		}
		else if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
		{
			if (!SquadState.bSuppressionStarted && CanSquadMemberStartFiring())
			{
				StartAim();
			}
			else
			{
				StartObserve(0.15f);
			}
		}
		else if (FiringsAtCurrentPosition < PositionFiringBudget && CanSquadMemberStartFiring())
		{
			StartAim();
		}
		else
		{
			StartReposition();
		}
		break;

	case ETunaSweeperRangedCombatState::Reload:
		TickReload(EnemyCharacter, CurrentTimeSeconds);
		break;

	case ETunaSweeperRangedCombatState::Reposition:
	case ETunaSweeperRangedCombatState::SeekLineOfFire:
	case ETunaSweeperRangedCombatState::HitEvade:
		FaceTarget(DeltaSeconds, true);
		if (bMoveRequestActive && CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds)
		{
			bMoveRequestActive = false;
			StopMovement();
			HandleMoveFinished(false);
		}
		else if (!bMoveRequestActive && bSquadMoverSettling &&
			CurrentTimeSeconds >= RepositionSettleEndTimeSeconds)
		{
			RefreshSquadState();
			if (SettleCycleId > 0 &&
				(!IsPairedSquadMember() || SquadState.CycleId != SettleCycleId))
			{
				bSquadMoverSettling = false;
				SettleCycleId = 0;
				StartObserve(0.15f);
				break;
			}
			if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Reposition &&
				SquadState.bSuppressionStarted && !SquadState.bSuppressionFinished)
			{
				// The mover is settled, but role ownership must not rotate until the
				// suppressor's final shot establishes the squad silence gate.
				break;
			}
			bSquadMoverSettling = false;
			const int32 CompletedSettleCycleId = SettleCycleId;
			CompleteSquadMoverRole(CompletedSettleCycleId);
			SettleCycleId = 0;
			RefreshSquadState();
			FiringsAtCurrentPosition = 0;
			PositionFiringBudget = FMath::RandRange(
				FMath::Max(1, FMath::Min(CombatProfile.PositionFiringBudgetMin, CombatProfile.PositionFiringBudgetMax)),
				FMath::Max(1, FMath::Max(CombatProfile.PositionFiringBudgetMin, CombatProfile.PositionFiringBudgetMax)));
			if (CanSquadMemberStartFiring())
			{
				StartAim(true);
			}
			else
			{
				StartObserve(0.15f);
			}
		}
		break;
	}
}

void ATunaSweeperEnemyAIController::EnterRangedState(
	ETunaSweeperRangedCombatState NewState,
	float DurationSeconds)
{
	RangedCombatState = NewState;
	RangedCombatStateStartTimeSeconds = GetWorldTimeSeconds(this);
	RangedCombatStateEndTimeSeconds = RangedCombatStateStartTimeSeconds + FMath::Max(0.0f, DurationSeconds);
}

void ATunaSweeperEnemyAIController::StartAim(bool bFromReposition)
{
	if (!CurrentTargetActor.IsValid())
	{
		return;
	}
	bMoveRequestActive = false;
	StopMovement();
	bSquadMoverSettling = false;
	LineOfFireBlockedStartTimeSeconds = -1.0;
	const float AimSeconds = GetRandomRangeValue(
		FVector2D(CombatProfile.AimSecondsMin, CombatProfile.AimSecondsMax),
		0.0f);
	EnterRangedState(ETunaSweeperRangedCombatState::Aim, AimSeconds);
}

void ATunaSweeperEnemyAIController::StartFiring(ATunaSweeperEnemyCharacter* EnemyCharacter)
{
	if (!EnemyCharacter || !CanSquadMemberStartFiring())
	{
		StartObserve(0.15f);
		return;
	}
	if (AActor* TargetActor = CurrentTargetActor.Get();
		!TargetActor || FVector::Dist2D(EnemyCharacter->GetActorLocation(), TargetActor->GetActorLocation()) >
			ResolveAttackRange())
	{
		if (IsPairedSquadMember())
		{
			if (SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
			{
				RequestSquadLineOfFireRecovery();
				StartObserve(0.15f);
			}
			else if (CanSquadMoverStartReposition())
			{
				StartReposition(false, false, true);
			}
			else
			{
				StartObserve(0.15f);
			}
		}
		else
		{
			StartReposition(false, false, true);
		}
		return;
	}
	const FTunaSweeperEnemyWeaponRuntimeStatus WeaponStatus = EnemyCharacter->GetEnemyWeaponRuntimeStatus();
	if (WeaponStatus.LoadedAmmo <= 0)
	{
		if (WeaponStatus.ReserveAmmo > 0)
		{
			StartReload(EnemyCharacter);
		}
		else
		{
			UnregisterFromSquad();
			StartObserve(0.5f);
		}
		return;
	}

	const int32 DesiredShots = WeaponStatus.FireMode == ETunaSweeperWeaponFireMode::SemiAutomatic
		? 1
		: bOpeningFiring
			? FMath::Max(1, CombatProfile.OpeningFiringShotCount)
			: FMath::Max(1, CombatProfile.FiringShotCount);
	ShotsPlannedThisFiring = FMath::Min(DesiredShots, WeaponStatus.LoadedAmmo);
	ShotsFiredThisFiring = 0;
	bSquadLastShotReported = false;
	NextShotTimeSeconds = GetWorldTimeSeconds(this);
	EnterRangedState(ETunaSweeperRangedCombatState::Firing, 10.0f);
}

void ATunaSweeperEnemyAIController::TickFiring(
	ATunaSweeperEnemyCharacter* EnemyCharacter,
	AActor* TargetActor,
	double CurrentTimeSeconds)
{
	if (!EnemyCharacter || !TargetActor || CurrentTimeSeconds < NextShotTimeSeconds)
	{
		return;
	}
	if (CurrentTimeSeconds >= RangedCombatStateEndTimeSeconds)
	{
		FinishFiring(EnemyCharacter, LastShotTimeSeconds);
		return;
	}
	if (FVector::Dist2D(EnemyCharacter->GetActorLocation(), TargetActor->GetActorLocation()) > ResolveAttackRange())
	{
		FinishFiring(EnemyCharacter, LastShotTimeSeconds);
		return;
	}
	if (!bHasDirectTargetSight || !CanSquadMemberStartFiring())
	{
		FinishFiring(EnemyCharacter, LastShotTimeSeconds);
		return;
	}
	if (!IsFacingCurrentTarget())
	{
		NextShotTimeSeconds = CurrentTimeSeconds + 0.02;
		return;
	}

	const ETunaSweeperLineOfFireResult LineOfFire = EvaluateLineOfFire(TargetActor);
	if (LineOfFire == ETunaSweeperLineOfFireResult::BlockedByFriendly ||
		LineOfFire == ETunaSweeperLineOfFireResult::BlockedByIndestructible)
	{
		FinishFiring(EnemyCharacter, LastShotTimeSeconds);
		if (ShotsFiredThisFiring <= 0 &&
			RangedCombatState != ETunaSweeperRangedCombatState::Reload)
		{
			if (IsPairedSquadMember())
			{
				if (SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
				{
					RequestSquadLineOfFireRecovery();
					StartObserve(0.15f);
				}
				else if (CanSquadMoverStartReposition())
				{
					StartReposition(true, false);
				}
			}
			else
			{
				StartReposition(true, false);
			}
		}
		return;
	}

	const ETunaSweeperEnemyFireResult FireResult = EnemyCharacter->TryFireProjectileAt(TargetActor);
	switch (FireResult)
	{
	case ETunaSweeperEnemyFireResult::Fired:
	{
		const bool bFirstShot = ShotsFiredThisFiring == 0;
		++ShotsFiredThisFiring;
		LastShotTimeSeconds = CurrentTimeSeconds;
		const FTunaSweeperEnemyWeaponRuntimeStatus StatusAfterShot = EnemyCharacter->GetEnemyWeaponRuntimeStatus();
		const bool bLastShot = ShotsFiredThisFiring >= ShotsPlannedThisFiring || StatusAfterShot.LoadedAmmo <= 0;
		ReportSquadShot(bFirstShot, bLastShot);
		bSquadLastShotReported = bLastShot;
		if (bLastShot)
		{
			FinishFiring(EnemyCharacter, CurrentTimeSeconds);
			return;
		}
		NextShotTimeSeconds = CurrentTimeSeconds + GetRandomRangeValue(
			FVector2D(CombatProfile.ShotIntervalSecondsMin, CombatProfile.ShotIntervalSecondsMax),
			0.01f);
		break;
	}
	case ETunaSweeperEnemyFireResult::MagazineEmpty:
	case ETunaSweeperEnemyFireResult::Reloading:
		StartReload(EnemyCharacter);
		break;
	case ETunaSweeperEnemyFireResult::Cooldown:
		NextShotTimeSeconds = CurrentTimeSeconds + 0.02;
		break;
	case ETunaSweeperEnemyFireResult::OutOfAmmo:
		UnregisterFromSquad();
		FinishFiring(EnemyCharacter, LastShotTimeSeconds);
		break;
	case ETunaSweeperEnemyFireResult::FriendlyTarget:
	case ETunaSweeperEnemyFireResult::Blocked:
	default:
		FinishFiring(EnemyCharacter, LastShotTimeSeconds);
		break;
	}
}

void ATunaSweeperEnemyAIController::FinishFiring(
	ATunaSweeperEnemyCharacter* EnemyCharacter,
	double InLastShotTimeSeconds)
{
	if (!EnemyCharacter)
	{
		return;
	}
	if (ShotsFiredThisFiring > 0 && !bSquadLastShotReported)
	{
		ReportSquadShot(false, true);
		bSquadLastShotReported = true;
	}
	if (ShotsFiredThisFiring > 0)
	{
		++FiringsAtCurrentPosition;
		bOpeningFiring = false;
		LastShotTimeSeconds = InLastShotTimeSeconds;
		NextAllowedFireTimeSeconds = FMath::Max(
			NextAllowedFireTimeSeconds,
			InLastShotTimeSeconds + CombatProfile.RecoverSecondsMin + CombatProfile.ObserveSecondsMin);
	}

	const FTunaSweeperEnemyWeaponRuntimeStatus WeaponStatus = EnemyCharacter->GetEnemyWeaponRuntimeStatus();
	if (WeaponStatus.LoadedAmmo <= 0 && WeaponStatus.ReserveAmmo > 0)
	{
		StartReload(EnemyCharacter);
		return;
	}
	if (WeaponStatus.LoadedAmmo <= 0 && WeaponStatus.ReserveAmmo <= 0)
	{
		UnregisterFromSquad();
	}
	StartRecover();
}

void ATunaSweeperEnemyAIController::StartRecover()
{
	bMoveRequestActive = false;
	StopMovement();
	const float RecoverSeconds = GetRandomRangeValue(
		FVector2D(CombatProfile.RecoverSecondsMin, CombatProfile.RecoverSecondsMax),
		0.0f);
	EnterRangedState(ETunaSweeperRangedCombatState::Recover, RecoverSeconds);
}

void ATunaSweeperEnemyAIController::StartObserve(float DurationOverride)
{
	bMoveRequestActive = false;
	StopMovement();
	const float ObserveSeconds = DurationOverride >= 0.0f
		? DurationOverride
		: GetRandomRangeValue(
			FVector2D(CombatProfile.ObserveSecondsMin, CombatProfile.ObserveSecondsMax),
			0.0f);
	EnterRangedState(ETunaSweeperRangedCombatState::Observe, ObserveSeconds);
}

void ATunaSweeperEnemyAIController::StartReload(ATunaSweeperEnemyCharacter* EnemyCharacter)
{
	if (!EnemyCharacter)
	{
		return;
	}
	if (RangedCombatState == ETunaSweeperRangedCombatState::Firing &&
		ShotsFiredThisFiring > 0 && !bSquadLastShotReported)
	{
		ReportSquadShot(false, true);
		bSquadLastShotReported = true;
	}
	// Enter Reload before yielding a squad lease. Yield broadcasts synchronously;
	// this prevents the role-change callback from recursively finishing the burst.
	EnterRangedState(ETunaSweeperRangedCombatState::Reload, 0.0f);

	if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
	{
		if (UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
			GetWorld()->GetSubsystem<UTunaSweeperEnemySquadSubsystem>())
		{
			SquadSubsystem->YieldSuppression(GetPawn(), SquadState.CycleId);
		}
		RefreshSquadState();
	}

	bReloadWasStarted = EnemyCharacter->StartEnemyReload();
	const FTunaSweeperEnemyWeaponRuntimeStatus Status = EnemyCharacter->GetEnemyWeaponRuntimeStatus();
	if (!bReloadWasStarted && !Status.bIsReloading)
	{
		if (Status.ReserveAmmo <= 0)
		{
			UnregisterFromSquad();
		}
		StartObserve(0.5f);
		return;
	}

	EnterRangedState(
		ETunaSweeperRangedCombatState::Reload,
		FMath::Max(0.15f, Status.ReloadSeconds) + CombatProfile.ReloadReadySecondsMax + 0.2f);
	ReloadReadyEndTimeSeconds = 0.0;
	bSafeReloadMoveUsed = false;
	bPendingReloadSafeMove = false;
	bMoveRequestActive = false;
	StopMovement();

	AActor* TargetActor = CurrentTargetActor.Get();
	const float TargetDistance = TargetActor
		? FVector::Dist2D(EnemyCharacter->GetActorLocation(), TargetActor->GetActorLocation())
		: TNumericLimits<float>::Max();
	const bool bDangerClose = TargetDistance < FMath::Max(0.0f, CombatProfile.DangerRange);
	const bool bPaired = IsPairedSquadMember();
	const bool bSquadSafeMove = bPaired && SquadState.Role == ETunaSweeperEnemySquadRole::Reposition;
	if (bSquadSafeMove || (!bPaired && bDangerClose))
	{
		FVector SafeGoal;
		if (BuildNormalRepositionGoal(false, bDangerClose, false, SafeGoal))
		{
			RangedMoveGoal = SafeGoal;
			RangedMoveKind = bDangerClose ? ETunaSweeperRangedMoveKind::Retreat : ETunaSweeperRangedMoveKind::Orbit;
			if (bSquadSafeMove)
			{
				bPendingReloadSafeMove = true;
			}
			else
			{
				bSafeReloadMoveUsed = RequestCurrentMoveGoal();
				if (UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement())
				{
					Movement->MaxWalkSpeed = CombatProfile.MovementSpeed *
						FMath::Clamp(SoloDangerReloadMoveSpeedScale, 0.1f, 1.0f);
					bReloadMovementSpeedReduced = true;
				}
			}
		}
	}
}

void ATunaSweeperEnemyAIController::TickReload(
	ATunaSweeperEnemyCharacter* EnemyCharacter,
	double CurrentTimeSeconds)
{
	if (!EnemyCharacter)
	{
		return;
	}
	if (bPendingReloadSafeMove)
	{
		RefreshSquadState();
		if (CanSquadMoverStartReposition())
		{
			bPendingReloadSafeMove = false;
			bSafeReloadMoveUsed = RequestCurrentMoveGoal();
		}
	}
	const FTunaSweeperEnemyWeaponRuntimeStatus Status = EnemyCharacter->GetEnemyWeaponRuntimeStatus();
	if (Status.bIsReloading)
	{
		return;
	}
	if (bReloadMovementSpeedReduced)
	{
		if (UCharacterMovementComponent* Movement = EnemyCharacter->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = FMath::Max(0.0f, CombatProfile.MovementSpeed);
		}
		bReloadMovementSpeedReduced = false;
	}
	if (ReloadReadyEndTimeSeconds <= 0.0)
	{
		bPendingReloadSafeMove = false;
		ReloadReadyEndTimeSeconds = CurrentTimeSeconds + GetRandomRangeValue(
			FVector2D(CombatProfile.ReloadReadySecondsMin, CombatProfile.ReloadReadySecondsMax),
			0.0f);
		StopMovement();
		return;
	}
	if (CurrentTimeSeconds >= ReloadReadyEndTimeSeconds)
	{
		StartObserve(0.1f);
	}
}

void ATunaSweeperEnemyAIController::StartHitEvade(AActor* ThreatActor)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !CurrentTargetActor.IsValid())
	{
		return;
	}
	const double CurrentTimeSeconds = GetWorldTimeSeconds(this);
	if (CurrentTimeSeconds - LastHitEvadeTimeSeconds < FMath::Max(0.0f, HitEvadeCooldownSeconds))
	{
		return;
	}
	if (IsPairedSquadMember() && SquadState.Role == ETunaSweeperEnemySquadRole::Suppress)
	{
		if (UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
			GetWorld()->GetSubsystem<UTunaSweeperEnemySquadSubsystem>())
		{
			SquadSubsystem->YieldSuppression(GetPawn(), SquadState.CycleId);
		}
		RefreshSquadState();
		if (IsPairedSquadMember() && SquadState.Role != ETunaSweeperEnemySquadRole::Reposition)
		{
			return;
		}
	}
	LastHitEvadeTimeSeconds = CurrentTimeSeconds;
	if (RangedCombatState == ETunaSweeperRangedCombatState::Firing && ShotsFiredThisFiring > 0 &&
		!bSquadLastShotReported)
	{
		ReportSquadShot(false, true);
		bSquadLastShotReported = true;
		++FiringsAtCurrentPosition;
		bOpeningFiring = false;
		NextAllowedFireTimeSeconds = FMath::Max(
			NextAllowedFireTimeSeconds,
			LastShotTimeSeconds + CombatProfile.RecoverSecondsMin + CombatProfile.ObserveSecondsMin);
	}

	const FVector ThreatLocation = ThreatActor
		? ThreatActor->GetActorLocation()
		: CurrentTargetActor->GetActorLocation();
	const FVector AwayDirection = GetPlanarDirection(ThreatLocation, ControlledPawn->GetActorLocation());
	const float SideSign = FMath::RandBool() ? 1.0f : -1.0f;
	const FVector SideDirection = FVector::CrossProduct(FVector::UpVector, AwayDirection) * SideSign;
	const FVector EvadeDirection = (AwayDirection * 0.55f + SideDirection).GetSafeNormal();
	const float EvadeDistance = GetRandomRangeValue(HitEvadeDistance, 150.0f);
	FVector ProjectedGoal;
	if (EvadeDirection.IsNearlyZero() || !ProjectAndValidateMoveGoal(
		ControlledPawn->GetActorLocation() + EvadeDirection * EvadeDistance,
		ProjectedGoal))
	{
		StartObserve(0.25f);
		return;
	}

	bMoveRequestActive = false;
	StopMovement();
	RangedMoveGoal = ProjectedGoal;
	RangedMoveKind = ETunaSweeperRangedMoveKind::HitEvade;
	CrossRepositionWaypoints.Reset();
	RepositionRetryCount = 1;
	EnterRangedState(ETunaSweeperRangedCombatState::HitEvade, 2.0f);
	if (!RequestCurrentMoveGoal())
	{
		StartObserve(0.25f);
	}
}

void ATunaSweeperEnemyAIController::StartReposition(
	bool bSeekLineOfFire,
	bool bSafeReloadMove,
	bool bApproachTarget)
{
	APawn* ControlledPawn = GetPawn();
	AActor* TargetActor = CurrentTargetActor.Get();
	if (!ControlledPawn || !TargetActor)
	{
		return;
	}
	const FVector TacticalTargetLocation = bHasDirectTargetSight
		? TargetActor->GetActorLocation()
		: LastKnownTargetLocation;
	const float DistanceToTarget = FVector::Dist2D(
		ControlledPawn->GetActorLocation(),
		TacticalTargetLocation);
	if (IsPairedSquadMember())
	{
		const bool bOwnsMovementLease = SquadState.Role == ETunaSweeperEnemySquadRole::Reposition;
		const bool bMoverGateOpen = CanSquadMoverStartReposition();
		if (!bOwnsMovementLease || !bMoverGateOpen)
		{
			StartObserve(0.15f);
			return;
		}
	}
	bMoveRequestActive = false;
	StopMovement();
	bSquadMoverSettling = false;
	CrossRepositionWaypoints.Reset();
	CrossWaypointIndex = INDEX_NONE;
	RepositionRetryCount = 0;
	RepositionSideSign = FMath::RandBool() ? 1.0f : -1.0f;

	const bool bRetreat = DistanceToTarget < FMath::Max(0.0f, CombatProfile.DangerRange);
	const bool bCanUseCross = !bSeekLineOfFire && !bRetreat && !bSafeReloadMove && !bApproachTarget &&
		CombatProfile.Role == ETunaSweeperEnemyCombatRole::Flanker &&
		(!IsPairedSquadMember() || SquadState.Role == ETunaSweeperEnemySquadRole::Reposition) &&
		GetWorldTimeSeconds(this) - LastCrossRepositionTimeSeconds >=
			FMath::Max(0.0f, CombatProfile.CrossRepositionCooldownSeconds) &&
		FMath::FRand() <= FMath::Clamp(CombatProfile.CrossRepositionChance, 0.0f, 1.0f);

	if (bCanUseCross && BuildCrossRepositionPath(CrossRepositionWaypoints) &&
		CrossRepositionWaypoints.Num() > 0)
	{
		LastCrossRepositionTimeSeconds = GetWorldTimeSeconds(this);
		CrossWaypointIndex = 0;
		RangedMoveGoal = CrossRepositionWaypoints[0];
		RangedMoveKind = ETunaSweeperRangedMoveKind::CrossReposition;
	}
	else
	{
		if (!BuildNormalRepositionGoal(
			bSeekLineOfFire,
			bRetreat || bSafeReloadMove,
			bApproachTarget,
			RangedMoveGoal))
		{
			StartObserve(0.35f);
			return;
		}
		RangedMoveKind = bApproachTarget
			? ETunaSweeperRangedMoveKind::Approach
			: bSeekLineOfFire
				? ETunaSweeperRangedMoveKind::SeekLineOfFire
				: bRetreat || bSafeReloadMove
					? ETunaSweeperRangedMoveKind::Retreat
					: ETunaSweeperRangedMoveKind::Orbit;
	}

	const float MoveTimeoutSeconds = IsPairedSquadMember()
		? FMath::Max(0.25f, SquadState.LeaseRemainingSeconds)
		: UTunaSweeperEnemySquadSubsystem::RoleLeaseSeconds;
	EnterRangedState(
		bSeekLineOfFire
			? ETunaSweeperRangedCombatState::SeekLineOfFire
			: ETunaSweeperRangedCombatState::Reposition,
		MoveTimeoutSeconds);
	if (!RequestCurrentMoveGoal())
	{
		HandleMoveFinished(false);
	}
}

bool ATunaSweeperEnemyAIController::BuildNormalRepositionGoal(
	bool bSeekLineOfFire,
	bool bRetreat,
	bool bApproachTarget,
	FVector& OutGoal)
{
	APawn* ControlledPawn = GetPawn();
	AActor* TargetActor = CurrentTargetActor.Get();
	if (!ControlledPawn || !TargetActor)
	{
		return false;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = bHasDirectTargetSight
		? TargetActor->GetActorLocation()
		: LastKnownTargetLocation;
	const FVector OutwardDirection = GetPlanarDirection(TargetLocation, PawnLocation);
	if (OutwardDirection.IsNearlyZero())
	{
		return false;
	}
	if (RepositionRetryCount > 0)
	{
		RepositionSideSign *= -1.0f;
	}
	if (bSeekLineOfFire && IsPairedSquadMember())
	{
		RepositionSideSign = SquadState.SlotIndex == 0 ? -1.0f : 1.0f;
	}
	const FVector TangentDirection =
		FVector::CrossProduct(FVector::UpVector, OutwardDirection).GetSafeNormal() * RepositionSideSign;
	const float DistanceToTarget = FVector::Dist2D(PawnLocation, TargetLocation);
	const float PreferredMin = FMath::Max(CombatProfile.DangerRange, CombatProfile.PreferredRangeMin);
	const float PreferredMax = FMath::Max(PreferredMin, CombatProfile.PreferredRangeMax);

	FVector MoveDirection = TangentDirection;
	if (bApproachTarget)
	{
		MoveDirection = (TangentDirection * 0.3f - OutwardDirection).GetSafeNormal();
	}
	else if (bRetreat || DistanceToTarget < PreferredMin)
	{
		MoveDirection = (TangentDirection * 0.65f + OutwardDirection).GetSafeNormal();
	}
	else if (DistanceToTarget > PreferredMax)
	{
		MoveDirection = (TangentDirection - OutwardDirection * 0.55f).GetSafeNormal();
	}
	else if (bSeekLineOfFire)
	{
		MoveDirection = (TangentDirection - OutwardDirection * 0.1f).GetSafeNormal();
	}
	float MoveDistance = GetRandomRangeValue(
		FVector2D(CombatProfile.RepositionDistanceMin, CombatProfile.RepositionDistanceMax),
		150.0f);
	if (bApproachTarget)
	{
		const float DesiredDistance = FMath::Max(0.0f, ResolveAttackRange() - 75.0f);
		MoveDistance = FMath::Clamp(
			DistanceToTarget - DesiredDistance,
			FMath::Max(150.0f, CombatProfile.RepositionDistanceMin),
			FMath::Max(200.0f, CombatProfile.RepositionDistanceMax));
	}
	return ProjectAndValidateMoveGoal(PawnLocation + MoveDirection * MoveDistance, OutGoal);
}

bool ATunaSweeperEnemyAIController::BuildCrossRepositionPath(TArray<FVector>& OutWaypoints)
{
	OutWaypoints.Reset();
	const APawn* ControlledPawn = GetPawn();
	const AActor* TargetActor = CurrentTargetActor.Get();
	if (!ControlledPawn || !TargetActor || CombatProfile.Role != ETunaSweeperEnemyCombatRole::Flanker)
	{
		return false;
	}

	const FVector TargetSnapshot = TargetActor->GetActorLocation();
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector StartRadial = GetPlanarDirection(TargetSnapshot, PawnLocation);
	if (StartRadial.IsNearlyZero())
	{
		return false;
	}
	const float OrbitRadius = FMath::Max(150.0f, CombatProfile.CrossRepositionOrbitRadius);
	const float SideSign = RepositionSideSign;
	const float StartRadius = FVector::Dist2D(PawnLocation, TargetSnapshot);
	const FVector TangentDirection =
		FVector::CrossProduct(FVector::UpVector, StartRadial).GetSafeNormal() * SideSign;
	const float TangentStep = FMath::Clamp(
		GetRandomRangeValue(
			FVector2D(CombatProfile.RepositionDistanceMin, CombatProfile.RepositionDistanceMax),
			200.0f),
		200.0f,
		300.0f);
	const float OuterApproachRadius = FMath::Max(OrbitRadius + 240.0f, StartRadius - 180.0f);
	const float InnerApproachRadius = FMath::Max(OrbitRadius + 110.0f, OuterApproachRadius - 200.0f);
	TArray<FVector> RawGoals;
	// Commit to a tangential leg before reducing radius. This prevents the first
	// path request from becoming a straight charge through the player's capsule.
	RawGoals.Add(PawnLocation + TangentDirection * TangentStep);
	RawGoals.Add(TargetSnapshot +
		StartRadial.RotateAngleAxis(45.0f * SideSign, FVector::UpVector) * OuterApproachRadius);
	RawGoals.Add(TargetSnapshot +
		StartRadial.RotateAngleAxis(90.0f * SideSign, FVector::UpVector) * InnerApproachRadius);
	RawGoals.Add(TargetSnapshot +
		StartRadial.RotateAngleAxis(135.0f * SideSign, FVector::UpVector) * OrbitRadius);
	const FVector OppositeRadial = -StartRadial;
	RawGoals.Add(TargetSnapshot + OppositeRadial * OrbitRadius);
	RawGoals.Add(TargetSnapshot + OppositeRadial * 350.0f);
	RawGoals.Add(TargetSnapshot + OppositeRadial * FMath::Max(560.0f, CombatProfile.PreferredRangeMin));

	const float MinimumCapsuleClearance = FMath::Max(120.0f, OrbitRadius * 0.78f);
	auto SegmentClearsTarget = [&TargetSnapshot, MinimumCapsuleClearance](
		const FVector& SegmentStart,
		const FVector& SegmentEnd)
	{
		const FVector2D Start2D(SegmentStart.X, SegmentStart.Y);
		const FVector2D End2D(SegmentEnd.X, SegmentEnd.Y);
		const FVector2D Target2D(TargetSnapshot.X, TargetSnapshot.Y);
		const FVector2D Segment = End2D - Start2D;
		const float SegmentSizeSquared = Segment.SizeSquared();
		const float Alpha = SegmentSizeSquared > KINDA_SMALL_NUMBER
			? FMath::Clamp(FVector2D::DotProduct(Target2D - Start2D, Segment) / SegmentSizeSquared, 0.0f, 1.0f)
			: 0.0f;
		return FVector2D::Distance(Target2D, Start2D + Segment * Alpha) >= MinimumCapsuleClearance;
	};

	FVector SegmentStart = PawnLocation;
	for (const FVector& RawGoal : RawGoals)
	{
		const float SegmentDistance = FVector::Dist2D(SegmentStart, RawGoal);
		const int32 SegmentCount = FMath::Max(1, FMath::CeilToInt(SegmentDistance / 280.0f));
		const FVector RawSegmentStart = SegmentStart;
		FVector ValidatedSegmentStart = OutWaypoints.IsEmpty() ? PawnLocation : OutWaypoints.Last();
		for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const FVector Candidate = FMath::Lerp(
				RawSegmentStart,
				RawGoal,
				static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount));
			FVector Projected;
			if (!ProjectAndValidateMoveGoal(Candidate, Projected) ||
				FVector::Dist2D(Projected, TargetSnapshot) < MinimumCapsuleClearance ||
				!SegmentClearsTarget(ValidatedSegmentStart, Projected))
			{
				OutWaypoints.Reset();
				return false;
			}
			if (OutWaypoints.IsEmpty() || FVector::DistSquared2D(OutWaypoints.Last(), Projected) > FMath::Square(35.0f))
			{
				OutWaypoints.Add(Projected);
			}
			ValidatedSegmentStart = Projected;
		}
		SegmentStart = RawGoal;
	}
	if (OutWaypoints.IsEmpty())
	{
		return false;
	}
	float EstimatedPathLength = 0.0f;
	FVector PreviousWaypoint = PawnLocation;
	for (const FVector& Waypoint : OutWaypoints)
	{
		EstimatedPathLength += FVector::Dist2D(PreviousWaypoint, Waypoint);
		PreviousWaypoint = Waypoint;
	}
	const float SettleBudgetSeconds = FMath::Max(SquadSettleSeconds.X, SquadSettleSeconds.Y);
	const float AvailableLeaseSeconds = IsPairedSquadMember()
		? FMath::Max(0.0f, SquadState.LeaseRemainingSeconds)
		: UTunaSweeperEnemySquadSubsystem::RoleLeaseSeconds;
	const float TravelBudgetSeconds = FMath::Max(
		0.0f,
		AvailableLeaseSeconds - SettleBudgetSeconds - 0.25f);
	if (EstimatedPathLength / FMath::Max(1.0f, CombatProfile.MovementSpeed) > TravelBudgetSeconds)
	{
		OutWaypoints.Reset();
		return false;
	}
	return true;
}

bool ATunaSweeperEnemyAIController::ProjectAndValidateMoveGoal(
	const FVector& Candidate,
	FVector& OutProjectedGoal) const
{
	const APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavigationSystem = World
		? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World)
		: nullptr;
	if (!ControlledPawn || !World || !NavigationSystem)
	{
		return false;
	}

	FNavLocation NavLocation;
	if (!NavigationSystem->ProjectPointToNavigation(Candidate, NavLocation, FVector(90.0f, 90.0f, 180.0f)))
	{
		return false;
	}
	float CapsuleRadius = 34.0f;
	float CapsuleHalfHeight = 88.0f;
	if (const ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		if (const UCapsuleComponent* Capsule = ControlledCharacter->GetCapsuleComponent())
		{
			CapsuleRadius = Capsule->GetScaledCapsuleRadius();
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperEnemyMoveGoal), false, ControlledPawn);
	const FVector CapsuleTestCenter =
		NavLocation.Location + FVector::UpVector * CapsuleHalfHeight;
	if (World->OverlapBlockingTestByChannel(
		CapsuleTestCenter,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeCapsule(CapsuleRadius * 0.9f, CapsuleHalfHeight * 0.9f),
		QueryParams))
	{
		return false;
	}
	OutProjectedGoal = NavLocation.Location;
	return true;
}

bool ATunaSweeperEnemyAIController::DoesMoveSegmentClearCurrentTarget(
	const FVector& SegmentEnd,
	float MinimumClearance) const
{
	const APawn* ControlledPawn = GetPawn();
	const AActor* TargetActor = CurrentTargetActor.Get();
	if (!ControlledPawn || !TargetActor)
	{
		return false;
	}
	const FVector2D SegmentStart2D(
		ControlledPawn->GetActorLocation().X,
		ControlledPawn->GetActorLocation().Y);
	const FVector2D SegmentEnd2D(SegmentEnd.X, SegmentEnd.Y);
	const FVector2D TargetLocation2D(
		TargetActor->GetActorLocation().X,
		TargetActor->GetActorLocation().Y);
	const FVector2D Segment = SegmentEnd2D - SegmentStart2D;
	const float SegmentSizeSquared = Segment.SizeSquared();
	const float Alpha = SegmentSizeSquared > KINDA_SMALL_NUMBER
		? FMath::Clamp(
			FVector2D::DotProduct(TargetLocation2D - SegmentStart2D, Segment) /
				SegmentSizeSquared,
			0.0f,
			1.0f)
		: 0.0f;
	return FVector2D::Distance(
		TargetLocation2D,
		SegmentStart2D + Segment * Alpha) >= FMath::Max(0.0f, MinimumClearance);
}

bool ATunaSweeperEnemyAIController::RequestCurrentMoveGoal()
{
	const EPathFollowingRequestResult::Type RequestResult = MoveToLocation(
		RangedMoveGoal,
		FMath::Max(5.0f, RangedMoveGoalAcceptanceRadius),
		true,
		true,
		true,
		true,
		nullptr,
		false);
	if (RequestResult == EPathFollowingRequestResult::Failed)
	{
		bMoveRequestActive = false;
		ActiveMoveRequestId = FAIRequestID::InvalidRequest;
		ActiveMoveCycleId = 0;
		return false;
	}
	ActiveMoveCycleId = IsPairedSquadMember() ? SquadState.CycleId : 0;
	if (RequestResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		bMoveRequestActive = false;
		ActiveMoveRequestId = FAIRequestID::InvalidRequest;
		HandleMoveFinished(true);
		return true;
	}
	bMoveRequestActive = true;
	ActiveMoveRequestId = GetCurrentMoveRequestID();
	return true;
}

void ATunaSweeperEnemyAIController::OnMoveCompleted(
	FAIRequestID RequestId,
	const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestId, Result);
	if (!bMoveRequestActive || !RequestId.IsEquivalent(ActiveMoveRequestId))
	{
		return;
	}
	const int32 CompletedMoveCycleId = ActiveMoveCycleId;
	bMoveRequestActive = false;
	ActiveMoveRequestId = FAIRequestID::InvalidRequest;
	if (CompletedMoveCycleId > 0)
	{
		RefreshSquadState();
		if (!IsPairedSquadMember() || SquadState.CycleId != CompletedMoveCycleId)
		{
			ActiveMoveCycleId = 0;
			if (RangedCombatState != ETunaSweeperRangedCombatState::Reload)
			{
				StartObserve(0.15f);
			}
			return;
		}
	}
	HandleMoveFinished(Result.Code == EPathFollowingResult::Success);
}

void ATunaSweeperEnemyAIController::HandleMoveFinished(bool bSucceeded)
{
	if (RangedCombatState == ETunaSweeperRangedCombatState::Reload)
	{
		ActiveMoveCycleId = 0;
		StopMovement();
		return;
	}

	if (bSucceeded && RangedMoveKind == ETunaSweeperRangedMoveKind::CrossReposition &&
		CrossWaypointIndex != INDEX_NONE && CrossWaypointIndex + 1 < CrossRepositionWaypoints.Num())
	{
		if (IsPairedSquadMember())
		{
			RefreshSquadState();
			if (SquadState.Role != ETunaSweeperEnemySquadRole::Reposition ||
				ActiveMoveCycleId <= 0 || SquadState.CycleId != ActiveMoveCycleId)
			{
				ActiveMoveCycleId = 0;
				StopMovement();
				CrossRepositionWaypoints.Reset();
				CrossWaypointIndex = INDEX_NONE;
				StartObserve(0.15f);
				return;
			}
		}
		const FVector NextWaypoint = CrossRepositionWaypoints[CrossWaypointIndex + 1];
		if (!DoesMoveSegmentClearCurrentTarget(NextWaypoint, 120.0f))
		{
			ActiveMoveCycleId = 0;
			CrossRepositionWaypoints.Reset();
			CrossWaypointIndex = INDEX_NONE;
			StartReposition(false, false, false);
			return;
		}
		++CrossWaypointIndex;
		RangedMoveGoal = NextWaypoint;
		if (RequestCurrentMoveGoal())
		{
			return;
		}
		bSucceeded = false;
	}

	if (!bSucceeded && RepositionRetryCount < 1 &&
		RangedMoveKind != ETunaSweeperRangedMoveKind::CrossReposition &&
		RangedMoveKind != ETunaSweeperRangedMoveKind::HitEvade)
	{
		++RepositionRetryCount;
		const bool bSeek = RangedMoveKind == ETunaSweeperRangedMoveKind::SeekLineOfFire;
		const bool bRetreat = RangedMoveKind == ETunaSweeperRangedMoveKind::Retreat;
		const bool bApproach = RangedMoveKind == ETunaSweeperRangedMoveKind::Approach;
		if (BuildNormalRepositionGoal(bSeek, bRetreat, bApproach, RangedMoveGoal) &&
			RequestCurrentMoveGoal())
		{
			return;
		}
	}

	StopMovement();
	CrossRepositionWaypoints.Reset();
	CrossWaypointIndex = INDEX_NONE;
	if (!bSucceeded)
	{
		StartObserve(0.35f);
		return;
	}
	if (RangedMoveKind == ETunaSweeperRangedMoveKind::HitEvade)
	{
		if (ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(GetPawn()))
		{
			const FTunaSweeperEnemyWeaponRuntimeStatus Status = Enemy->GetEnemyWeaponRuntimeStatus();
			if (Status.LoadedAmmo <= 0 && Status.ReserveAmmo > 0)
			{
				StartReload(Enemy);
				return;
			}
		}
		StartObserve(0.25f);
		return;
	}

	bSquadMoverSettling = true;
	SettleCycleId = ActiveMoveCycleId;
	ActiveMoveCycleId = 0;
	RepositionSettleEndTimeSeconds = GetWorldTimeSeconds(this) +
		GetRandomRangeValue(SquadSettleSeconds, 0.0f);
	RangedCombatStateEndTimeSeconds = RepositionSettleEndTimeSeconds;
	RangedMoveKind = ETunaSweeperRangedMoveKind::None;
}

void ATunaSweeperEnemyAIController::FaceTarget(float DeltaSeconds, bool bSlowly)
{
	APawn* ControlledPawn = GetPawn();
	AActor* TargetActor = CurrentTargetActor.Get();
	if (!ControlledPawn || !TargetActor ||
		RangedCombatState == ETunaSweeperRangedCombatState::Recover)
	{
		return;
	}
	const FVector FacingLocation = bHasDirectTargetSight
		? TargetActor->GetActorLocation()
		: LastKnownTargetLocation;
	RotateTowardLocation(FacingLocation, DeltaSeconds, bSlowly ? 0.45f : 1.0f);
}

bool ATunaSweeperEnemyAIController::RotateTowardLocation(
	const FVector& FacingLocation,
	float DeltaSeconds,
	float SpeedScale)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || DeltaSeconds <= 0.0f)
	{
		return false;
	}
	const FVector Direction = GetPlanarDirection(ControlledPawn->GetActorLocation(), FacingLocation);
	if (Direction.IsNearlyZero())
	{
		return true;
	}
	const FRotator CurrentRotation = ControlledPawn->GetActorRotation();
	const FRotator DesiredRotation(0.0f, Direction.Rotation().Yaw, 0.0f);
	const float TurnSpeed = FMath::Max(
		1.0f,
		CombatProfile.TurnSpeedDegreesPerSecond * FMath::Max(0.0f, SpeedScale));
	const FRotator NewRotation = FMath::RInterpConstantTo(
		CurrentRotation,
		DesiredRotation,
		DeltaSeconds,
		TurnSpeed);
	ControlledPawn->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
	return IsFacingLocation(FacingLocation, CombatProfile.AttackFacingToleranceDegrees);
}

bool ATunaSweeperEnemyAIController::IsFacingLocation(
	const FVector& FacingLocation,
	float ToleranceDegrees) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}
	const FVector Direction = GetPlanarDirection(ControlledPawn->GetActorLocation(), FacingLocation);
	if (Direction.IsNearlyZero())
	{
		return true;
	}
	const float DesiredYaw = Direction.Rotation().Yaw;
	const float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(
		ControlledPawn->GetActorRotation().Yaw,
		DesiredYaw));
	return DeltaYaw <= FMath::Clamp(ToleranceDegrees, 0.0f, 90.0f);
}

bool ATunaSweeperEnemyAIController::IsFacingCurrentTarget() const
{
	const AActor* TargetActor = CurrentTargetActor.Get();
	return TargetActor && IsFacingLocation(
		TargetActor->GetActorLocation(),
		CombatProfile.AttackFacingToleranceDegrees);
}

ETunaSweeperLineOfFireResult ATunaSweeperEnemyAIController::EvaluateLineOfFire(AActor* TargetActor) const
{
	const APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !TargetActor || !World)
	{
		return ETunaSweeperLineOfFireResult::BlockedByIndestructible;
	}

	const FVector TraceStart = ControlledPawn->GetActorLocation() +
		FVector(0.0f, 0.0f, FMath::Max(0.0f, RangedLineOfFireTraceHeight));
	const FVector TraceEnd = TargetActor->GetActorLocation() +
		FVector(0.0f, 0.0f, FMath::Max(0.0f, RangedTargetTraceHeight));
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperEnemyLineOfFire), false, ControlledPawn);
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
	if (const UTunaSweeperFactionSubsystem* FactionSubsystem =
		World->GetSubsystem<UTunaSweeperFactionSubsystem>();
		FactionSubsystem && FactionSubsystem->AreActorsFriendly(ControlledPawn, HitActor))
	{
		return ETunaSweeperLineOfFireResult::BlockedByFriendly;
	}
	return Cast<ATunaSweeperSandbagCoverActor>(HitActor)
		? ETunaSweeperLineOfFireResult::BlockedByDestructible
		: ETunaSweeperLineOfFireResult::BlockedByIndestructible;
}

bool ATunaSweeperEnemyAIController::CanAcquireCombatTarget(
	AActor* TargetActor,
	float DistanceToTarget) const
{
	const APawn* ControlledPawn = GetPawn();
	const UWorld* World = GetWorld();
	const UTunaSweeperFactionSubsystem* FactionSubsystem = World
		? World->GetSubsystem<UTunaSweeperFactionSubsystem>()
		: nullptr;
	if (!ControlledPawn || !FactionSubsystem || IsUnavailableCombatTarget(TargetActor) ||
		DistanceToTarget > ResolveTrackingRange() ||
		!FactionSubsystem->CanTargetActor(ControlledPawn, TargetActor))
	{
		return false;
	}

	FVector DirectionToTarget = GetPlanarDirection(
		ControlledPawn->GetActorLocation(),
		TargetActor->GetActorLocation());
	if (!DirectionToTarget.IsNearlyZero() && AwarenessState != ETunaSweeperEnemyAwarenessState::Combat)
	{
		FVector ForwardDirection = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
		if (ForwardDirection.IsNearlyZero())
		{
			ForwardDirection = NonCombatFacingDirection.GetSafeNormal2D();
		}
		const float VisionAngle = FMath::Clamp(CombatVisionAngleDegrees, 0.0f, 360.0f);
		if (VisionAngle < 360.0f && FVector::DotProduct(ForwardDirection, DirectionToTarget) <
			FMath::Cos(FMath::DegreesToRadians(VisionAngle * 0.5f)))
		{
			return false;
		}
	}
	return HasDirectSightTo(TargetActor);
}

bool ATunaSweeperEnemyAIController::HasDirectSightTo(AActor* TargetActor) const
{
	const ETunaSweeperLineOfFireResult Result = EvaluateLineOfFire(TargetActor);
	return Result == ETunaSweeperLineOfFireResult::Clear ||
		Result == ETunaSweeperLineOfFireResult::BlockedByDestructible;
}

void ATunaSweeperEnemyAIController::RegisterWithSquad()
{
	if (bSquadRegistered)
	{
		RefreshSquadState();
		return;
	}
	ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(GetPawn());
	UWorld* World = GetWorld();
	if (!Enemy || !World || Enemy->UsesMeleeAttack())
	{
		return;
	}
	const UTunaSweeperFactionComponent* FactionComponent = Enemy->GetFactionComponent();
	UTunaSweeperEnemySquadSubsystem* SquadSubsystem = World->GetSubsystem<UTunaSweeperEnemySquadSubsystem>();
	if (!FactionComponent || !SquadSubsystem)
	{
		return;
	}
	FTunaSweeperEnemySquadKey SquadKey;
	SquadKey.FactionId = FactionComponent->GetFactionId();
	SquadKey.SquadId = FactionComponent->GetSquadId();
	const int32 SquadSlot = FactionComponent->GetSquadSlot();
	if (!SquadKey.IsValid() || SquadSlot < 0 || SquadSlot > 1)
	{
		return;
	}

	SquadSubsystem->OnMemberStateChanged.RemoveAll(this);
	SquadSubsystem->OnMemberStateChanged.AddUObject(
		this,
		&ATunaSweeperEnemyAIController::HandleSquadStateChanged);
	bSquadRegistered = SquadSubsystem->RegisterMember(GetPawn(), SquadKey, SquadSlot, SquadState);
	if (!bSquadRegistered)
	{
		SquadSubsystem->OnMemberStateChanged.RemoveAll(this);
		SquadState = FTunaSweeperEnemySquadState();
	}
}

void ATunaSweeperEnemyAIController::UnregisterFromSquad()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
			World->GetSubsystem<UTunaSweeperEnemySquadSubsystem>())
		{
			SquadSubsystem->OnMemberStateChanged.RemoveAll(this);
			if (bSquadRegistered && GetPawn())
			{
				SquadSubsystem->UnregisterMember(GetPawn());
			}
		}
	}
	bSquadRegistered = false;
	SquadState = FTunaSweeperEnemySquadState();
}

void ATunaSweeperEnemyAIController::RefreshSquadState()
{
	if (!bSquadRegistered || !GetPawn() || !GetWorld())
	{
		return;
	}
	if (UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
		GetWorld()->GetSubsystem<UTunaSweeperEnemySquadSubsystem>())
	{
		if (!SquadSubsystem->GetMemberState(GetPawn(), SquadState))
		{
			bSquadRegistered = false;
			SquadState = FTunaSweeperEnemySquadState();
		}
	}
}

void ATunaSweeperEnemyAIController::HandleSquadStateChanged(
	APawn* MemberPawn,
	const FTunaSweeperEnemySquadState& NewState,
	ETunaSweeperEnemySquadUpdateReason Reason)
{
	if (MemberPawn != GetPawn())
	{
		return;
	}
	const ETunaSweeperEnemySquadRole PreviousRole = SquadState.Role;
	SquadState = NewState;
	bSquadRegistered = NewState.bRegistered;
	const bool bPairJustFormed =
		Reason == ETunaSweeperEnemySquadUpdateReason::PairFormed &&
		PreviousRole == ETunaSweeperEnemySquadRole::Solo &&
		NewState.Role != ETunaSweeperEnemySquadRole::Solo;
	const bool bMovementLeaseLost =
		PreviousRole == ETunaSweeperEnemySquadRole::Reposition &&
		NewState.Role != ETunaSweeperEnemySquadRole::Reposition;
	if (bIsCombatEngaged && bPairJustFormed &&
		RangedCombatState == ETunaSweeperRangedCombatState::Firing)
	{
		if (ShotsFiredThisFiring > 0)
		{
			++FiringsAtCurrentPosition;
			bOpeningFiring = false;
			NextAllowedFireTimeSeconds = FMath::Max(
				NextAllowedFireTimeSeconds,
				LastShotTimeSeconds + CombatProfile.RecoverSecondsMin + CombatProfile.ObserveSecondsMin);
		}
		ShotsPlannedThisFiring = 0;
		ShotsFiredThisFiring = 0;
		bSquadLastShotReported = true;
		if (bOpeningFiring)
		{
			StartObserve(0.15f);
		}
		else
		{
			StartRecover();
		}
	}
	if (bIsCombatEngaged && (bPairJustFormed || bMovementLeaseLost) &&
		(bMoveRequestActive || bSquadMoverSettling || CrossWaypointIndex != INDEX_NONE))
	{
		bMoveRequestActive = false;
		ActiveMoveRequestId = FAIRequestID::InvalidRequest;
		ActiveMoveCycleId = 0;
		bSquadMoverSettling = false;
		SettleCycleId = 0;
		bPendingReloadSafeMove = false;
		StopMovement();
		CrossRepositionWaypoints.Reset();
		CrossWaypointIndex = INDEX_NONE;
		if (RangedCombatState != ETunaSweeperRangedCombatState::Reload)
		{
			StartObserve(0.15f);
		}
	}
	if (bIsCombatEngaged && PreviousRole == ETunaSweeperEnemySquadRole::Suppress &&
		NewState.Role == ETunaSweeperEnemySquadRole::Reposition &&
		RangedCombatState == ETunaSweeperRangedCombatState::Firing)
	{
		if (ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(GetPawn()))
		{
			FinishFiring(Enemy, LastShotTimeSeconds);
		}
	}
	if (!CurrentTargetActor.IsValid() && AwarenessState == ETunaSweeperEnemyAwarenessState::Unaware &&
		NewState.bHasSharedLastKnownLocation)
	{
		StartSuspicion(NewState.SharedLastKnownLocation, TEXT("Search: squad contact"));
	}
}

bool ATunaSweeperEnemyAIController::IsPairedSquadMember() const
{
	return bSquadRegistered && SquadState.bRegistered &&
		SquadState.Role != ETunaSweeperEnemySquadRole::Solo;
}

bool ATunaSweeperEnemyAIController::CanSquadMemberStartFiring() const
{
	if (GetWorldTimeSeconds(this) + KINDA_SMALL_NUMBER < NextAllowedFireTimeSeconds)
	{
		return false;
	}
	return !IsPairedSquadMember() ||
		(SquadState.Role == ETunaSweeperEnemySquadRole::Suppress &&
			SquadState.NextFireRemainingSeconds <= KINDA_SMALL_NUMBER);
}

bool ATunaSweeperEnemyAIController::CanSquadMoverStartReposition() const
{
	return IsPairedSquadMember() &&
		SquadState.Role == ETunaSweeperEnemySquadRole::Reposition &&
		(SquadState.bSuppressionStarted || SquadState.bLineOfFireRecoveryRequested) &&
		SquadState.MoverStartRemainingSeconds <= KINDA_SMALL_NUMBER;
}

bool ATunaSweeperEnemyAIController::RequestSquadLineOfFireRecovery()
{
	if (!IsPairedSquadMember() || SquadState.Role != ETunaSweeperEnemySquadRole::Suppress ||
		!GetPawn() || !GetWorld())
	{
		return false;
	}
	UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
		GetWorld()->GetSubsystem<UTunaSweeperEnemySquadSubsystem>();
	if (!SquadSubsystem ||
		!SquadSubsystem->RequestLineOfFireRecovery(GetPawn(), SquadState.CycleId))
	{
		RefreshSquadState();
		return false;
	}
	RefreshSquadState();
	return true;
}

void ATunaSweeperEnemyAIController::ReportSquadContact(
	AActor* TargetActor,
	const FVector& LastKnownLocation)
{
	if (!bSquadRegistered || !GetPawn() || !GetWorld())
	{
		return;
	}
	if (UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
		GetWorld()->GetSubsystem<UTunaSweeperEnemySquadSubsystem>())
	{
		if (!SquadSubsystem->ReportHostileContact(
			GetPawn(),
			SquadState.CycleId,
			TargetActor,
			LastKnownLocation))
		{
			RefreshSquadState();
		}
	}
}

void ATunaSweeperEnemyAIController::ReportSquadShot(bool bFirstShot, bool bLastShot)
{
	if (!IsPairedSquadMember() || !GetPawn() || !GetWorld())
	{
		return;
	}
	if (UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
		GetWorld()->GetSubsystem<UTunaSweeperEnemySquadSubsystem>())
	{
		if (!SquadSubsystem->ReportSuppressionShot(
			GetPawn(),
			SquadState.CycleId,
			bFirstShot,
			bLastShot))
		{
			RefreshSquadState();
		}
	}
}

void ATunaSweeperEnemyAIController::CompleteSquadMoverRole(int32 ExpectedCycleId)
{
	if (!IsPairedSquadMember() || SquadState.Role != ETunaSweeperEnemySquadRole::Reposition ||
		!GetPawn() || !GetWorld())
	{
		return;
	}
	if (UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
		GetWorld()->GetSubsystem<UTunaSweeperEnemySquadSubsystem>())
	{
		if (ExpectedCycleId <= 0 ||
			!SquadSubsystem->ReportRoleCompleted(GetPawn(), ExpectedCycleId))
		{
			RefreshSquadState();
		}
	}
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
	if (!ControlledPawn || !World)
	{
		return;
	}
	UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>();
	if (!NoiseSubsystem)
	{
		return;
	}
	FTunaSweeperHeardNoiseEvent HeardNoise;
	if (NoiseSubsystem->CalculateHeardNoiseAtLocation(
		NoiseEvent,
		ControlledPawn->GetActorLocation(),
		HearingRange,
		HearingSensitivity,
		HearingMinimumStrength,
		HeardNoise,
		ControlledPawn))
	{
		StartSuspicion(HeardNoise.SourceLocation, TEXT("Search: heard noise"));
	}
}

void ATunaSweeperEnemyAIController::RecordCombatDebugEntryReason(const TCHAR* EntryReason)
{
	CombatDebugEntryReason = EntryReason ? EntryReason : TEXT("");
	CombatDebugEntryReasonTimeSeconds = GetWorldTimeSeconds(this);
}

void ATunaSweeperEnemyAIController::DrawCombatDebug() const
{
	if (!bDrawCombatDebug || !GetPawn() || !GetWorld())
	{
		return;
	}
	const FVector Origin = GetPawn()->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
	const FColor StateColor = RangedCombatState == ETunaSweeperRangedCombatState::Firing
		? FColor::Red
		: RangedCombatState == ETunaSweeperRangedCombatState::Reposition ||
			RangedCombatState == ETunaSweeperRangedCombatState::SeekLineOfFire
			? FColor::Cyan
			: FColor::Yellow;
	DrawDebugSphere(GetWorld(), Origin, 12.0f, 8, StateColor, false, 0.0f, 0, 1.5f);
	if (bMoveRequestActive)
	{
		DrawDebugLine(GetWorld(), Origin, RangedMoveGoal + FVector(0.0f, 0.0f, 20.0f), StateColor, false, 0.0f, 0, 2.0f);
	}
}

float ATunaSweeperEnemyAIController::ResolveCombatDisengageRange() const
{
	return FMath::Max(ResolveTrackingRange(), CombatDisengageRange);
}

float ATunaSweeperEnemyAIController::ResolveTrackingRange() const
{
	return FMath::Max(0.0f, CombatProfile.TrackingRange);
}

float ATunaSweeperEnemyAIController::ResolveAttackRange() const
{
	return FMath::Max(0.0f, CombatProfile.AttackRange);
}

float ATunaSweeperEnemyAIController::ResolveApproachStartRange() const
{
	return FMath::Max(0.0f, CombatProfile.MeleeApproachStartRange);
}

float ATunaSweeperEnemyAIController::ResolveApproachStopRange() const
{
	return FMath::Max(0.0f, CombatProfile.MeleeApproachStopRange);
}

float ATunaSweeperEnemyAIController::ResolveAttackCooldownSeconds() const
{
	return FMath::Max(0.05f, CombatProfile.AttackCooldownSeconds);
}

float ATunaSweeperEnemyAIController::GetRandomRangeValue(
	const FVector2D& ValueRange,
	float MinValue)
{
	return FMath::Max(
		MinValue,
		FMath::FRandRange(
			FMath::Min(ValueRange.X, ValueRange.Y),
			FMath::Max(ValueRange.X, ValueRange.Y)));
}

FVector ATunaSweeperEnemyAIController::GetRandomPlanarDirection()
{
	const float AngleRadians = FMath::FRandRange(0.0f, 2.0f * PI);
	return FVector(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f);
}
