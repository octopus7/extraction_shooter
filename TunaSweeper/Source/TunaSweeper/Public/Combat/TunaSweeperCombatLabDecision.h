#pragma once
#include "CoreMinimal.h"

/** Local, visible projectile prediction used by the temporary combat-lab pilot. */
namespace TunaSweeperCombatLab
{
inline bool IsIncomingThreat(const FVector& RelativePosition, const FVector& RelativeVelocity,
	float HorizonSeconds = 0.35f, float Clearance = 100.0f)
{
	const FVector P(RelativePosition.X, RelativePosition.Y, 0);
	const FVector V(RelativeVelocity.X, RelativeVelocity.Y, 0);
	const float SpeedSquared = V.SizeSquared();
	if (SpeedSquared < 1.0f || HorizonSeconds <= 0 || Clearance <= 0) return false;
	const float ClosestTime = -FVector::DotProduct(P, V) / SpeedSquared;
	return ClosestTime > 0 && ClosestTime <= HorizonSeconds &&
		(P + V * ClosestTime).SizeSquared() <= FMath::Square(Clearance);
}
}
