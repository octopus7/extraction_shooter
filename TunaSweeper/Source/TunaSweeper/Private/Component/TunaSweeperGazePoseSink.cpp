#include "Component/TunaSweeperGazePoseSink.h"

namespace
{
	bool BuildOrthonormalEyeBasis(
		const FVector& EyeAimAxis,
		const FVector& EyeUpAxis,
		FVector& OutAimAxis,
		FVector& OutRightAxis,
		FVector& OutUpAxis)
	{
		OutAimAxis = EyeAimAxis.GetSafeNormal();
		if (OutAimAxis.IsNearlyZero())
		{
			return false;
		}

		OutUpAxis = EyeUpAxis - OutAimAxis * FVector::DotProduct(EyeUpAxis, OutAimAxis);
		OutUpAxis = OutUpAxis.GetSafeNormal();
		if (OutUpAxis.IsNearlyZero())
		{
			return false;
		}

		OutRightAxis = FVector::CrossProduct(OutUpAxis, OutAimAxis).GetSafeNormal();
		return !OutRightAxis.IsNearlyZero();
	}
}

bool TunaSweeperGaze::SolveClampedLookAngles(
	const FQuat& BaseEyeRotation,
	const FVector& EyeAimAxis,
	const FVector& EyeUpAxis,
	const FVector& DesiredDirection,
	float MaxYawDegrees,
	float MaxPitchUpDegrees,
	float MaxPitchDownDegrees,
	float& OutYawDegrees,
	float& OutPitchDegrees)
{
	OutYawDegrees = 0.0f;
	OutPitchDegrees = 0.0f;

	FVector AimAxis;
	FVector RightAxis;
	FVector UpAxis;
	if (!BuildOrthonormalEyeBasis(EyeAimAxis, EyeUpAxis, AimAxis, RightAxis, UpAxis))
	{
		return false;
	}

	const FVector NormalizedDirection = DesiredDirection.GetSafeNormal();
	if (NormalizedDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector LocalDirection = BaseEyeRotation.UnrotateVector(NormalizedDirection);
	const float ForwardAmount = FVector::DotProduct(LocalDirection, AimAxis);
	const float RightAmount = FVector::DotProduct(LocalDirection, RightAxis);
	const float UpAmount = FVector::DotProduct(LocalDirection, UpAxis);

	const float RawYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(RightAmount, ForwardAmount));
	const float HorizontalLength = FMath::Sqrt(
		ForwardAmount * ForwardAmount + RightAmount * RightAmount);
	const float RawPitchDegrees = FMath::RadiansToDegrees(FMath::Atan2(UpAmount, HorizontalLength));

	OutYawDegrees = FMath::Clamp(
		RawYawDegrees,
		-FMath::Max(0.0f, MaxYawDegrees),
		FMath::Max(0.0f, MaxYawDegrees));
	OutPitchDegrees = FMath::Clamp(
		RawPitchDegrees,
		-FMath::Max(0.0f, MaxPitchDownDegrees),
		FMath::Max(0.0f, MaxPitchUpDegrees));
	return true;
}

FQuat TunaSweeperGaze::BuildLookDelta(
	const FQuat& BaseEyeRotation,
	const FVector& EyeAimAxis,
	const FVector& EyeUpAxis,
	float YawDegrees,
	float PitchDegrees)
{
	FVector AimAxis;
	FVector RightAxis;
	FVector UpAxis;
	if (!BuildOrthonormalEyeBasis(EyeAimAxis, EyeUpAxis, AimAxis, RightAxis, UpAxis))
	{
		return FQuat::Identity;
	}

	const float YawRadians = FMath::DegreesToRadians(YawDegrees);
	const float PitchRadians = FMath::DegreesToRadians(PitchDegrees);
	const float CosPitch = FMath::Cos(PitchRadians);
	const FVector DesiredLocalDirection =
		AimAxis * (FMath::Cos(YawRadians) * CosPitch) +
		RightAxis * (FMath::Sin(YawRadians) * CosPitch) +
		UpAxis * FMath::Sin(PitchRadians);
	const FVector CurrentAimDirection = BaseEyeRotation.RotateVector(AimAxis).GetSafeNormal();
	const FVector DesiredAimDirection = BaseEyeRotation.RotateVector(DesiredLocalDirection).GetSafeNormal();
	if (CurrentAimDirection.IsNearlyZero() || DesiredAimDirection.IsNearlyZero())
	{
		return FQuat::Identity;
	}

	return FQuat::FindBetweenNormals(CurrentAimDirection, DesiredAimDirection).GetNormalized();
}

float TunaSweeperGaze::CalculateExponentialInterpolationAlpha(float InterpolationSpeed, float DeltaSeconds)
{
	if (InterpolationSpeed <= 0.0f)
	{
		return 1.0f;
	}
	if (DeltaSeconds <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(1.0f - FMath::Exp(-InterpolationSpeed * DeltaSeconds), 0.0f, 1.0f);
}
