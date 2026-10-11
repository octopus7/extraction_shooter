#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperQuestRewardLockerActor.generated.h"

/** Receive-only access point for quest rewards that did not fit in the inventory. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestRewardLockerActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()
public:
	ATunaSweeperQuestRewardLockerActor();
};
