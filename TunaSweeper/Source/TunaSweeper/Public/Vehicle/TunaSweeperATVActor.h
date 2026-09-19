#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperATVActor.generated.h"
class UBoxComponent;
class USkeletalMeshComponent;
class UTunaSweeperVehicleMountComponent;

/** Placeable ATV using the shared interaction path. Movement is supplied separately. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperATVActor : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperATVActor();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV")
	TObjectPtr<UBoxComponent> ChassisCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV")
	TObjectPtr<USkeletalMeshComponent> VehicleMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV")
	TObjectPtr<UTunaSweeperVehicleMountComponent> MountComponent;
};
