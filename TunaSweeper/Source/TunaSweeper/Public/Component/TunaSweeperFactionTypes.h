#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperFactionTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperFactionAttitude : uint8
{
	Friendly,
	Neutral,
	Hostile
};

namespace TunaSweeperFactionIds
{
	inline constexpr uint8 Player = 1;
	inline constexpr uint8 Enemy = 10;
	inline constexpr uint8 NoFaction = MAX_uint8;

	inline bool IsValid(uint8 FactionId)
	{
		return FactionId >= 1 && FactionId <= 254;
	}
}
