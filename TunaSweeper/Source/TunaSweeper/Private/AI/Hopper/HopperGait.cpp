#include "AI/Hopper/HopperGait.h"

#include "Math/RotationMatrix.h"

namespace TunaSweeperHopper
{
namespace
{
FVector FiniteOr(const FVector& Value, const FVector& Fallback)
{
	return Value.ContainsNaN() ? Fallback : Value;
}

float NonNegative(float Value, float Fallback)
{
	return FMath::IsFinite(Value) ? FMath::Max(0.f, Value) : Fallback;
}

FVector UnitNormal(const FVector& Normal)
{
	return FiniteOr(Normal, FVector::UpVector).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
}

FQuat FootRotation(const FTransform& Body, const FVector& Normal)
{
	return FRotationMatrix::MakeFromZX(Normal, Body.GetRotation().GetForwardVector()).ToQuat();
}
}

FTwoBoneSolution SolveTwoBone(const FVector& Hip, const FVector& Target,
	const FVector& BendDirection, float UpperLength, float LowerLength)
{
	FTwoBoneSolution Result;
	const FVector Origin = FiniteOr(Hip, FVector::ZeroVector);
	const FVector Requested = FiniteOr(Target, Origin);
	const double Upper = NonNegative(UpperLength, 0.f);
	const double Lower = NonNegative(LowerLength, 0.f);
	const FVector Delta = Requested - Origin;
	const double RequestedDistance = Delta.Size();
	const double Distance = FMath::Clamp(RequestedDistance, FMath::Abs(Upper - Lower), Upper + Lower);
	const FVector Direction = Delta.GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
	FVector Pole = FiniteOr(BendDirection, FVector::ForwardVector);
	Pole = (Pole - Direction * FVector::DotProduct(Pole, Direction)).GetSafeNormal();
	if (Pole.IsNearlyZero())
	{
		const FVector Reference = FMath::Abs(Direction.Z) < .9 ? FVector::UpVector : FVector::ForwardVector;
		Pole = (Reference - Direction * FVector::DotProduct(Reference, Direction)).GetSafeNormal();
	}
	Result.Foot = Origin + Direction * Distance;
	if (Distance <= UE_DOUBLE_SMALL_NUMBER)
	{
		// Equal bones may fold completely; division by a zero hip-to-foot length is unnecessary.
		Result.Knee = Origin + Pole * Upper;
	}
	else
	{
		const double Along = (Upper * Upper - Lower * Lower + Distance * Distance) / (2. * Distance);
		const double Bend = FMath::Sqrt(FMath::Max(0., Upper * Upper - Along * Along));
		Result.Knee = Origin + Direction * Along + Pole * Bend;
	}
	Result.bClamped = !Result.Foot.Equals(Requested, .0001) || Hip.ContainsNaN() || Target.ContainsNaN()
		|| !FMath::IsFinite(UpperLength) || !FMath::IsFinite(LowerLength) || UpperLength < 0 || LowerLength < 0;
	return Result;
}

void FBipedGait::Reset()
{
	const FGaitSettings SavedSettings = Settings;
	*this = FBipedGait();
	Settings = SavedSettings;
}

void FBipedGait::Initialize(const FTransform& Body, const FVector* Targets, const FVector* Normals, bool bGrounded)
{
	SwingFoot = INDEX_NONE;
	NextFoot = 0;
	SwingTime = SupportTime = Settle = 0.f;
	BodyOffset = FVector::ZeroVector;
	PitchLeanDegrees = RollLeanDegrees = 0.f;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Feet[Index].Position = bGrounded ? Targets[Index]
			: Body.TransformPosition(Settings.HipOffsets[Index] + FVector(0, 0, -NonNegative(Settings.MaxLegReach, 160.f) * .75f));
		Feet[Index].Normal = Normals[Index];
		Feet[Index].Rotation = FootRotation(Body, Normals[Index]);
		Feet[Index].bPlanted = bGrounded;
		Feet[Index].bTouchdown = false;
		PreviousTargets[Index] = Targets[Index];
		PreviousNormals[Index] = Normals[Index];
	}
	PreviousBody = Body;
	bInitialized = true;
	bWasGrounded = bGrounded;
}

void FBipedGait::Update(float DeltaTime, const FTransform& Body, const FVector& Velocity,
	const FVector& LeftGroundTarget, const FVector& LeftGroundNormal,
	const FVector& RightGroundTarget, const FVector& RightGroundNormal, bool bGrounded)
{
	Feet[0].bTouchdown = Feet[1].bTouchdown = false;
	if (Body.ContainsNaN()) return;
	const FVector Targets[2] = {FiniteOr(LeftGroundTarget, Body.GetLocation()), FiniteOr(RightGroundTarget, Body.GetLocation())};
	const FVector Normals[2] = {UnitNormal(LeftGroundNormal), UnitNormal(RightGroundNormal)};
	if (!bInitialized)
	{
		Initialize(Body, Targets, Normals, bGrounded);
		return;
	}
	if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) return;
	const bool bTeleported = FVector::DistSquared(Body.GetLocation(), PreviousBody.GetLocation())
		> FMath::Square(FMath::Max(1.f, NonNegative(Settings.TeleportDistance, 200.f)));
	if (bTeleported || (bGrounded && !bWasGrounded))
	{
		Initialize(Body, Targets, Normals, bGrounded);
		return;
	}
	if (!bGrounded)
	{
		// Carry the previous pose with the chassis in flight; traced ground is not a foot anchor.
		const FQuat RotationDelta = Body.GetRotation() * PreviousBody.GetRotation().Inverse();
		for (FGaitFoot& Foot : Feet)
		{
			Foot.Position = Body.TransformPosition(PreviousBody.InverseTransformPosition(Foot.Position));
			Foot.Normal = RotationDelta.RotateVector(Foot.Normal);
			Foot.Rotation = RotationDelta * Foot.Rotation;
			Foot.bPlanted = false;
		}
		SwingFoot = INDEX_NONE;
		BodyOffset = FVector::ZeroVector;
		PitchLeanDegrees = RollLeanDegrees = Settle = 0.f;
	}
	else
	{
		// Bound hitch recovery and use <= 1/120s steps. Interpolated samples keep the stride
		// decision at the same body position at common render rates, including 30 and 120 fps.
		const float Time = FMath::Min(DeltaTime, .25f);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Time * 120.f - .0001f));
		for (int32 Index = 1; Index <= Steps; ++Index)
		{
			const float Alpha = static_cast<float>(Index) / Steps;
			FTransform SampleBody;
			SampleBody.Blend(PreviousBody, Body, Alpha);
			FVector SampleTargets[2], SampleNormals[2];
			for (int32 Foot = 0; Foot < 2; ++Foot)
			{
				SampleTargets[Foot] = FMath::Lerp(PreviousTargets[Foot], Targets[Foot], Alpha);
				SampleNormals[Foot] = UnitNormal(FMath::Lerp(PreviousNormals[Foot], Normals[Foot], Alpha));
			}
			Step(Time / Steps, SampleBody, FiniteOr(Velocity, FVector::ZeroVector), SampleTargets, SampleNormals);
		}
	}
	PreviousBody = Body;
	for (int32 Foot = 0; Foot < 2; ++Foot)
	{
		PreviousTargets[Foot] = Targets[Foot];
		PreviousNormals[Foot] = Normals[Foot];
	}
	bWasGrounded = bGrounded;
}

void FBipedGait::Step(float DeltaTime, const FTransform& Body, const FVector& Velocity,
	const FVector* Targets, const FVector* Normals)
{
	const float Duration = FMath::Max(.05f, NonNegative(Settings.StepDuration, .5f));
	const float Reach = FMath::Max(1.f, NonNegative(Settings.MaxLegReach, 160.f));
	const float Sway = NonNegative(Settings.BodySway, 3.f);
	Settle *= FMath::Exp(-10.f * DeltaTime);
	SupportTime = FMath::Max(0.f, SupportTime - DeltaTime);
	if (SwingFoot == INDEX_NONE && SupportTime <= 0.f)
	{
		bool bNeedsStep = false;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const FVector Hip = Body.TransformPosition(Settings.HipOffsets[Index]);
			const double TurnAngle = FMath::RadiansToDegrees(Feet[Index].Rotation.AngularDistance(FootRotation(Body, Normals[Index])));
			bNeedsStep |= FVector::DistSquared(Feet[Index].Position, Targets[Index]) > FMath::Square(FMath::Max(1.f, NonNegative(Settings.StrideTrigger, 38.f)))
				|| TurnAngle > FMath::Max(1.f, NonNegative(Settings.TurnTriggerDegrees, 25.f))
				|| FVector::DistSquared(Hip, Feet[Index].Position) > FMath::Square(Reach);
		}
		if (bNeedsStep)
		{
			SwingFoot = NextFoot;
			FGaitFoot& Foot = Feet[SwingFoot];
			SwingTime = 0.f;
			SwingStart = Foot.Position;
			SwingStartNormal = Foot.Normal;
			SwingStartRotation = Foot.Rotation;
			SwingEndNormal = Normals[SwingFoot];
			const FVector Prediction = FVector::VectorPlaneProject(Velocity, SwingEndNormal)
				* NonNegative(Settings.PredictionTime, .22f);
			SwingEnd = Targets[SwingFoot] + Prediction.GetClampedToMaxSize(Reach * .4f);
			const FVector Hip = Body.TransformPosition(Settings.HipOffsets[SwingFoot]);
			// Intersect the leg's reach sphere with the sampled ground plane. A radial
			// sphere clamp would lift an unreachable touchdown above the floor.
			const double PlaneHeight = FVector::DotProduct(Hip - Targets[SwingFoot], SwingEndNormal);
			if (FMath::Abs(PlaneHeight) < Reach)
			{
				const FVector PlaneCenter = Hip - SwingEndNormal * PlaneHeight;
				const double PlaneRadius = FMath::Sqrt(FMath::Max(0., Reach * Reach - PlaneHeight * PlaneHeight));
				SwingEnd = PlaneCenter + (SwingEnd - PlaneCenter).GetClampedToMaxSize(PlaneRadius);
			}
			else
			{
				// No grounded point is reachable. Preserve the trace; the caller must lower
				// the chassis or suspend grounding, and SolveTwoBone still cannot stretch.
				SwingEnd = Targets[SwingFoot];
			}
			SwingEndRotation = FootRotation(Body, SwingEndNormal);
			Foot.bPlanted = false;
		}
	}
	FVector DesiredOffset = FVector::ZeroVector;
	if (SwingFoot != INDEX_NONE)
	{
		FGaitFoot& Foot = Feet[SwingFoot];
		SwingTime += DeltaTime;
		const float Phase = FMath::Clamp(SwingTime / Duration, 0.f, 1.f);
		const float Smooth = Phase * Phase * (3.f - 2.f * Phase);
		const float Arc = FMath::Sin(PI * Phase);
		Foot.Position = FMath::Lerp(SwingStart, SwingEnd, Smooth)
			+ FVector::UpVector * (Arc * NonNegative(Settings.StepHeight, 18.f));
		Foot.Normal = UnitNormal(FMath::Lerp(SwingStartNormal, SwingEndNormal, Smooth));
		Foot.Rotation = FQuat::Slerp(SwingStartRotation, SwingEndRotation, Smooth).GetNormalized();
		DesiredOffset.Y = FMath::Sign(Settings.HipOffsets[1-SwingFoot].Y) * Sway * Arc;
		DesiredOffset.Z = -Sway * .5f * Arc;
		if (Phase >= 1.f - UE_KINDA_SMALL_NUMBER)
		{
			Foot.Position = SwingEnd;
			Foot.Normal = SwingEndNormal;
			Foot.Rotation = SwingEndRotation;
			Foot.bPlanted = true;
			Foot.bTouchdown = true;
			NextFoot = 1 - SwingFoot;
			SwingFoot = INDEX_NONE;
			SupportTime = NonNegative(Settings.DoubleSupportDuration, .08f);
			Settle = Sway;
		}
	}
	DesiredOffset.Z -= Settle;
	const float Blend = 1.f - FMath::Exp(-12.f * DeltaTime);
	BodyOffset = FMath::Lerp(BodyOffset, DesiredOffset, Blend);
	const FVector LocalVelocity = Body.InverseTransformVectorNoScale(Velocity);
	const float LeanLimit = NonNegative(Settings.MaxLeanDegrees, 7.f);
	PitchLeanDegrees = FMath::Lerp(PitchLeanDegrees, FMath::Clamp(-static_cast<float>(LocalVelocity.X) * .025f, -LeanLimit, LeanLimit), Blend);
	RollLeanDegrees = FMath::Lerp(RollLeanDegrees, FMath::Clamp(static_cast<float>(LocalVelocity.Y) * .025f, -LeanLimit, LeanLimit), Blend);
}
}
