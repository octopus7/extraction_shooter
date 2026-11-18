#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterfaceExport.h"
#include "HopperOilImpactReceiver.generated.h"

class UMaterialInterface;

/** One collision receiver per oil emitter. A stamp is placed at the exported impact. */
UCLASS()
class UHopperOilImpactReceiver : public UObject, public INiagaraParticleCallbackHandler
{
    GENERATED_BODY()

public:
    void Initialize(AActor* Source, UMaterialInterface* Material, bool bUseLargeSplash, const FVector& TravelDirection);
    virtual void ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* System,
        const FVector& SimulationPositionOffset) override;

private:
    TWeakObjectPtr<AActor> Wreck;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> StampMaterial;
    FVector SprayDirection = FVector::ForwardVector;
    int32 NextVariant = 0;
    bool bLargeSplash = false;
};
