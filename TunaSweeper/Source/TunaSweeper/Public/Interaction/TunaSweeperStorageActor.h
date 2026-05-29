#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperStorageActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperStorageActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperStorageActor();
};
