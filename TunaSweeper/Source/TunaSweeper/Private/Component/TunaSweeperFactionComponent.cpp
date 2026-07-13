#include "Component/TunaSweeperFactionComponent.h"

#include "Engine/World.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"

UTunaSweeperFactionComponent::UTunaSweeperFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTunaSweeperFactionComponent::BeginPlay()
{
	Super::BeginPlay();
	SetFactionId(FactionId);

	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperFactionSubsystem* FactionSubsystem = World->GetSubsystem<UTunaSweeperFactionSubsystem>())
		{
			FactionSubsystem->RegisterFactionActor(GetOwner());
		}
	}
}

void UTunaSweeperFactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperFactionSubsystem* FactionSubsystem = World->GetSubsystem<UTunaSweeperFactionSubsystem>())
		{
			FactionSubsystem->UnregisterFactionActor(GetOwner());
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperFactionComponent::SetFactionId(uint8 InFactionId)
{
	FactionId = TunaSweeperFactionIds::IsValid(InFactionId)
		? InFactionId
		: TunaSweeperFactionIds::NoFaction;
}
