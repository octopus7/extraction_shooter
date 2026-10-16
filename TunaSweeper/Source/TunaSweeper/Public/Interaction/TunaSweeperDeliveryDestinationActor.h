#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperDeliveryDestinationActor.generated.h"

class UTunaSweeperInteractableComponent;
class UStaticMeshComponent;

/** Place at a delivery destination. Item and quantity belong to the quest definition. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperDeliveryDestinationActor : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperDeliveryDestinationActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UFUNCTION(BlueprintPure, Category = "Delivery")
	bool CanDeliver() const;
	UFUNCTION(BlueprintCallable, Category = "Delivery")
	bool Deliver(APawn* Player);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery", meta = (ClampMin = "1"))
	int32 QuestId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FName ObjectiveId;
	// Matches target_provider_id of the item_submitted objective; not the reward NPC.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FName DestinationId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery")
	TObjectPtr<UTunaSweeperInteractableComponent> Interactable;
	// Replace this component's static mesh/material in a BP without changing delivery logic.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery")
	TObjectPtr<UStaticMeshComponent> VisualMesh;
private:
	void RefreshInteraction();
};
