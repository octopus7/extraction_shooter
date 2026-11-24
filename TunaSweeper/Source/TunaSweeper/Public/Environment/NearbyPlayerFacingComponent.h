#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NearbyPlayerFacingComponent.generated.h"

/** Turns a stationary actor toward the local player and then returns to its placed heading. */
UCLASS(ClassGroup=(Environment), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UNearbyPlayerFacingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UNearbyPlayerFacingComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

    UPROPERTY(EditAnywhere, Category="Facing", meta=(ClampMin="0", Units="cm")) float StartDistance = 400.f;
    UPROPERTY(EditAnywhere, Category="Facing", meta=(ClampMin="0", Units="cm")) float StopDistance = 500.f;
    /** Heading of the model's nose relative to the actor's positive X axis. */
    UPROPERTY(EditAnywhere, Category="Facing", meta=(Units="deg")) float ModelForwardYaw = 0.f;
private:
    float RestYaw = 0.f;
    float ReactionTime = 0.f;
    float ReturnDelay = 0.f;
    bool bTracking = false;
};
