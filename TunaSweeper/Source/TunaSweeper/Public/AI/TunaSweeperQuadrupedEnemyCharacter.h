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

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quadruped")
	UQuadrupedComponent* GetQuadrupedComponent() const { return QuadrupedComponent; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void OnDeathPresentationStarted() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UQuadrupedComponent> QuadrupedComponent;

private:
	void EnsureValidQuadrupedLegLayout();
};
