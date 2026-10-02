#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperHopper
{
struct FTwoBoneSolution
{
	FVector Knee = FVector::ZeroVector;
	FVector Foot = FVector::ZeroVector;
	bool bClamped = false;
};

// BendDirection is a direction, not a pole position. Neither bone stretches.
TUNASWEEPER_API FTwoBoneSolution SolveTwoBone(const FVector& Hip, const FVector& Target,
	const FVector& BendDirection, float UpperLength, float LowerLength);

struct FGaitFoot
{
	FVector Position = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;
	FQuat Rotation = FQuat::Identity;
	bool bPlanted = false;
	// A pulse for this Update call, including when it contains several simulation steps.
	bool bTouchdown = false;
};

struct FGaitSettings
{
	float StrideTrigger = 38.f;
	float StepHeight = 18.f;
	float StepDuration = .5f;
	float DoubleSupportDuration = .08f;
	float PredictionTime = .22f;
	float MaxLegReach = 160.f;
	FVector HipOffsets[2] = {FVector(0, -34, 0), FVector(0, 34, 0)};
	float BodySway = 3.f;
	float TeleportDistance = 200.f;
	float TurnTriggerDegrees = 25.f;
	float MaxLeanDegrees = 7.f;
};

// No world dependency. Ground targets are sampled under neutral feet, in world space.
// Body is the unmodified hip-center transform. Apply BodyOffset (body-local) and
// lean to the visual body after updating; do not feed visual sway back into Body.
struct TUNASWEEPER_API FBipedGait
{
	FGaitSettings Settings;
	FGaitFoot Feet[2]; // Left (-Y), right (+Y).
	FVector BodyOffset = FVector::ZeroVector;
	float PitchLeanDegrees = 0.f;
	float RollLeanDegrees = 0.f;

	void Reset();
	void Update(float DeltaTime, const FTransform& Body, const FVector& Velocity,
		const FVector& LeftGroundTarget, const FVector& LeftGroundNormal,
		const FVector& RightGroundTarget, const FVector& RightGroundNormal, bool bGrounded);

private:
	bool bInitialized = false;
	bool bWasGrounded = false;
	FTransform PreviousBody = FTransform::Identity;
	FVector PreviousTargets[2] = {FVector::ZeroVector, FVector::ZeroVector};
	FVector PreviousNormals[2] = {FVector::UpVector, FVector::UpVector};
	int32 SwingFoot = INDEX_NONE;
	int32 NextFoot = 0;
	float SwingTime = 0.f;
	float SupportTime = 0.f;
	float Settle = 0.f;
	FVector SwingStart = FVector::ZeroVector;
	FVector SwingEnd = FVector::ZeroVector;
	FVector SwingStartNormal = FVector::UpVector;
	FVector SwingEndNormal = FVector::UpVector;
	FQuat SwingStartRotation = FQuat::Identity;
	FQuat SwingEndRotation = FQuat::Identity;

	void Initialize(const FTransform& Body, const FVector* Targets, const FVector* Normals, bool bGrounded);
	void Step(float DeltaTime, const FTransform& Body, const FVector& Velocity,
		const FVector* Targets, const FVector* Normals);
};
}
