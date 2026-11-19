#include "Environment/CoastalGullProximitySubsystem.h"
#include "Environment/CoastalGull.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UCoastalGullProximitySubsystem::RegisterGull(ACoastalGull* Gull)
{
	Gulls.AddUnique(Gull);
	if (!GetWorld()->GetTimerManager().IsTimerActive(RefreshTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &ThisClass::Refresh, 0.25f, true);
	}
	Refresh();
}

void UCoastalGullProximitySubsystem::UnregisterGull(ACoastalGull* Gull)
{
	Gulls.Remove(Gull);
	if (Gulls.IsEmpty()) GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
}

void UCoastalGullProximitySubsystem::Deinitialize()
{
	GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
	Gulls.Empty();
	Super::Deinitialize();
}

void UCoastalGullProximitySubsystem::Refresh()
{
	TArray<FVector> Locations;
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		const APawn* Pawn = Controller ? Controller->GetPawnOrSpectator() : nullptr;
		if (Pawn) Locations.Add(Pawn->GetActorLocation());
	}
	ApplyPlayerLocations(Locations);
}

void UCoastalGullProximitySubsystem::ApplyPlayerLocations(const TArray<FVector>& Locations)
{
	constexpr double RangeSquared = 10000. * 10000.;
	for (const TWeakObjectPtr<ACoastalGull>& Entry : Gulls)
	{
		if (ACoastalGull* Gull = Entry.Get())
		{
			const FVector Position = Gull->GetActorLocation();
			const bool bActive = Locations.ContainsByPredicate([&](const FVector& Player)
			{
				return FVector::DistSquared(Position, Player) <= RangeSquared;
			});
			Gull->SetActorTickEnabled(bActive);
		}
	}
}
