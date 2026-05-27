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

UENUM(BlueprintType)
enum class ETunaSweeperHudMode : uint8
{
	None UMETA(DisplayName = "None"),
	Inventory UMETA(DisplayName = "Inventory"),
	Quest UMETA(DisplayName = "Quest"),
	Map UMETA(DisplayName = "Map"),
	Memo UMETA(DisplayName = "Memo")
};
