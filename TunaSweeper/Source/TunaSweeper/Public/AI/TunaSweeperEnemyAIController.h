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
	float TrackingRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Randomization")
	FVector2D TrackingRangeRandomOffset = FVector2D(-180.0f, 220.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackRange = 1000.0f;

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
	float RangedPreferredRangeMin = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedPreferredRangeMax = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedDangerCloseRange = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedLongAdvanceThreshold = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RangedMediumAdvanceThreshold = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedLongAdvanceDistance = FVector2D(400.0f, 600.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedMediumAdvanceDistance = FVector2D(250.0f, 400.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedLongHoldSeconds = FVector2D(0.8f, 1.2f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedMediumHoldSeconds = FVector2D(1.2f, 1.8f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Ranged Combat")
	FVector2D RangedPreferredHoldSeconds = FVector2D(1.4f, 2.2f);

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

private:
	void RandomizeCombatTuning();
	void UpdateAttackTarget();
	void UpdateApproachState(float DistanceToTarget, float InApproachStartRange, float InApproachStopRange);
	void MoveTowardCurrentTarget(float DeltaSeconds);
	void UpdateRangedCombatState(float DistanceToTarget, AActor* TargetActor, class ATunaSweeperEnemyCharacter* EnemyCharacter);
	void MoveRangedCombatState(float DeltaSeconds);
	void StartRangedAdvance(float DistanceToTarget, const FVector& DirectionToTarget);
	void StartRangedHold(float DistanceToTarget);
	void StartRangedSeekLineOfFire(float DistanceToTarget, const FVector& DirectionToTarget);
	void StartRangedKeepDistance(const FVector& DirectionToTarget);
	void TryRangedAttack(float DistanceToTarget, AActor* TargetActor, class ATunaSweeperEnemyCharacter* EnemyCharacter, ETunaSweeperLineOfFireResult LineOfFireResult);
	ETunaSweeperLineOfFireResult EvaluateLineOfFire(AActor* TargetActor) const;
	float ResolveRangedAttackRange() const;
	float ResolveRangedAdvanceDistance(float DistanceToTarget) const;
	float ResolveRangedHoldSeconds(float DistanceToTarget) const;
	float ResolveRangedMoveDuration(float MoveDistance) const;
	static float GetRandomRangeValue(const FVector2D& ValueRange, float MinValue);
	void ClearCombatTarget();
	float ResolveTrackingRange() const;
	float ResolveAttackRange() const;
	float ResolveApproachStartRange() const;
	float ResolveApproachStopRange() const;
	float ResolveAttackCooldownSeconds() const;

	FTimerHandle UpdateTimerHandle;
	TWeakObjectPtr<AActor> CurrentTargetActor;
	float EffectiveUpdateInterval = 0.25f;
	float EffectiveTrackingRange = 1800.0f;
	float EffectiveAttackRange = 1000.0f;
	float EffectiveApproachStartRange = 1000.0f;
	float EffectiveApproachStopRange = 800.0f;
	float EffectiveAttackCooldownSeconds = 1.5f;
	double LastAttackTimeSeconds = -1000.0;
	double RangedCombatStateEndTimeSeconds = 0.0;
	FVector RangedMoveDirection = FVector::ZeroVector;
	FVector RangedMoveGoal = FVector::ZeroVector;
	ETunaSweeperRangedCombatState RangedCombatState = ETunaSweeperRangedCombatState::Idle;
	bool bIsClosingDistance = false;
	bool bHasRandomizedCombatTuning = false;
};
