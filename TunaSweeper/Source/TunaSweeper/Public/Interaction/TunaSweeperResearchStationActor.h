#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperResearchStationActor.generated.h"

class UTunaSweeperInteractionMarkerWidget;

/** Placement-only interaction anchor that opens the ability-stat research tree. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperResearchStationActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperResearchStationActor();

	void ConfigureResearchStationDefaults(
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass);
};
