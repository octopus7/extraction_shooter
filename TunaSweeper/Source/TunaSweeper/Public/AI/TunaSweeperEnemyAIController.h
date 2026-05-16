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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float TrackingRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackRange = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float ApproachStartRange = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float ApproachStopRange = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackCooldownSeconds = 1.5f;

private:
	void UpdateAttackTarget();
	void UpdateApproachState(float DistanceToTarget);
	void MoveTowardCurrentTarget(float DeltaSeconds);
	void ClearCombatTarget();

	FTimerHandle UpdateTimerHandle;
	TWeakObjectPtr<AActor> CurrentTargetActor;
	double LastAttackTimeSeconds = -1000.0;
	bool bIsClosingDistance = false;
};
