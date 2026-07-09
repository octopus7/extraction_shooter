#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperDifficultyAdjustmentActor.generated.h"

class UTunaSweeperInteractionMarkerWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperDifficultyAdjustmentActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperDifficultyAdjustmentActor();

	void ConfigureDifficultyAdjustmentDefaults(
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		FName InInteractionDisplayNameStringKey);
};
