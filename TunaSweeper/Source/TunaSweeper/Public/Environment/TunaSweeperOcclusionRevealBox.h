#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperOcclusionRevealBox.generated.h"

class UStaticMeshComponent;
class UTunaSweeperOcclusionRevealComponent;

/** Placeable, unspawned-by-default box prototype for the reusable occlusion reveal component. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperOcclusionRevealBox : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperOcclusionRevealBox();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RevealMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperOcclusionRevealComponent> OcclusionRevealComponent;
};
