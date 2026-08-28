#pragma once

#include "AI/TunaSweeperEnemyCharacter.h"
#include "TunaSweeperQuadrupedEnemyCharacter.generated.h"

class UQuadrupedComponent;

/**
 * Gun-using TunaSweeper enemy with a procedural quadruped skeletal presentation.
 * Combat, faction, damage, loot, and AI movement remain owned by ATunaSweeperEnemyCharacter.
 */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuadrupedEnemyCharacter : public ATunaSweeperEnemyCharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperQuadrupedEnemyCharacter();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quadruped")
	UQuadrupedComponent* GetQuadrupedComponent() const { return QuadrupedComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UQuadrupedComponent> QuadrupedComponent;
};
