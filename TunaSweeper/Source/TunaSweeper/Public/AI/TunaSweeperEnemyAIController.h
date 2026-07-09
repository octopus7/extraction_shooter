#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TunaSweeperEnemyAIController.generated.h"

enum class ETunaSweeperRangedCombatState : uint8
{
	Idle,
	AdvanceBurst,
	HoldFire,
	SeekLineOfFire,
	KeepDistance
};

enum class ETunaSweeperNonCombatState : uint8
{
	Idle,
	Wander
};

enum class ETunaSweeperLineOfFireResult : uint8
{
	Clear,
	BlockedByDestructible,
	BlockedByIndestructible
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATunaSweeperEnemyAIController();

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float UpdateInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D UpdateIntervalRandomOffset = FVector2D(-0.04f, 0.06f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float TrackingRange = 2300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D TrackingRangeRandomOffset = FVector2D(0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float CombatDisengageRange = 3600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", ClampMax = "360.0", UIMin = "0.0", UIMax = "360.0"))
	float CombatVisionAngleDegrees = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|NonCombat")
	FVector2D IdleSeconds = FVector2D(1.4f, 3.7f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|NonCombat")
	FVector2D WanderSeconds = FVector2D(0.9f, 2.4f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|NonCombat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WanderMoveSpeed = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackRange = 1450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D AttackRangeRandomOffset = FVector2D(-120.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float ApproachStartRange = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D ApproachStartRangeRandomOffset = FVector2D(-120.0f, 180.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float ApproachStopRange = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D ApproachStopRangeRandomOffset = FVector2D(-140.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float MinApproachRangeGap = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackCooldownSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D AttackCooldownRandomOffset = FVector2D(-0.25f, 0.45f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedPreferredRangeMin = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedPreferredRangeMax = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedDangerCloseRange = 430.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedLongAdvanceThreshold = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedMediumAdvanceThreshold = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedLongAdvanceDistance = FVector2D(400.0f, 600.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedMediumAdvanceDistance = FVector2D(250.0f, 400.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedLongHoldSeconds = FVector2D(1.6f, 2.4f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedMediumHoldSeconds = FVector2D(2.4f, 3.6f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedPreferredHoldSeconds = FVector2D(2.8f, 4.4f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedSeekLineOfFireSeconds = FVector2D(0.8f, 1.4f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedKeepDistanceSeconds = FVector2D(0.5f, 0.9f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedAdvanceStrafeWeight = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedSeekForwardWeight = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedKeepDistanceStrafeWeight = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedMoveGoalAcceptanceRadius = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedLineOfFireTraceHeight = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedTargetTraceHeight = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Debug")
	bool bDrawCombatDebug = false;

private:
	void RandomizeCombatTuning();
	void UpdateAttackTarget();
	void UpdateNonCombatState(float DeltaSeconds);
	void StartNonCombatIdle();
	void StartNonCombatWander();
	void UpdateApproachState(float DistanceToTarget, float InApproachStartRange, float InApproachStopRange);
	void MoveTowardCurrentTarget(float DeltaSeconds);
	void UpdateRangedCombatState(float DistanceToTarget, AActor* TargetActor, class ATunaSweeperEnemyCharacter* EnemyCharacter);
	void MoveRangedCombatState(float DeltaSeconds);
	void StartRangedAdvance(float DistanceToTarget, const FVector& DirectionToTarget);
	void StartRangedHold(float DistanceToTarget, bool bOpeningHold = false);
	void StartRangedSeekLineOfFire(float DistanceToTarget, const FVector& DirectionToTarget);
	void StartRangedKeepDistance(const FVector& DirectionToTarget);
	void TryRangedAttack(float DistanceToTarget, AActor* TargetActor, class ATunaSweeperEnemyCharacter* EnemyCharacter, ETunaSweeperLineOfFireResult LineOfFireResult);
	ETunaSweeperLineOfFireResult EvaluateLineOfFire(AActor* TargetActor) const;
	bool CanAcquireCombatTarget(AActor* TargetActor, float DistanceToTarget) const;
	void DrawCombatDebug() const;
	float ResolveRangedAttackRange() const;
	float ResolveRangedAdvanceDistance(float DistanceToTarget) const;
	float ResolveRangedHoldSeconds(float DistanceToTarget) const;
	float ResolveRangedMoveDuration(float MoveDistance) const;
	float ResolveCombatDisengageRange() const;
	static float GetRandomRangeValue(const FVector2D& ValueRange, float MinValue);
	static FVector GetRandomPlanarDirection();
	void ClearCombatTarget();
	float ResolveTrackingRange() const;
	float ResolveAttackRange() const;
	float ResolveApproachStartRange() const;
	float ResolveApproachStopRange() const;
	float ResolveAttackCooldownSeconds() const;

	FTimerHandle UpdateTimerHandle;
	TWeakObjectPtr<AActor> CurrentTargetActor;
	float EffectiveUpdateInterval = 0.25f;
	float EffectiveTrackingRange = 2300.0f;
	float EffectiveAttackRange = 1450.0f;
	float EffectiveApproachStartRange = 1000.0f;
	float EffectiveApproachStopRange = 800.0f;
	float EffectiveAttackCooldownSeconds = 1.5f;
	double LastAttackTimeSeconds = -1000.0;
	double NonCombatStateEndTimeSeconds = 0.0;
	double RangedCombatStateEndTimeSeconds = 0.0;
	FVector NonCombatFacingDirection = FVector::ForwardVector;
	FVector RangedMoveDirection = FVector::ZeroVector;
	FVector RangedMoveGoal = FVector::ZeroVector;
	ETunaSweeperNonCombatState NonCombatState = ETunaSweeperNonCombatState::Idle;
	ETunaSweeperRangedCombatState RangedCombatState = ETunaSweeperRangedCombatState::Idle;
	bool bIsCombatEngaged = false;
	bool bIsClosingDistance = false;
	bool bIsOpeningHold = false;
	bool bHasRandomizedCombatTuning = false;
};
