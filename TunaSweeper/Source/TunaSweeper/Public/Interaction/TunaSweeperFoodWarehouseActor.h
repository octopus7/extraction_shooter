#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperFoodWarehouseActor.generated.h"
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;

UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperFoodWarehouseActor : public AActor
{
    GENERATED_BODY()
public:
    ATunaSweeperFoodWarehouseActor();
    UFUNCTION(BlueprintCallable, Category="Food Warehouse")
    bool TakeFood();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> WarehouseMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UTunaSweeperInteractableComponent> Interactable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Warehouse")
    int32 FoodItemId = 3004;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Warehouse", meta=(ClampMin="1"))
    int32 FoodQuantity = 1;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Food Warehouse")
    bool bCollected = false;
};
