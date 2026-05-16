#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TunaSweeperEnemyAIController.generated.h"

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

private:
	void RandomizeCombatTuning();
	void UpdateAttackTarget();
	void UpdateApproachState(float DistanceToTarget);
	void MoveTowardCurrentTarget(float DeltaSeconds);
	void ClearCombatTarget();

	FTimerHandle UpdateTimerHandle;
	TWeakObjectPtr<AActor> CurrentTargetActor;
	float EffectiveUpdateInterval = 0.25f;
	float EffectiveTrackingRange = 1800.0f;
	float EffectiveAttackRange = 1000.0f;
	float EffectiveApproachStartRange = 1000.0f;
	float EffectiveApproachStopRange = 800.0f;
	float EffectiveAttackCooldownSeconds = 1.5f;
	double LastAttackTimeSeconds = -1000.0;
	bool bIsClosingDistance = false;
	bool bHasRandomizedCombatTuning = false;
};
