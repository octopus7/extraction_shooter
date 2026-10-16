#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperParcelContainerActor.generated.h"

class UTunaSweeperInteractableComponent;
class UStaticMeshComponent;

/** Dedicated one-item pickup. Retains its native name for existing parcel BP references. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperParcelContainerActor : public AActor
{
    GENERATED_BODY()
public:
    ATunaSweeperParcelContainerActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure, Category = "Delivery")
    bool CanPickUp() const;
    UFUNCTION(BlueprintCallable, Category = "Delivery")
    bool PickUp(APawn* Player);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery", meta = (ClampMin = "1"))
    int32 QuestId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
    FName ObjectiveId;
    // Replace the mesh/material in the BP; quest data never overwrites its appearance.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery")
    TObjectPtr<UStaticMeshComponent> VisualMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery")
    TObjectPtr<UTunaSweeperInteractableComponent> Interactable;
private:
    int32 ResolvePickupItemId() const;
    void RefreshPresence();
    // World-lifetime only: a new raid visit permits retry after volatile items are lost.
    bool bCollected = false;
    bool bCollecting = false;
};
