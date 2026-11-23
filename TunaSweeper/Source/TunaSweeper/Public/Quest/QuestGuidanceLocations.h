#pragma once

#include "CoreMinimal.h"

class UWorld;

namespace QuestGuidanceLocations
{
	/** Current map, tracked incomplete objectives, and farming difficulty only. */
	TUNASWEEPER_API void Get(UWorld* World, TArray<FVector>& OutLocations);
}
