#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "TunaSweeperPatternEnemyCharacter.generated.h"

/** Placeable example: duplicate as a Blueprint and choose a pattern sequence for bosses or teaching enemies. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPatternEnemyCharacter : public ATunaSweeperEnemyCharacter
{
	GENERATED_BODY()
public:
	ATunaSweeperPatternEnemyCharacter();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ChargeChassis;
};
