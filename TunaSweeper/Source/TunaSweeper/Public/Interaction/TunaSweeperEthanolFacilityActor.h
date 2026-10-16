#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperPowerFacilityActor.h"
#include "TunaSweeperEthanolFacilityActor.generated.h"

class APawn;

/** A repairable facility that exchanges one carried ingredient batch per interaction. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperEthanolFacilityActor : public ATunaSweeperPowerFacilityActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void RefreshFacilityState() override;

    UFUNCTION(BlueprintCallable, Category = "Ethanol Facility")
    bool Produce(APawn* InstigatorPawn, bool bSaveImmediately = true);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ethanol Facility|Production", meta = (ClampMin = "1"))
    int32 ProductionInputItemId = 3007;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ethanol Facility|Production", meta = (ClampMin = "1"))
    int32 ProductionInputQuantity = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ethanol Facility|Production", meta = (ClampMin = "1"))
    int32 ProductionOutputItemId = 6009;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ethanol Facility|Production", meta = (ClampMin = "1"))
    int32 ProductionOutputQuantity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ethanol Facility|Presentation")
    FName ProductionStringKey = TEXT("ui.ethanol.produce");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ethanol Facility|Presentation")
    FName MissingMaterialStringKey = TEXT("ui.ethanol.no_material");

private:
    bool bProductionInProgress = false;
};
