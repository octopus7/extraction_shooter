#include "TunaSweeperLocationBlendCameraSetup.h"

#include "TunaSweeperEditorSetupShared.h"
#include "Camera/TunaSweeperLocationBlendCameraActor.h"

namespace
{
	const FString LocationCameraAssetPath = TEXT("/Game/Camera");
	const FString LocationCameraBlueprintAssetName = TEXT("BP_LocationBlendCamera");
}

bool TunaSweeperLocationBlendCameraSetup::Run()
{
	UBlueprint* Blueprint = TunaSweeperEditorSetup::EnsureBlueprint(
		LocationCameraAssetPath,
		LocationCameraBlueprintAssetName,
		ATunaSweeperLocationBlendCameraActor::StaticClass());
	if (!Blueprint)
	{
		UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create location blend camera Blueprint."));
		return false;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	if (!TunaSweeperEditorSetup::SaveAsset(Blueprint))
	{
		UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save location blend camera Blueprint."));
		return false;
	}

	UE_LOG(
		LogTunaSweeperEditor,
		Display,
		TEXT("Created location blend camera Blueprint: %s/%s"),
		*LocationCameraAssetPath,
		*LocationCameraBlueprintAssetName);
	return true;
}
