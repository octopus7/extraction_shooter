#pragma once

#include "Engine/DataAsset.h"
#include "TunaSweeperSupplyDropDataAsset.generated.h"

class USoundBase;
class UStaticMesh;

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperSupplyDropDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supply Drop")
	TObjectPtr<UStaticMesh> ContainerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supply Drop")
	TObjectPtr<UStaticMesh> ParachuteMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supply Drop|Audio")
	TObjectPtr<USoundBase> TransportSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supply Drop|Audio", meta = (ClampMin = "0.0"))
	float TransportVolume = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supply Drop|Audio", meta = (ClampMin = "0.1"))
	float TransportPitch = 0.75f;
};
