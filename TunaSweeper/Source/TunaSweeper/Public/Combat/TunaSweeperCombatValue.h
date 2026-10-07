#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperCombatValue
{
	// Positive half points round up. Keep calculation inputs fractional until this boundary.
	inline float Round(float Value)
	{
		return FMath::IsFinite(Value) ? FMath::RoundToFloat(FMath::Max(0.0f, Value)) : 0.0f;
	}

	inline float RoundDelta(float Delta)
	{
		return Delta < 0.0f ? -Round(-Delta) : Round(Delta);
	}

	inline float ClampGauge(float Value, float Maximum)
	{
		return FMath::Clamp(Round(Value), 0.0f, Round(Maximum));
	}

	inline void DiscardOutwardRemainder(float Value, float Maximum, double& Remainder)
	{
		if ((Value <= 0.0f && Remainder < 0.0) || (Value >= Maximum && Remainder > 0.0)) Remainder = 0.0;
	}

	// Continuous rates accumulate privately; the gameplay gauge itself changes only in whole points.
	inline float Accumulate(float Value, float Maximum, float Delta, double& Remainder)
	{
		Maximum = Round(Maximum);
		Value = ClampGauge(Value, Maximum);
		if (!FMath::IsFinite(Delta)) return Value;
		Remainder += static_cast<double>(Delta);
		// Tolerate only floating-point noise at an exact whole-point boundary.
		const double Whole = FMath::TruncToDouble(Remainder + (Remainder >= 0.0 ? 1.e-6 : -1.e-6));
		Remainder -= Whole;
		if (FMath::Abs(Remainder) < 1.e-6) Remainder = 0.0;
		Value = FMath::Clamp(Value + static_cast<float>(Whole), 0.0f, Maximum);
		DiscardOutwardRemainder(Value, Maximum, Remainder);
		return Value;
	}
}
