#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperHudTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperHudExternalPanelMode : uint8
{
	None UMETA(DisplayName = "None"),
	LootingBox UMETA(DisplayName = "Looting Box"),
	Shop UMETA(DisplayName = "Shop"),
	Storage UMETA(DisplayName = "Storage")
};

