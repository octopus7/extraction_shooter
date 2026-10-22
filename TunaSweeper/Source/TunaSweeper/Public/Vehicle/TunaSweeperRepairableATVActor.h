#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperPowerFacilityActor.h"
#include "TunaSweeperRepairableATVActor.generated.h"

class ATunaSweeperATVActor;
class USkeletalMeshComponent;

/** Persistent repair scenery; becomes a drivable ATV only on a later map entry after the reward is claimed. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRepairableATVActor : public ATunaSweeperPowerFacilityActor
{
    GENERATED_BODY()

public:
    ATunaSweeperRepairableATVActor();
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repairable ATV|Components")
    TObjectPtr<USkeletalMeshComponent> VehicleMesh;

    // Required independently of the repair interaction gate. Zero leaves the repaired ATV as scenery.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repairable ATV|Replacement", meta = (ClampMin = "0"))
    int32 RequiredRewardQuestId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repairable ATV|Replacement")
    TSubclassOf<ATunaSweeperATVActor> RepairedVehicleClass;
};
