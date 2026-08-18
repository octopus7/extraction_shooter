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
	ATunaSweeperLocationBlendCameraActor* Defaults = Blueprint->GeneratedClass
		? Cast<ATunaSweeperLocationBlendCameraActor>(Blueprint->GeneratedClass->GetDefaultObject())
		: nullptr;
	if (!Defaults)
	{
		UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to read location blend camera Blueprint defaults."));
		return false;
	}

	TArray<UCameraComponent*> EmbeddedCameraComponents;
	Defaults->GetComponents<UCameraComponent>(EmbeddedCameraComponents);
	if (!EmbeddedCameraComponents.IsEmpty())
	{
		UE_LOG(
			LogTunaSweeperEditor,
			Error,
			TEXT("Location blend camera Blueprint still contains %d embedded CameraComponent(s)."),
			EmbeddedCameraComponents.Num());
		return false;
	}

	Blueprint->MarkPackageDirty();
	if (!TunaSweeperEditorSetup::SaveAsset(Blueprint))
	{
		UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save location blend camera Blueprint."));
		return false;
	}

	UE_LOG(
		LogTunaSweeperEditor,
		Display,
		TEXT("Created location blend camera Blueprint without embedded cameras: %s/%s"),
		*LocationCameraAssetPath,
		*LocationCameraBlueprintAssetName);
	return true;
}
