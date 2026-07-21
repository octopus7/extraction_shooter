#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperVerticalOcclusionContainer.generated.h"

class UStaticMeshComponent;
class UTunaSweeperVerticalOcclusionRevealComponent;

/** A collision-free U-shaped occluder with a central passage for vertical proximity dissolve testing. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperVerticalOcclusionContainer : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperVerticalOcclusionContainer();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Vertical Occlusion Reveal")
	TObjectPtr<UStaticMeshComponent> LeftPillar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Vertical Occlusion Reveal")
	TObjectPtr<UStaticMeshComponent> RightPillar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Vertical Occlusion Reveal")
	TObjectPtr<UStaticMeshComponent> TopBeam;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Vertical Occlusion Reveal")
	TObjectPtr<UTunaSweeperVerticalOcclusionRevealComponent> VerticalOcclusionRevealComponent;
};
