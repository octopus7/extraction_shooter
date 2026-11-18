#pragma once

#include "CoreMinimal.h"

class ADecalActor;
class UWorld;

/** Snapshot of the projected layout textures used to generate forest instances in the editor. */
class FORESTSCATTER_API FForestScatterMask
{
public:
	/** Read each selected source texture once on the game thread. An empty selection discovers the forest guide. */
	bool Build(UWorld* World, const TArray<TObjectPtr<ADecalActor>>& ExplicitDecals);

	/** Sample at the landscape surface, including its height inside the decal projection. */
	bool IsGreen(const FVector& WorldPosition) const;

private:
#if WITH_EDITOR
	struct FGuideImage;

	struct FProjection
	{
		FTransform DecalToWorld;
		TSharedPtr<const FGuideImage> Image;
	};

	TArray<FProjection> Projections;
#endif
};
