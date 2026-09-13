#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MoleAssetSetupLibrary.generated.h"
class UBlendSpace;
class UAnimationAsset;
class USkeleton;
UCLASS()
class UMoleAssetSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Mole Import")
	static void FinalizeBlendSpace(UBlendSpace* BlendSpace);
	UFUNCTION(BlueprintCallable, Category="Mole Import")
	static void AssignSkeleton(UAnimationAsset* Asset, USkeleton* Skeleton);
};
