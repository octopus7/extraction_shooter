#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperLevelTravelInteractableActor.generated.h"

class APawn;
class UTunaSweeperInteractionMarkerWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLevelTravelInteractableActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLevelTravelInteractableActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Level Travel")
	FName GetTargetLevelName() const { return TargetLevelName; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Travel")
	void ConfigureLevelTravelDefaults(
		FName InTargetLevelName,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Level Travel")
	bool TravelToTargetLevel(APawn* InstigatorPawn);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Travel")
	FName TargetLevelName;
};
