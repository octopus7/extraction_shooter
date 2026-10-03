#include "TunaRocketFlight.h"
FVector TunaRocketFlight::Turn(const FVector& Forward, const FVector& Desired, float DegreesPerSecond, float DeltaSeconds)
{
    const FVector From = Forward.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
    const FVector To = Desired.GetSafeNormal();
    if (To.IsNearlyZero() || !FMath::IsFinite(DegreesPerSecond) || !FMath::IsFinite(DeltaSeconds)) return From;
    const double Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(From, To), -1.0, 1.0));
    const double Limit = FMath::DegreesToRadians(FMath::Max(0.f, DegreesPerSecond) * FMath::Max(0.f, DeltaSeconds));
    if (Angle <= Limit || Angle < SMALL_NUMBER) return To;
    if (Limit <= 0) return From;
    const FQuat Rotation = FQuat::FindBetweenNormals(From, To);
    return FQuat::Slerp(FQuat::Identity, Rotation, Limit / Angle).RotateVector(From).GetSafeNormal();
}
bool TunaRocketFlight::CanGuide(float Age, float Delay, float Duration, bool bLost, const FVector& Forward, const FVector& ToTarget, float ConeHalfAngle)
{
    return !bLost && Age >= Delay && Age < Delay + Duration && !ToTarget.IsNearlyZero()
        && FVector::DotProduct(Forward.GetSafeNormal(), ToTarget.GetSafeNormal())
            >= FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));
}
float TunaRocketFlight::FiniteClamp(float Value, float Fallback, float Minimum, float Maximum)
{
    return FMath::Clamp(FMath::IsFinite(Value) ? Value : Fallback, Minimum, Maximum);
}
