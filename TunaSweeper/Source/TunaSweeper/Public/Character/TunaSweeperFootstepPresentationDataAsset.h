#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperFootstepPresentationDataAsset.generated.h"

class USoundBase;

/** Shared player-footstep audio presentation data. Surface-specific resolution will extend this asset later. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperFootstepPresentationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Footstep Presentation")
	TSoftObjectPtr<USoundBase> BasicFootstepSound;
};
