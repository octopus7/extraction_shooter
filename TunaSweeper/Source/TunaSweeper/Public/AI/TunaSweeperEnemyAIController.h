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
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float UpdateInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackRange = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackCooldownSeconds = 1.5f;

private:
	void UpdateAttackTarget();

	FTimerHandle UpdateTimerHandle;
	double LastAttackTimeSeconds = -1000.0;
};
