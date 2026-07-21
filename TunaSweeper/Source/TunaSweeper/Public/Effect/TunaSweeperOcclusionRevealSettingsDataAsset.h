#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperOcclusionRevealSettingsDataAsset.generated.h"

/** Shared world-space dimensions for masked occluder reveal materials. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperOcclusionRevealSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Diameter of the fully removed inner area. Default: 4m (2m radius). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float InnerDiameterCm = 400.0f;

	/** Diameter where the dither dissolve reaches the untouched material. Default: 6m (3m radius). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float OuterDiameterCm = 600.0f;

	float GetInnerRadiusCm() const { return FMath::Max(0.0f, InnerDiameterCm * 0.5f); }
	float GetOuterRadiusCm() const { return FMath::Max(GetInnerRadiusCm(), OuterDiameterCm * 0.5f); }
};
