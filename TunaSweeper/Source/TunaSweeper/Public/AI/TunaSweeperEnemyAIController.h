#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AITypes.h"
#include "AI/TunaSweeperEnemyCombatProfile.h"
#include "AI/TunaSweeperEnemySquadTypes.h"
#include "GenericTeamAgentInterface.h"
#include "TunaSweeperEnemyAIController.generated.h"

struct FPathFollowingResult;
struct FTunaSweeperNoiseEvent;

enum class ETunaSweeperRangedCombatState : uint8
{
	Idle,
	Aim,
	Firing,
	Recover,
	Observe,
	Reposition,
	Reload,
	HitEvade,
	SeekLineOfFire
};

enum class ETunaSweeperRangedMoveKind : uint8
{
	None,
	Orbit,
	Approach,
	Retreat,
	SeekLineOfFire,
	CrossReposition,
	HitEvade
};

enum class ETunaSweeperNonCombatState : uint8
{
	Idle,
	Wander
};

enum class ETunaSweeperEnemyAwarenessState : uint8
{
	Unaware,
	Suspicious,
	Alerted,
	Combat
};

enum class ETunaSweeperLineOfFireResult : uint8
{
	Clear,
	BlockedByFriendly,
	BlockedByDestructible,
	BlockedByIndestructible
};

struct FTunaSweeperEnemyCombatDebugSnapshot
{
	bool bIsCombatEngaged = false;
	bool bHasDirectTargetSight = false;
	FString StateLabel;
	float RemainingStateSeconds = 0.0f;
	float MaxStateSeconds = 0.0f;
	float TrackingRange = 0.0f;
	float VisionAngleDegrees = 0.0f;
	float HearingRange = 0.0f;
	FString RecentEntryReason;
	float RecentEntryReasonRemainingSeconds = 0.0f;
	FVector FacingDirection = FVector::ForwardVector;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATunaSweeperEnemyAIController();

	bool GetCombatDebugSnapshot(FTunaSweeperEnemyCombatDebugSnapshot& OutSnapshot) const;
	void NotifySuspicionAtLocation(const FVector& SuspicionLocation);
	void NotifyDamageTaken(AActor* SuspectedActor);

	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestId, const FPathFollowingResult& Result) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float UpdateInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D UpdateIntervalRandomOffset = FVector2D(-0.04f, 0.06f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float CombatDisengageRange = 3600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float CombatVisionAngleDegrees = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float HearingRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float HearingSensitivity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float HearingMinimumStrength = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Suspicion")
	FVector2D SuspicionSearchSeconds = FVector2D(2.6f, 3.8f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Suspicion", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SearchSweepHalfAngleDegrees = 48.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|NonCombat")
	FVector2D IdleSeconds = FVector2D(1.4f, 3.7f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|NonCombat")
	FVector2D WanderSeconds = FVector2D(0.9f, 2.4f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|NonCombat", meta = (ClampMin = "0.0"))
	float WanderMoveSpeed = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0"))
	float RangedMoveGoalAcceptanceRadius = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0"))
	float RangedLineOfFireTraceHeight = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0"))
	float RangedTargetTraceHeight = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D SquadSettleSeconds = FVector2D(0.35f, 0.55f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0"))
	float LineOfFireFailureSeconds = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float TargetSightLossGraceSeconds = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D HitEvadeDistance = FVector2D(150.0f, 250.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0"))
	float HitEvadeCooldownSeconds = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SoloDangerReloadMoveSpeedScale = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Debug")
	bool bDrawCombatDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Debug", meta = (ClampMin = "0.0"))
	float CombatDebugEntryReasonDisplaySeconds = 4.0f;

private:
	void InitializeFromControlledCharacter();
	void UpdateAttackTarget();
	AActor* FindBestHostileTarget() const;
	void UpdateAwarenessState(float DeltaSeconds);
	void UpdateNonCombatState(float DeltaSeconds);
	void StartNonCombatIdle();
	void StartNonCombatWander();
	void StartSuspicion(const FVector& InSuspicionLocation, const TCHAR* EntryReason);
	void StartAlerted(AActor* TargetActor, const TCHAR* EntryReason);
	void EnterCombat();
	void ClearCombatTarget();
	void RecordCombatDebugEntryReason(const TCHAR* EntryReason);

	void UpdateMeleeCombat(
		float DistanceToTarget,
		class ATunaSweeperEnemyCharacter* EnemyCharacter,
		float DeltaSeconds);
	void MoveTowardCurrentTarget(float DeltaSeconds);
	void TickRangedCombat(float DeltaSeconds, class ATunaSweeperEnemyCharacter* EnemyCharacter, AActor* TargetActor);
	void EnterRangedState(ETunaSweeperRangedCombatState NewState, float DurationSeconds);
	void StartAim(bool bFromReposition = false);
	void StartFiring(class ATunaSweeperEnemyCharacter* EnemyCharacter);
	void TickFiring(class ATunaSweeperEnemyCharacter* EnemyCharacter, AActor* TargetActor, double CurrentTimeSeconds);
	void FinishFiring(class ATunaSweeperEnemyCharacter* EnemyCharacter, double LastShotTimeSeconds);
	void StartRecover();
	void StartObserve(float DurationOverride = -1.0f);
	void StartReload(class ATunaSweeperEnemyCharacter* EnemyCharacter);
	void TickReload(class ATunaSweeperEnemyCharacter* EnemyCharacter, double CurrentTimeSeconds);
	void StartHitEvade(AActor* ThreatActor);
	void StartReposition(
		bool bSeekLineOfFire = false,
		bool bSafeReloadMove = false,
		bool bApproachTarget = false);
	bool BuildNormalRepositionGoal(
		bool bSeekLineOfFire,
		bool bRetreat,
		bool bApproachTarget,
		FVector& OutGoal);
	bool BuildCrossRepositionPath(TArray<FVector>& OutWaypoints);
	bool ProjectAndValidateMoveGoal(const FVector& Candidate, FVector& OutProjectedGoal) const;
	bool DoesMoveSegmentClearCurrentTarget(const FVector& SegmentEnd, float MinimumClearance) const;
	bool RequestCurrentMoveGoal();
	void HandleMoveFinished(bool bSucceeded);
	void FaceTarget(float DeltaSeconds, bool bSlowly);
	bool RotateTowardLocation(const FVector& FacingLocation, float DeltaSeconds, float SpeedScale = 1.0f);
	bool IsFacingLocation(const FVector& FacingLocation, float ToleranceDegrees) const;
	bool IsFacingCurrentTarget() const;
	ETunaSweeperLineOfFireResult EvaluateLineOfFire(AActor* TargetActor) const;
	bool CanAcquireCombatTarget(AActor* TargetActor, float DistanceToTarget) const;
	bool HasDirectSightTo(AActor* TargetActor) const;

	void RegisterWithSquad();
	void UnregisterFromSquad();
	void RefreshSquadState();
	void HandleSquadStateChanged(
		APawn* MemberPawn,
		const FTunaSweeperEnemySquadState& NewState,
		ETunaSweeperEnemySquadUpdateReason Reason);
	bool IsPairedSquadMember() const;
	bool CanSquadMemberStartFiring() const;
	bool CanSquadMoverStartReposition() const;
	bool RequestSquadLineOfFireRecovery();
	void ReportSquadContact(AActor* TargetActor, const FVector& LastKnownLocation);
	void ReportSquadShot(bool bFirstShot, bool bLastShot);
	void CompleteSquadMoverRole(int32 ExpectedCycleId);

	void HandleNoiseReported(const FTunaSweeperNoiseEvent& NoiseEvent);
	void DrawCombatDebug() const;
	float ResolveCombatDisengageRange() const;
	float ResolveTrackingRange() const;
	float ResolveAttackRange() const;
	float ResolveApproachStartRange() const;
	float ResolveApproachStopRange() const;
	float ResolveAttackCooldownSeconds() const;
	static float GetRandomRangeValue(const FVector2D& ValueRange, float MinValue);
	static FVector GetRandomPlanarDirection();

	FTimerHandle UpdateTimerHandle;
	TWeakObjectPtr<AActor> CurrentTargetActor;
	FTunaSweeperEnemyCombatProfile CombatProfile;
	FTunaSweeperEnemySquadState SquadState;
	FGenericTeamId CachedTeamId = FGenericTeamId::NoTeam;
	FAIRequestID ActiveMoveRequestId = FAIRequestID::InvalidRequest;

	float EffectiveUpdateInterval = 0.25f;
	double NonCombatStateEndTimeSeconds = 0.0;
	double RangedCombatStateStartTimeSeconds = 0.0;
	double RangedCombatStateEndTimeSeconds = 0.0;
	double AwarenessStateStartTimeSeconds = 0.0;
	double AwarenessStateEndTimeSeconds = 0.0;
	double CombatDebugEntryReasonTimeSeconds = -1000.0;
	double LastMeleeAttackTimeSeconds = -1000.0;
	double NextShotTimeSeconds = 0.0;
	double NextAllowedFireTimeSeconds = 0.0;
	double LastShotTimeSeconds = -1000.0;
	double LineOfFireBlockedStartTimeSeconds = -1.0;
	double LastHitEvadeTimeSeconds = -1000.0;
	double LastCrossRepositionTimeSeconds = -1000.0;
	double TargetSightLostTimeSeconds = -1.0;
	double ReloadReadyEndTimeSeconds = 0.0;
	double RepositionSettleEndTimeSeconds = 0.0;

	FString CombatDebugEntryReason;
	FVector NonCombatFacingDirection = FVector::ForwardVector;
	FVector SuspicionLocation = FVector::ZeroVector;
	FVector LastKnownTargetLocation = FVector::ZeroVector;
	FVector RangedMoveGoal = FVector::ZeroVector;
	TArray<FVector> CrossRepositionWaypoints;

	int32 ShotsPlannedThisFiring = 0;
	int32 ShotsFiredThisFiring = 0;
	int32 FiringsAtCurrentPosition = 0;
	int32 PositionFiringBudget = 1;
	int32 CrossWaypointIndex = INDEX_NONE;
	int32 RepositionRetryCount = 0;
	int32 ActiveMoveCycleId = 0;
	int32 SettleCycleId = 0;
	float RepositionSideSign = 1.0f;

	ETunaSweeperNonCombatState NonCombatState = ETunaSweeperNonCombatState::Idle;
	ETunaSweeperRangedCombatState RangedCombatState = ETunaSweeperRangedCombatState::Idle;
	ETunaSweeperRangedMoveKind RangedMoveKind = ETunaSweeperRangedMoveKind::None;
	ETunaSweeperEnemyAwarenessState AwarenessState = ETunaSweeperEnemyAwarenessState::Unaware;

	bool bIsCombatEngaged = false;
	bool bIsClosingDistance = false;
	bool bOpeningFiring = true;
	bool bHasDirectTargetSight = false;
	bool bAlertBubbleShownThisCycle = false;
	bool bMoveRequestActive = false;
	bool bMoveGoalSucceeded = false;
	bool bReloadWasStarted = false;
	bool bSquadLastShotReported = false;
	bool bSquadRegistered = false;
	bool bSquadMoverSettling = false;
	bool bSafeReloadMoveUsed = false;
	bool bPendingReloadSafeMove = false;
	bool bReloadMovementSpeedReduced = false;
};
