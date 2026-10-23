#include "AI/TunaSweeperWheeledMovementComponent.h"

#include "AI/TunaSweeperWheeledEnemyCharacter.h"

bool UTunaSweeperWheeledMovementComponent::HasSteeringInput() const
{
    return (bHasRequestedVelocity && !RequestedVelocity.IsNearlyZero()) ||
        !Acceleration.IsNearlyZero() || !GetPendingInputVector().IsNearlyZero();
}

void UTunaSweeperWheeledMovementComponent::CalcVelocity(
    float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
    const auto* Robot = Cast<ATunaSweeperWheeledEnemyCharacter>(CharacterOwner);
    if (!Robot || !Robot->bTricycle || Robot->IsDead() || !HasValidData() ||
        !IsMovingOnGround() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME ||
        CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
    {
        Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
        return;
    }

    // Keep navigation's intended heading: deriving it from the constrained velocity
    // would lock the steering to its own forward axis. Both nav input modes use this.
    const FVector DesiredDirection = (bHasRequestedVelocity ? RequestedVelocity : Acceleration).GetSafeNormal2D();
    float SpeedScale = 1.0f;
    if (!DesiredDirection.IsNearlyZero())
    {
        const FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
        const float NewYaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredDirection.Rotation().Yaw,
            FMath::Max(1.0f, Robot->GetCombatProfile().TurnSpeedDegreesPerSecond) * DeltaTime);
        MoveUpdatedComponent(FVector::ZeroVector, FRotator(0, NewYaw, 0), true);
        const float Alignment = FVector::DotProduct(UpdatedComponent->GetForwardVector(), DesiredDirection);
        // Beyond 60 degrees, brake and turn before accelerating into the path.
        SpeedScale = FMath::Clamp((Alignment - 0.5f) / 0.5f, 0.0f, 1.0f);
    }

    const FVector Forward = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
    const auto ConstrainToWheels = [&Forward](FVector& Value)
    {
        const double Vertical = Value.Z;
        Value = Forward * FVector::DotProduct(Value, Forward);
        Value.Z = Vertical;
    };
    ConstrainToWheels(Velocity);

    // Scope these changes to integration. Path following must still see the normal
    // maximum speed next frame, including when a sharp turn temporarily stops us.
    TGuardValue<FVector> InputGuard(Acceleration, Forward * Acceleration.Size2D() * SpeedScale);
    TGuardValue<FVector> RequestGuard(RequestedVelocity, Forward * RequestedVelocity.Size2D() * SpeedScale);
    TGuardValue<float> SpeedGuard(MaxWalkSpeed, MaxWalkSpeed * SpeedScale);
    TGuardValue<float> CrouchedSpeedGuard(MaxWalkSpeedCrouched, MaxWalkSpeedCrouched * SpeedScale);
    Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
    ConstrainToWheels(Velocity);
}

void UTunaSweeperWheeledMovementComponent::PhysicsRotation(float DeltaTime)
{
    const auto* Robot = Cast<ATunaSweeperWheeledEnemyCharacter>(CharacterOwner);
    if (Robot && Robot->bTricycle && IsMovingOnGround()) return;
    Super::PhysicsRotation(DeltaTime);
}

float UTunaSweeperWheeledMovementComponent::SlideAlongSurface(
    const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact)
{
    const auto* Robot = Cast<ATunaSweeperWheeledEnemyCharacter>(CharacterOwner);
    if (Robot && Robot->bTricycle && IsMovingOnGround() && Hit.bBlockingHit && !IsWalkable(Hit))
    {
        // Walking resolves collision after CalcVelocity. Sliding along an oblique
        // wall here would reintroduce lateral travel; let steering/path recovery
        // clear the obstacle instead. Walkable ramps keep the engine response.
        return 0.0f;
    }
    return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
}
