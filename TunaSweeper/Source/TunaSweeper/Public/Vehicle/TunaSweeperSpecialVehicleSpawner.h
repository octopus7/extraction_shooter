#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperSpecialVehicleSpawner.generated.h"

class ATunaSweeperATVActor;
class USceneComponent;

/** A placed Blueprint owns the vehicle class and editor icon; spawning is session-local. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperSpecialVehicleSpawner : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperSpecialVehicleSpawner();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Special Vehicle Spawner")
	TObjectPtr<USceneComponent> SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Vehicle Spawner")
	TSubclassOf<ATunaSweeperATVActor> VehicleClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Special Vehicle Spawner")
	TObjectPtr<ATunaSweeperATVActor> SpawnedVehicle;

	/** Idempotent within one level visit; never spawns in an editor world. */
	UFUNCTION(BlueprintCallable, Category = "Special Vehicle Spawner")
	ATunaSweeperATVActor* SpawnVehicle();

	/** Currently always true. Future quest-dependent BP policy can override this. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Special Vehicle Spawner")
	bool ShouldSpawnVehicle() const;
	virtual bool ShouldSpawnVehicle_Implementation() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	bool bSpawnedThisVisit = false;
};
