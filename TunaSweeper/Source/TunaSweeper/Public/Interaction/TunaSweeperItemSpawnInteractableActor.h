#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperItemSpawnInteractableActor.generated.h"

class ATunaSweeperPickupItemActor;
class UTunaSweeperInteractableComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperItemSpawnInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperItemSpawnInteractableActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Item Spawn")
	bool SpawnRandomPickupItem(APawn* InstigatorPawn);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	void ConfigureItemSpawnDefaults(TSoftClassPtr<ATunaSweeperPickupItemActor> InPickupItemActorClass);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn")
	TSoftClassPtr<ATunaSweeperPickupItemActor> PickupItemActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinSpawnRadius = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxSpawnRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnTraceHeight = 800.0f;

private:
	bool PickRandomItemDefinition(FTunaSweeperItemDefinition& OutItemDefinition) const;
	FVector BuildRandomSpawnLocation() const;
};
