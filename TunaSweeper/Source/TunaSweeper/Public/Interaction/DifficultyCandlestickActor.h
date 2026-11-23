#pragma once

#include "Interaction/TunaSweeperInteractableActor.h"
#include "DifficultyCandlestickActor.generated.h"

class UCandlestickFlameComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ADifficultyCandlestickActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ADifficultyCandlestickActor();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCandlestickFlameComponent> Flame;
};
