#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperTutorialReviewActor.generated.h"

/** Invisible interaction point; the normal localized interaction marker remains visible. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperTutorialReviewActor : public ATunaSweeperInteractableActor
{
    GENERATED_BODY()
public:
    ATunaSweeperTutorialReviewActor();
};
