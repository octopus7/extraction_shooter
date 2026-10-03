#pragma once
#include "CoreMinimal.h"
namespace TunaRocketFlight
{
    TUNAGUIDEDROCKET_API FVector Turn(const FVector& Forward, const FVector& Desired, float DegreesPerSecond, float DeltaSeconds);
    TUNAGUIDEDROCKET_API bool CanGuide(float Age, float Delay, float Duration, bool bLost, const FVector& Forward, const FVector& ToTarget, float ConeHalfAngle);
    TUNAGUIDEDROCKET_API float FiniteClamp(float Value, float Fallback, float Minimum, float Maximum);
}
