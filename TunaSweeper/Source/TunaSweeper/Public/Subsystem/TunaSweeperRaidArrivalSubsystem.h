#pragma once

#include "CoreMinimal.h"
#include "Game/TunaSweeperRaidPlayerStart.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperRaidArrivalSubsystem.generated.h"

/** Transient one-trip request; never serialized into a gameplay save. */
struct TUNASWEEPER_API FTunaSweeperPendingRaidArrival
{
    void Prepare(FName Destination, ETunaSweeperRaidArrivalGroup Group);
    ETunaSweeperRaidArrivalGroup Consume(FName LoadedLevel);
    void Reset();
private:
    FName DestinationName;
    ETunaSweeperRaidArrivalGroup ArrivalGroup = ETunaSweeperRaidArrivalGroup::None;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperRaidArrivalSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void PrepareArrival(FName Destination, ETunaSweeperRaidArrivalGroup Group);
    ETunaSweeperRaidArrivalGroup ConsumeArrival(UWorld* World);
    static APlayerStart* SelectRandomStart(const TArray<APlayerStart*>& Starts,
        ETunaSweeperRaidArrivalGroup Group, FRandomStream& Random);
private:
    FTunaSweeperPendingRaidArrival Pending;
    FDelegateHandle PostLoadHandle;
    FDelegateHandle TravelFailureHandle;
};
