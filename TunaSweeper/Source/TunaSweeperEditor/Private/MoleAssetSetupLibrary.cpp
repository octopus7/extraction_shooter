#include "MoleAssetSetupLibrary.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimationAsset.h"
void UMoleAssetSetupLibrary::FinalizeBlendSpace(UBlendSpace* BlendSpace)
{
	check(BlendSpace);
	BlendSpace->ValidateSampleData();
	BlendSpace->ResampleData();
	BlendSpace->PostEditChange();
	BlendSpace->MarkPackageDirty();
}
void UMoleAssetSetupLibrary::AssignSkeleton(UAnimationAsset* Asset, USkeleton* Skeleton)
{
	check(Asset && Skeleton);
	Asset->SetSkeleton(Skeleton);
	Asset->PostEditChange();
	Asset->MarkPackageDirty();
}
