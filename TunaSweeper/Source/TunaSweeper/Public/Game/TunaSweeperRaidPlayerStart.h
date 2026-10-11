#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TunaSweeperRaidPlayerStart.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperRaidArrivalGroup : uint8
{
    None,
    Forest,
    VillageEast,
    VillageWest,
    Plains
};

/** Place multiple starts per group to randomize lift arrivals within that group. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRaidPlayerStart : public APlayerStart
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raid Arrival")
    ETunaSweeperRaidArrivalGroup ArrivalGroup = ETunaSweeperRaidArrivalGroup::None;
};
