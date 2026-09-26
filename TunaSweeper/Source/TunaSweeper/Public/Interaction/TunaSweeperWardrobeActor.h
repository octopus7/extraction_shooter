#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperWardrobeActor.generated.h"

/** Bunker facility that opens the outfit selection panel. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWardrobeActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperWardrobeActor();
};
