#include "Interaction/TunaSweeperLevelTravelPresentationDataAsset.h"

bool UTunaSweeperLevelTravelPresentationDataAsset::TryGetPresentation(
	ETunaSweeperLevelTravelDestination Destination,
	FTunaSweeperLevelTravelPresentationDefinition& OutDefinition) const
{
	for (const FTunaSweeperLevelTravelPresentationDefinition& Definition : Presentations)
	{
		if (Definition.Destination == Destination)
		{
			OutDefinition = Definition;
			return true;
		}
	}

	OutDefinition = FTunaSweeperLevelTravelPresentationDefinition();
	OutDefinition.Destination = Destination;
	return false;
}
