#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TunaSweeperSimulationTestLibrary.generated.h"
class APlayerController;
class APawn;
/** Test setup for character interaction checks in simulate-in-editor sessions. */
UCLASS()
class TUNASWEEPEREDITOR_API UTunaSweeperSimulationTestLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="TunaSweeper|Editor Tests")
 static bool ConfigureSimulationPlayer(APlayerController* Controller, APawn* Pawn);
};
