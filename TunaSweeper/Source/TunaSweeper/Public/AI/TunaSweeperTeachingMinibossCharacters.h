#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperPatternEnemyCharacter.h"
#include "AI/TunaSweeperRollingRobotMinion.h"
#include "TunaSweeperTeachingMinibossCharacters.generated.h"

/** First lesson: a long, locked warning followed by a slower body charge. */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Teaching Miniboss - Charge"))
class TUNASWEEPER_API ATunaSweeperChargeTeachingMiniboss : public ATunaSweeperPatternEnemyCharacter
{
	GENERATED_BODY()
public:
	ATunaSweeperChargeTeachingMiniboss();
};

/** Second lesson: three vulnerable rolling robots, with time to clear each wave. */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Teaching Miniboss - Robot Carrier"))
class TUNASWEEPER_API ATunaSweeperRobotTeachingMiniboss : public ATunaSweeperPatternEnemyCharacter
{
	GENERATED_BODY()
public:
	ATunaSweeperRobotTeachingMiniboss();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftRobotPod;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightRobotPod;
};

/** Keeps the boss minion's hit rules, with slower rollout and unfolding for the lesson. */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Teaching Rolling Robot Minion"))
class TUNASWEEPER_API ATunaSweeperTeachingRollingRobotMinion : public ATunaSweeperRollingRobotMinion
{
	GENERATED_BODY()
public:
	ATunaSweeperTeachingRollingRobotMinion();
};
