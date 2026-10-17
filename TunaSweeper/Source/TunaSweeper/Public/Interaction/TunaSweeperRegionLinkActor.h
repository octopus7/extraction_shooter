#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "TunaSweeperRegionLinkActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** A saved passage or repairable lift. The destination is selected through the shared travel resolver. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperRegionLinkActor : public ATunaSweeperLevelTravelInteractableActor
{
    GENERATED_BODY()
public:
    ATunaSweeperRegionLinkActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool TravelToTargetLevel(APawn* Pawn) override;
    virtual ETunaSweeperRaidArrivalGroup GetArrivalGroup() const override { return ArrivalGroup; }
    // Select the destination's dedicated start group; None preserves ordinary passage travel.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Travel")
    ETunaSweeperRaidArrivalGroup ArrivalGroup = ETunaSweeperRaidArrivalGroup::None;
    UFUNCTION(BlueprintPure) bool IsPassageUnlocked() const;
    UFUNCTION(BlueprintCallable) bool RepairPassage(bool bSaveImmediately = true);
    UFUNCTION(BlueprintCallable) void RefreshPassage();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Passage") FName RequiredProgressId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Passage") TArray<FTunaSweeperItemStack> RepairParts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Passage") FName TravelStringKey = TEXT("ui.interaction.travel");

    /** Enable on an enclosed lift; ordinary region passages retain their existing behavior. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lift") bool bUseSlidingDoors = false;
    /** VisualMesh is the replaceable fixed body. These meshes are independent of that body. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<UStaticMeshComponent> LeftDoorMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<UStaticMeshComponent> RightDoorMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<UBoxComponent> TravelInterior;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<USceneComponent> RepairInteractionPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lift") TObjectPtr<USceneComponent> InteriorInteractionPoint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lift", meta=(Units="cm")) FVector LeftDoorOpenOffset = FVector(0,-215,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lift", meta=(Units="cm")) FVector RightDoorOpenOffset = FVector(0,215,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lift", meta=(ClampMin="0.01", Units="s")) float DoorOpenSeconds = 1.0f;
    UPROPERTY(EditAnywhere, Category="Lift|Preview") bool bPreviewDoorsOpen = false;
    UFUNCTION(BlueprintPure, Category="Lift") bool CanUseLiftInterior(const APawn* Pawn) const;
    UFUNCTION(BlueprintPure, Category="Lift") float GetDoorOpenAlpha() const { return DoorOpenAlpha; }
private:
    friend class FTunaSweeperLiftDoorsTest;
    void ApplyDoorPresentation();
    bool IsLocalPlayerInside() const;
    UPROPERTY(VisibleAnywhere, Category="Lift") TObjectPtr<USceneComponent> LeftDoorSlide;
    UPROPERTY(VisibleAnywhere, Category="Lift") TObjectPtr<USceneComponent> RightDoorSlide;
    float DoorOpenAlpha = 0;
    bool bDoorTargetOpen = false;
    bool bInteriorTravelAvailable = false;
    bool bTravelStarted = false;
    bool bRepairing = false;
};
