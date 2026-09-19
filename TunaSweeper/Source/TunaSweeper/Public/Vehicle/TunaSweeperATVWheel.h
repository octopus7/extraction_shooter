#pragma once
#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "TunaSweeperATVWheel.generated.h"

UCLASS()
class TUNASWEEPER_API UTunaSweeperATVFrontWheel : public UChaosVehicleWheel
{
	GENERATED_BODY()
public:
	UTunaSweeperATVFrontWheel(const FObjectInitializer& Initializer);
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperATVRearWheel : public UTunaSweeperATVFrontWheel
{
	GENERATED_BODY()
public:
	UTunaSweeperATVRearWheel(const FObjectInitializer& Initializer);
};
