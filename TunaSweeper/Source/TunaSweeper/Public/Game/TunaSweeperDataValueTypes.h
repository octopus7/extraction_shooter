#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperDataValues
{
	constexpr int32 FixedPointBasis = 10000;
	constexpr int32 ProbabilityMax = FixedPointBasis;
	constexpr int32 RatioIdentity = FixedPointBasis;

	FORCEINLINE int32 ClampProbabilityValue(int32 Value)
	{
		return FMath::Clamp(Value, 0, ProbabilityMax);
	}

	FORCEINLINE int32 ClampRatioValue(int32 Value)
	{
		return FMath::Max(0, Value);
	}

	FORCEINLINE float ToRatioFloat(int32 Value)
	{
		return static_cast<float>(Value) / static_cast<float>(FixedPointBasis);
	}
}
