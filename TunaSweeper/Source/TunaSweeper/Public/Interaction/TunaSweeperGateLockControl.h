#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperGateLockControl.generated.h"

class ATunaSweeperChainLinkGate;

UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperGateLockControl : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperGateLockControl();
	bool CanOpenGate() const;
	bool OpenGate(APawn* InstigatorPawn);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Gate")
	TObjectPtr<ATunaSweeperChainLinkGate> LinkedGate;
};
