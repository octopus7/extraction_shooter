#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "EncyclopediaTerminal.generated.h"

class UEncyclopediaPanelGPT6Astra;

/** Placeable encyclopedia access point. Uses the normal world interaction flow. */
UCLASS(Blueprintable)
class TUNASWEEPER_API AEncyclopediaTerminal : public ATunaSweeperInteractableActor
{
    GENERATED_BODY()
public:
    AEncyclopediaTerminal();
    bool OpenFor(APawn* Player);
    TSubclassOf<UEncyclopediaPanelGPT6Astra> ResolvePanelWidgetClass() const;
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Encyclopedia")
    TSoftClassPtr<UEncyclopediaPanelGPT6Astra> PanelWidgetClass;
};
