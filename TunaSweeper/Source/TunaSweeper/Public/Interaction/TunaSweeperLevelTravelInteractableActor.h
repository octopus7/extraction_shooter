#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "Interaction/TunaSweeperLevelTravelPresentationDataAsset.h"
#include "TunaSweeperLevelTravelInteractableActor.generated.h"

class APawn;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLevelTravelInteractableActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLevelTravelInteractableActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Level Travel")
	ETunaSweeperLevelTravelDestination GetDestination() const { return Destination; }

	void SetDestination(ETunaSweeperLevelTravelDestination InDestination) { Destination = InDestination; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Travel")
	bool TravelToTargetLevel(APawn* InstigatorPawn);

protected:
	/** Per-instance selection only; maps and presentation stay centralized elsewhere. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Level Travel")
	ETunaSweeperLevelTravelDestination Destination = ETunaSweeperLevelTravelDestination::Raid;
};
