#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperRaidActorTestActor.generated.h"
/** Native fixture kept separate from the production actor/catalog classes. */
UCLASS(NotBlueprintable, Transient, NotPlaceable)
class ATunaSweeperRaidActorTestActor : public AActor
{
    GENERATED_BODY()
public:
    ATunaSweeperRaidActorTestActor();
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY() int32 ConstructionValue = 0;
    UPROPERTY(EditInstanceOnly) int32 InstanceValue = 1;
    UPROPERTY(EditInstanceOnly) bool bRemoveRootDuringConstruction = false;
    UPROPERTY(EditInstanceOnly) TObjectPtr<AActor> Peer;
    UPROPERTY() bool bObservedBindings = false;
};