#include "Subsystem/TunaSweeperFactionSubsystem.h"

#include "Component/TunaSweeperFactionComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

void UTunaSweeperFactionSubsystem::RegisterFactionActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	RegisteredFactionActors.RemoveAll([](const TWeakObjectPtr<AActor>& RegisteredActor)
	{
		return !RegisteredActor.IsValid();
	});
	RegisteredFactionActors.AddUnique(Actor);
}

void UTunaSweeperFactionSubsystem::UnregisterFactionActor(AActor* Actor)
{
	RegisteredFactionActors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& RegisteredActor)
	{
		return !RegisteredActor.IsValid() || RegisteredActor.Get() == Actor;
	});
}

uint8 UTunaSweeperFactionSubsystem::GetFactionIdForActor(const AActor* Actor) const
{
	const UTunaSweeperFactionComponent* FactionComponent = ResolveFactionComponent(Actor);
	return FactionComponent ? FactionComponent->GetFactionId() : TunaSweeperFactionIds::NoFaction;
}

ETunaSweeperFactionAttitude UTunaSweeperFactionSubsystem::GetFactionAttitudeById(
	uint8 SourceFactionId,
	uint8 TargetFactionId) const
{
	// Phase 3 faction diplomacy belongs behind this function. Callers must not compare ids directly.
	if (!TunaSweeperFactionIds::IsValid(SourceFactionId) || !TunaSweeperFactionIds::IsValid(TargetFactionId))
	{
		return ETunaSweeperFactionAttitude::Neutral;
	}

	return SourceFactionId == TargetFactionId
		? ETunaSweeperFactionAttitude::Friendly
		: ETunaSweeperFactionAttitude::Hostile;
}

ETunaSweeperFactionAttitude UTunaSweeperFactionSubsystem::GetFactionAttitude(
	const AActor* SourceActor,
	const AActor* TargetActor) const
{
	return GetFactionAttitudeById(
		GetFactionIdForActor(SourceActor),
		GetFactionIdForActor(TargetActor));
}

bool UTunaSweeperFactionSubsystem::AreActorsFriendly(const AActor* SourceActor, const AActor* TargetActor) const
{
	return GetFactionAttitude(SourceActor, TargetActor) == ETunaSweeperFactionAttitude::Friendly;
}

bool UTunaSweeperFactionSubsystem::AreActorsHostile(const AActor* SourceActor, const AActor* TargetActor) const
{
	return GetFactionAttitude(SourceActor, TargetActor) == ETunaSweeperFactionAttitude::Hostile;
}

bool UTunaSweeperFactionSubsystem::CanTargetActor(const AActor* SourceActor, const AActor* TargetActor) const
{
	if (!IsValid(SourceActor) || !IsValid(TargetActor) || SourceActor == TargetActor)
	{
		return false;
	}

	const UTunaSweeperFactionComponent* TargetFactionComponent =
		TargetActor->FindComponentByClass<UTunaSweeperFactionComponent>();
	return TargetFactionComponent &&
		TargetFactionComponent->CanBeCombatTarget() &&
		AreActorsHostile(SourceActor, TargetActor);
}

bool UTunaSweeperFactionSubsystem::CanApplyCombatEffect(const AActor* SourceActor, const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	// Unassigned sources are environmental and retain their existing damage behaviour.
	if (!IsValid(SourceActor))
	{
		return true;
	}

	return !AreActorsFriendly(SourceActor, TargetActor);
}

void UTunaSweeperFactionSubsystem::GetActorsWithAttitude(
	const AActor* SourceActor,
	ETunaSweeperFactionAttitude Attitude,
	TArray<AActor*>& OutActors) const
{
	OutActors.Reset();
	if (!IsValid(SourceActor))
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& RegisteredActor : RegisteredFactionActors)
	{
		AActor* CandidateActor = RegisteredActor.Get();
		if (IsValid(CandidateActor) &&
			CandidateActor != SourceActor &&
			GetFactionAttitude(SourceActor, CandidateActor) == Attitude)
		{
			OutActors.Add(CandidateActor);
		}
	}
}

const UTunaSweeperFactionComponent* UTunaSweeperFactionSubsystem::ResolveFactionComponent(const AActor* Actor) const
{
	TSet<const AActor*> VisitedActors;
	return ResolveFactionComponentRecursive(Actor, VisitedActors);
}

const UTunaSweeperFactionComponent* UTunaSweeperFactionSubsystem::ResolveFactionComponentRecursive(
	const AActor* Actor,
	TSet<const AActor*>& VisitedActors) const
{
	if (!IsValid(Actor) || VisitedActors.Contains(Actor))
	{
		return nullptr;
	}
	VisitedActors.Add(Actor);

	if (const UTunaSweeperFactionComponent* FactionComponent =
		Actor->FindComponentByClass<UTunaSweeperFactionComponent>())
	{
		return FactionComponent;
	}

	if (const AController* Controller = Cast<AController>(Actor))
	{
		if (const UTunaSweeperFactionComponent* PawnFaction =
			ResolveFactionComponentRecursive(Controller->GetPawn(), VisitedActors))
		{
			return PawnFaction;
		}
	}

	if (const UTunaSweeperFactionComponent* InstigatorFaction =
		ResolveFactionComponentRecursive(Actor->GetInstigator(), VisitedActors))
	{
		return InstigatorFaction;
	}

	return ResolveFactionComponentRecursive(Actor->GetOwner(), VisitedActors);
}
