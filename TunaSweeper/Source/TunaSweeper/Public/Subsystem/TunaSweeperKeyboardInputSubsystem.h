#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperKeyboardInputSubsystem.generated.h"

class APawn;

UCLASS()
class TUNASWEEPER_API UTunaSweeperKeyboardInputSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Input")
	void ReceiveQuickSlotKeyInput(int32 SlotNumber, APawn* InstigatorPawn);
};
