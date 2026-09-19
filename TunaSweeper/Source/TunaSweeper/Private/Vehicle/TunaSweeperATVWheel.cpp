#include "Vehicle/TunaSweeperATVWheel.h"

UTunaSweeperATVFrontWheel::UTunaSweeperATVFrontWheel(const FObjectInitializer& Initializer) : Super(Initializer)
{
	WheelRadius = 33.0f;
	WheelWidth = 23.0f;
	WheelMass = 12.0f;
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 32.0f;
	bAffectedByBrake = true;
	bAffectedByHandbrake = false;
	bAffectedByEngine = true;
	bABSEnabled = true;
	bTractionControlEnabled = true;
	FrictionForceMultiplier = 2.0f;
	CorneringStiffness = 600.0f;
	WheelLoadRatio = 0.65f;
	SuspensionMaxRaise = 8.0f;
	SuspensionMaxDrop = 12.0f;
	SuspensionDampingRatio = 0.65f;
	// UE 5.7 converts this value by 100, then multiplies by travel in cm.
	// 230 produces an effective 23 kN/m spring; 23000 launches the chassis.
	SpringRate = 230.0f;
	SpringPreload = 300.0f;
	RollbarScaling = 0.25f;
	SweepShape = ESweepShape::Spherecast;
	SweepType = ESweepType::SimpleSweep;
	MaxBrakeTorque = 700.0f;
	MaxHandBrakeTorque = 1200.0f;
}

UTunaSweeperATVRearWheel::UTunaSweeperATVRearWheel(const FObjectInitializer& Initializer) : Super(Initializer)
{
	AxleType = EAxleType::Rear;
	bAffectedBySteering = false;
	MaxSteerAngle = 0.0f;
	bAffectedByHandbrake = true;
}
