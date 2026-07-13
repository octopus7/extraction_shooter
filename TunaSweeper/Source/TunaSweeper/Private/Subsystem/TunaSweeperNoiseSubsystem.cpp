#include "Subsystem/TunaSweeperNoiseSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"

namespace
{
	constexpr float NoiseFalloffExponent = 1.35f;
}

void UTunaSweeperNoiseSubsystem::ReportNoiseAtLocation(
	const FVector& SourceLocation,
	float Loudness,
	float MaxRange,
	FName NoiseTag,
	AActor* SourceActor,
	AActor* InstigatorActor)
{
	const float SafeLoudness = FMath::Max(0.0f, Loudness);
	const float SafeMaxRange = FMath::Max(0.0f, MaxRange);
	if (SafeLoudness <= 0.0f || SafeMaxRange <= 0.0f)
	{
		return;
	}

	FTunaSweeperNoiseEvent NoiseEvent;
	NoiseEvent.SourceLocation = SourceLocation;
	NoiseEvent.Loudness = SafeLoudness;
	NoiseEvent.MaxRange = SafeMaxRange;
	NoiseEvent.NoiseTag = NoiseTag;
	NoiseEvent.SourceActor = SourceActor;
	NoiseEvent.InstigatorActor = InstigatorActor;

	OnNoiseReported.Broadcast(NoiseEvent);
}

bool UTunaSweeperNoiseSubsystem::CalculateHeardNoiseAtLocation(
	const FTunaSweeperNoiseEvent& NoiseEvent,
	const FVector& ListenerLocation,
	float ListenerHearingRange,
	float ListenerSensitivity,
	float ListenerMinStrength,
	FTunaSweeperHeardNoiseEvent& OutHeardNoise,
	AActor* ListenerActor) const
{
	const AActor* NoiseFactionSource = IsValid(NoiseEvent.InstigatorActor)
		? NoiseEvent.InstigatorActor.Get()
		: NoiseEvent.SourceActor.Get();
	const UTunaSweeperFactionSubsystem* FactionSubsystem = GetWorld()
		? GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>()
		: nullptr;
	if (FactionSubsystem && IsValid(ListenerActor) && IsValid(NoiseFactionSource) &&
		FactionSubsystem->AreActorsFriendly(ListenerActor, NoiseFactionSource))
	{
		return false;
	}

	const float SafeListenerRange = FMath::Max(0.0f, ListenerHearingRange);
	const float SafeNoiseRange = FMath::Max(0.0f, NoiseEvent.MaxRange);
	const float EffectiveRange = SafeListenerRange > 0.0f
		? FMath::Min(SafeListenerRange, SafeNoiseRange)
		: SafeNoiseRange;
	if (EffectiveRange <= 0.0f)
	{
		return false;
	}

	const float Distance = FVector::Dist2D(ListenerLocation, NoiseEvent.SourceLocation);
	if (Distance > EffectiveRange)
	{
		return false;
	}

	const float DistanceAlpha = FMath::Clamp(Distance / EffectiveRange, 0.0f, 1.0f);
	const float Attenuation = FMath::Pow(1.0f - DistanceAlpha, NoiseFalloffExponent);
	const float Strength =
		FMath::Max(0.0f, NoiseEvent.Loudness) *
		FMath::Max(0.0f, ListenerSensitivity) *
		Attenuation;
	if (Strength < FMath::Max(0.0f, ListenerMinStrength))
	{
		return false;
	}

	FVector Direction = NoiseEvent.SourceLocation - ListenerLocation;
	Direction.Z = 0.0f;
	Direction = Direction.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::ForwardVector;
	}

	OutHeardNoise.SourceLocation = NoiseEvent.SourceLocation;
	OutHeardNoise.ListenerLocation = ListenerLocation;
	OutHeardNoise.DirectionFromListener = Direction;
	OutHeardNoise.Distance = Distance;
	OutHeardNoise.Strength = Strength;
	OutHeardNoise.NoiseTag = NoiseEvent.NoiseTag;
	OutHeardNoise.SourceActor = NoiseEvent.SourceActor;
	OutHeardNoise.InstigatorActor = NoiseEvent.InstigatorActor;
	return true;
}
