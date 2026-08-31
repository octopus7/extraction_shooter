#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperDebugRifleSupplyActor.generated.h"

class UProceduralMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperDebugRifleSupplyActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperDebugRifleSupplyActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debug Supply")
	bool SupplyRifleAndAmmo(APawn* InstigatorPawn);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> HeartMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Debug Supply", meta = (ClampMin = "1"))
	int32 RifleItemId = 1002;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Debug Supply", meta = (ClampMin = "1"))
	int32 RifleAmmoItemId = 2002;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Debug Supply", meta = (ClampMin = "0"))
	int32 MinimumReserveAmmo = 60;

private:
	void BuildHeartMesh();
	void ApplyVisualMaterials();
};
