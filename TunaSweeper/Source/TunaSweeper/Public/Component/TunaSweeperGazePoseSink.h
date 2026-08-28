#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TunaSweeperGazePoseSink.generated.h"

struct TUNASWEEPER_API FTunaSweeperGazePoseRequest
{
	bool bEnabled = false;
	bool bHasLeftTarget = false;
	bool bHasRightTarget = false;
	FVector LeftTargetWorldLocation = FVector::ZeroVector;
	FVector RightTargetWorldLocation = FVector::ZeroVector;
	FName LeftEyeBoneName = TEXT("cc_base_l_eye");
	FName RightEyeBoneName = TEXT("cc_base_r_eye");
	FVector EyeAimAxis = -FVector::RightVector;
	FVector EyeUpAxis = FVector::UpVector;
	float MaxYawDegrees = 28.0f;
	float MaxPitchUpDegrees = 16.0f;
	float MaxPitchDownDegrees = 12.0f;
	float TrackingInterpolationSpeed = 12.0f;
	float NeutralReturnInterpolationSpeed = 8.0f;
	float MinimumTargetDistance = 1.0f;
	float Weight = 1.0f;
	float DeltaSeconds = 0.0f;
};

namespace TunaSweeperGaze
{
	TUNASWEEPER_API bool SolveClampedLookAngles(
		const FQuat& BaseEyeRotation,
		const FVector& EyeAimAxis,
		const FVector& EyeUpAxis,
		const FVector& DesiredDirection,
		float MaxYawDegrees,
		float MaxPitchUpDegrees,
		float MaxPitchDownDegrees,
		float& OutYawDegrees,
		float& OutPitchDegrees);

	TUNASWEEPER_API FQuat BuildLookDelta(
		const FQuat& BaseEyeRotation,
		const FVector& EyeAimAxis,
		const FVector& EyeUpAxis,
		float YawDegrees,
		float PitchDegrees);

	TUNASWEEPER_API float CalculateExponentialInterpolationAlpha(float InterpolationSpeed, float DeltaSeconds);
}

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTunaSweeperGazePoseSink : public UInterface
{
	GENERATED_BODY()
};

class TUNASWEEPER_API ITunaSweeperGazePoseSink
{
	GENERATED_BODY()

public:
	virtual void SetGazePoseRequest(const FTunaSweeperGazePoseRequest& Request) = 0;
};
