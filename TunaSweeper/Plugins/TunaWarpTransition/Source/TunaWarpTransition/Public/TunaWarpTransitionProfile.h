#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaWarpTransitionComponent.h"
#include "TunaWarpTransitionProfile.generated.h"

class UMaterialInterface;

/** Reusable project-wide warp presentation settings. */
UCLASS(BlueprintType, DisplayName = "Tuna Warp Transition Profile")
class TUNAWARPTRANSITION_API UTunaWarpTransitionProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UTunaWarpTransitionProfile();

	/** Close/open timing and all visual/light values used by the transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp Transition")
	FWarpTransitionStyle Style;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp Transition|Materials")
	TSoftObjectPtr<UMaterialInterface> WarpMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp Transition|Materials")
	TSoftObjectPtr<UMaterialInterface> ArrivalRimMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp Transition|Input")
	bool bLockMovementInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warp Transition|Input")
	bool bLockLookInput = false;
};
