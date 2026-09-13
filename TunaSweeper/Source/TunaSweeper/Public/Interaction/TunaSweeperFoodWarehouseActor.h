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
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UFUNCTION(BlueprintPure, Category="Food Warehouse")
    bool CanTakeFood() const;
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Warehouse|Quest")
    FName RequiredQuestId = TEXT("demo_q4_todays_reward");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Warehouse|Interaction")
    FText InteractionDisplayName = FText::FromString(TEXT("참치캔 획득"));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Warehouse|Interaction")
    FName InteractionDisplayNameStringKey = TEXT("ui.interaction.take_tuna_can");
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Food Warehouse")
    bool bCollected = false;
private:
    void RefreshInteractionState();
};
