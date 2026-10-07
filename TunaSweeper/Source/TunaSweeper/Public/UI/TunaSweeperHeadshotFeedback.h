#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperHeadshotFeedback
{
	struct FFrame
	{
		FVector2D NumberScale;
		FVector2D BurstScale;
		float NumberAngle = 0;
		float BurstAngle = 0;
		float NumberOpacity = 1;
		float BurstOpacity = 1;
	};

	inline float Phase(float Time, float Start, float End)
	{
		return FMath::Clamp((Time - Start) / (End - Start), 0.0f, 1.0f);
	}

	inline float Smooth(float Alpha) { return Alpha * Alpha * (3.0f - 2.0f * Alpha); }
	inline float Punch(float Alpha) { return 1.0f - FMath::Pow(1.0f - Alpha, 3.0f); }

	// The burst hits first, then squashes as the delayed number springs outward.
	// All phases use elapsed seconds, so low frame rates do not stretch the impact.
	inline FFrame Evaluate(float ElapsedSeconds, float NumberPeak = 3.0f, float NumberSettle = 1.5f)
	{
		const float Time = FMath::Max(0.0f, ElapsedSeconds);
		FFrame Frame;
		if (Time < .028f)
			Frame.BurstScale = FMath::Lerp(FVector2D(.12, .12), FVector2D(1.65, 1.25), Punch(Phase(Time, 0, .028f)));
		else if (Time < .075f)
			Frame.BurstScale = FMath::Lerp(FVector2D(1.65, 1.25), FVector2D(1.32, .72), Smooth(Phase(Time, .028f, .075f)));
		else if (Time < .14f)
			Frame.BurstScale = FMath::Lerp(FVector2D(1.32, .72), FVector2D(1.18, 1.16), Smooth(Phase(Time, .075f, .14f)));
		else
			Frame.BurstScale = FMath::Lerp(FVector2D(1.18, 1.16), FVector2D(1.14, 1.0), Smooth(Phase(Time, .14f, .22f)));
		Frame.BurstAngle = FMath::Lerp(-12.0f, -4.0f, Punch(Phase(Time, 0, .14f)));
		Frame.BurstOpacity = Phase(Time, 0, .008f) * (1.0f - Smooth(Phase(Time, .14f, .38f)));

		const FVector2D Peak(NumberPeak * .92f, NumberPeak * 1.05f);
		const FVector2D Squash(NumberSettle * .90f, NumberSettle * .82f);
		const FVector2D Rebound(NumberSettle * 1.06f, NumberSettle * 1.07f);
		if (Time < .07f)
			Frame.NumberScale = FMath::Lerp(FVector2D(.72, .72), Peak, Punch(Phase(Time, .018f, .07f)));
		else if (Time < .12f)
			Frame.NumberScale = FMath::Lerp(Peak, Squash, Smooth(Phase(Time, .07f, .12f)));
		else if (Time < .19f)
			Frame.NumberScale = FMath::Lerp(Squash, Rebound, Smooth(Phase(Time, .12f, .19f)));
		else
			Frame.NumberScale = FMath::Lerp(Rebound, FVector2D(NumberSettle), Smooth(Phase(Time, .19f, .27f)));
		Frame.NumberAngle = FMath::Sin(FMath::Max(0.0f, Time - .018f) * 48.0f) * 4.5f * FMath::Exp(-Time * 10.0f);
		Frame.NumberOpacity = Phase(Time, .012f, .025f);
		return Frame;
	}
}
