#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperRegionLinkActor.h"
#include "Game/TunaSweeperRaidPlayerStart.h"
#include "TunaSweeperRaidLiftActor.generated.h"
class UTunaSweeperRaidLiftWidget;
class UTexture2D;

/** Shared repaired lift; each instance can be placed without a destination-specific Blueprint. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperRaidLiftActor : public ATunaSweeperRegionLinkActor
{
    GENERATED_BODY()
public:
    ATunaSweeperRaidLiftActor();
    virtual bool TravelToTargetLevel(APawn* Pawn) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool SelectArrival(ETunaSweeperRaidArrivalGroup Group);
    void ClosePicker();
    bool IsPawnOnPlatform(const APawn* Pawn) const;
    UTexture2D* GetDestinationImage(int32 Index) const;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift|Cards") TArray<TSoftObjectPtr<UTexture2D>> DestinationImages;
protected:
    virtual ETunaSweeperRaidArrivalGroup GetArrivalGroup() const override { return SelectedArrival; }
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<UStaticMeshComponent> LeftRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<UStaticMeshComponent> RightRail;
private:
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperRaidLiftWidget> Picker;
    TWeakObjectPtr<APawn> Passenger;
    ETunaSweeperRaidArrivalGroup SelectedArrival = ETunaSweeperRaidArrivalGroup::None;
};
