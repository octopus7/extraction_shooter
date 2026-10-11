#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "TunaSweeperVehicleCollisionDamageType.generated.h"

// Vehicle impact producers use this type and the actual driver as instigator.
// Vehicle weapons/explosions must not use it. This introduces no collision damage rules.
UCLASS()
class TUNASWEEPER_API UTunaSweeperVehicleCollisionDamageType : public UDamageType
{
	GENERATED_BODY()
};
