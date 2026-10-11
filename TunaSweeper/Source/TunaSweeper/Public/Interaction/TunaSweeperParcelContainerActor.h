#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "TunaSweeperParcelContainerActor.generated.h"

/** A cardboard loot container that transfers its contents when opened. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperParcelContainerActor : public ATunaSweeperLootContainerActor
{
	GENERATED_BODY()
public:
	ATunaSweeperParcelContainerActor();
	virtual bool CollectContentsOnOpen(UTunaSweeperGameInstance* TunaGameInstance) override;
private:
	bool bCollecting = false;
};
