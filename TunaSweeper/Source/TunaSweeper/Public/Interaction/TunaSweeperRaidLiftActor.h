#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperRegionLinkActor.h"
#include "Game/TunaSweeperRaidPlayerStart.h"
#include "TunaSweeperRaidLiftActor.generated.h"
class UTunaSweeperRaidLiftWidget;
class UMaterialInterface;
class UTexture2D;
class UImage;
class UNiagaraSystem;
class UAnimInstance;
class ATunaSweeperPlayerController;
class ATunaSweeperTopDownCharacter;

/** Shared repaired lift; each instance can be placed without a destination-specific Blueprint. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperRaidLiftActor : public ATunaSweeperRegionLinkActor
{
    GENERATED_BODY()
public:
    ATunaSweeperRaidLiftActor();
    virtual bool TravelToTargetLevel(APawn* Pawn) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool SelectArrival(ETunaSweeperRaidArrivalGroup Group);
    void ClosePicker();
    bool IsPawnOnPlatform(const APawn* Pawn) const;
    UTexture2D* GetDestinationImage(int32 Index) const;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift|Cards") TArray<TSoftObjectPtr<UTexture2D>> DestinationImages;
    /** Use TravelInterior as a boarding area on the floor in front of a wall-mounted chair. */
    UPROPERTY(EditAnywhere, Category="Lift|Chair") bool bUseChairBoardingArea = false;
    /** Replaces the rail's StatusLens material only while the lift is repaired. */
    UPROPERTY(EditAnywhere, Category="Lift|Chair") TObjectPtr<UMaterialInterface> ReadyRailStatusMaterial;
    UPROPERTY(EditDefaultsOnly, Category="Lift|Chair") TSoftObjectPtr<UNiagaraSystem> RailContactSpark{FSoftObjectPath(TEXT("/Game/Characters/Hopper/Mk2/FX/NS_HopperMk2SeatLockSparks.NS_HopperMk2SeatLockSparks"))};
protected:
    virtual void ApplyPassagePresentation(bool bUnlocked) override;
    virtual ETunaSweeperRaidArrivalGroup GetArrivalGroup() const override { return SelectedArrival; }
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<UStaticMeshComponent> LeftRail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<UStaticMeshComponent> RightRail;
private:
    bool StartChairRide(ATunaSweeperTopDownCharacter* Character);
    void FinishChairRide(bool bRestoreBoardingState);
    void ReleasePassenger(bool bRestoreInput);
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperRaidLiftWidget> Picker;
    TWeakObjectPtr<APawn> Passenger;
    TWeakObjectPtr<ATunaSweeperPlayerController> PassengerController;
    UPROPERTY(Transient) TObjectPtr<UImage> RideFade;
    UPROPERTY(Transient) TSubclassOf<UAnimInstance> StandingAnimClass;
    FTransform BoardingTransform;
    FVector ParkedChairLocation = FVector::ZeroVector;
    float RideElapsed = 0.f;
    uint8 BoardingMovementMode = 0;
    uint8 BoardingCustomMode = 0;
    bool bBoardingCollision = true;
    ETunaSweeperRaidArrivalGroup SelectedArrival = ETunaSweeperRaidArrivalGroup::None;
};
