#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperLootContainerSpawnInteractableActor.generated.h"

class ATunaSweeperLootContainerActor;
class UTunaSweeperInteractableComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLootContainerSpawnInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLootContainerSpawnInteractableActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container Spawn")
	bool SpawnRandomLootContainer(APawn* InstigatorPawn);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	void ConfigureLootContainerSpawnDefaults(TSoftClassPtr<ATunaSweeperLootContainerActor> InLootContainerActorClass);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container Spawn")
	TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinSpawnRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxSpawnRadius = 440.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container Spawn", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnTraceHeight = 800.0f;

private:
	bool PickRandomContainerData(int32& OutContainerDefinitionId, int32& OutContentsId) const;
	FVector BuildRandomSpawnLocation() const;
};

