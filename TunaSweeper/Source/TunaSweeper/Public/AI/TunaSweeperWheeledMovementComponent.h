#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "TunaSweeperWheeledMovementComponent.generated.h"

/** Character navigation/collision with forward steering for the rabbit trike. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperWheeledMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    bool HasSteeringInput() const;
    virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
    virtual void PhysicsRotation(float DeltaTime) override;
    virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal,
        FHitResult& Hit, bool bHandleImpact) override;
};
