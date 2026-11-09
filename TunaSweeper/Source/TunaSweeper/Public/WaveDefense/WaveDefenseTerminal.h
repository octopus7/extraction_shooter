#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "WaveDefenseTerminal.generated.h"
class AWaveDefenseDirector;
UCLASS(Blueprintable)
class TUNASWEEPER_API AWaveDefenseTerminal : public ATunaSweeperInteractableActor
{
    GENERATED_BODY()
public:
    AWaveDefenseTerminal();
    bool OpenFor(APawn* Player);
protected:
    UPROPERTY(EditInstanceOnly, Category="Wave Defense") TObjectPtr<AWaveDefenseDirector> Director;
};
