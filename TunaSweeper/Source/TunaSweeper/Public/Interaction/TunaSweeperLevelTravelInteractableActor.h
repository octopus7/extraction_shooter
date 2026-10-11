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
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Travel")
	bool bWalkThroughInFullGame = false;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Level Travel")
	ETunaSweeperLevelTravelDestination GetDestination() const { return Destination; }

	void SetDestination(ETunaSweeperLevelTravelDestination InDestination) { Destination = InDestination; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Travel")
	virtual bool TravelToTargetLevel(APawn* InstigatorPawn);

protected:
	bool bWalkTravelStarted = false;
	/** Per-instance selection only; maps and presentation stay centralized elsewhere. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Travel")
	ETunaSweeperLevelTravelDestination Destination = ETunaSweeperLevelTravelDestination::Raid;
};
