#include "Subsystem/TunaSweeperRaidArrivalSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

namespace
{
FName NormalizeArrivalLevel(FName Level)
{
    FString ShortName = FPackageName::GetShortName(Level.ToString());
    if (ShortName.StartsWith(TEXT("UEDPIE_")))
    {
        const int32 PrefixEnd = ShortName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 7);
        if (PrefixEnd != INDEX_NONE) ShortName.RightChopInline(PrefixEnd + 1);
    }
    return FName(*ShortName);
}
}

void FTunaSweeperPendingRaidArrival::Prepare(FName Destination, ETunaSweeperRaidArrivalGroup Group)
{
    Reset();
    if (!Destination.IsNone() && Group != ETunaSweeperRaidArrivalGroup::None)
    {
        DestinationName = NormalizeArrivalLevel(Destination);
        ArrivalGroup = Group;
    }
}

ETunaSweeperRaidArrivalGroup FTunaSweeperPendingRaidArrival::Consume(FName LoadedLevel)
{
    const auto Result = !DestinationName.IsNone() && DestinationName == NormalizeArrivalLevel(LoadedLevel)
        ? ArrivalGroup : ETunaSweeperRaidArrivalGroup::None;
    Reset();
    return Result;
}

void FTunaSweeperPendingRaidArrival::Reset()
{
    DestinationName = NAME_None;
    ArrivalGroup = ETunaSweeperRaidArrivalGroup::None;
}

void UTunaSweeperRaidArrivalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PostLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddWeakLambda(this, [this](UWorld* World)
    {
        // Initial player spawning precedes PostLoadMap. Expire unused requests too.
        if (World && World->GetGameInstance() == GetGameInstance())
        {
            Pending.Reset();
            if (auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
                Game->RemoveVolatileItemsOnBaseArrival(NormalizeArrivalLevel(FName(*World->GetMapName())));
        }
    });
    if (GEngine) TravelFailureHandle = GEngine->OnTravelFailure().AddWeakLambda(this,
        [this](UWorld* World, ETravelFailure::Type, const FString&)
        {
            if (World && World->GetGameInstance() == GetGameInstance()) Pending.Reset();
        });
}

void UTunaSweeperRaidArrivalSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadHandle);
    if (GEngine) GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    Pending.Reset();
    Super::Deinitialize();
}

void UTunaSweeperRaidArrivalSubsystem::PrepareArrival(FName Destination, ETunaSweeperRaidArrivalGroup Group)
{
    Pending.Prepare(Destination, Group);
}

ETunaSweeperRaidArrivalGroup UTunaSweeperRaidArrivalSubsystem::ConsumeArrival(UWorld* World)
{
    if (!World || World->GetGameInstance() != GetGameInstance()) return ETunaSweeperRaidArrivalGroup::None;
    return Pending.Consume(FName(*World->GetMapName()));
}

APlayerStart* UTunaSweeperRaidArrivalSubsystem::SelectRandomStart(const TArray<APlayerStart*>& Starts,
    ETunaSweeperRaidArrivalGroup Group, FRandomStream& Random)
{
    if (Group == ETunaSweeperRaidArrivalGroup::None) return nullptr;
    TArray<APlayerStart*> Matches;
    for (APlayerStart* Start : Starts)
    {
        const auto* RaidStart = Cast<ATunaSweeperRaidPlayerStart>(Start);
        if (IsValid(RaidStart) && RaidStart->ArrivalGroup == Group) Matches.Add(Start);
    }
    return Matches.IsEmpty() ? nullptr : Matches[Random.RandRange(0, Matches.Num() - 1)];
}
