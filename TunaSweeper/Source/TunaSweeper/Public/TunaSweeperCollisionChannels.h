#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

namespace TunaSweeperCollisionChannels
{
	constexpr ECollisionChannel Projectile = ECC_GameTraceChannel1;
	constexpr ECollisionChannel VisionOccluder = ECC_GameTraceChannel2;
}
