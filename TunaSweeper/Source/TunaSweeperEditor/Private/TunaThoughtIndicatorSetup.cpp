#include "TunaThoughtIndicatorSetup.h"

#include "TunaSweeperEditorSetupShared.h"

#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "UI/TunaThoughtIndicatorActor.h"

namespace
{
	const FString BlueprintAssetPath = TEXT("/Game/Blueprints/UI");
	const FString BlueprintAssetName = TEXT("BP_TunaThoughtIndicator");
	const TCHAR* TextureObjectPath = TEXT("/Game/UI/Indicators/T_TunaThoughtIndicator.T_TunaThoughtIndicator");

	bool SaveAsset(UObject* Asset)
	{
		UEditorAssetSubsystem* AssetSubsystem = GEditor
			? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>()
			: nullptr;
		return AssetSubsystem && Asset && AssetSubsystem->SaveLoadedAsset(Asset, false);
	}
}

namespace TunaThoughtIndicatorSetup
{
	bool Run()
	{
		UTexture2D* IndicatorTexture = LoadObject<UTexture2D>(nullptr, TextureObjectPath);
		if (!IndicatorTexture)
		{
			UE_LOG(LogTemp, Error, TEXT("Tuna thought indicator setup could not load %s."), TextureObjectPath);
			return false;
		}

		UBlueprint* Blueprint = TunaSweeperEditorSetup::EnsureBlueprint(
			BlueprintAssetPath,
			BlueprintAssetName,
			ATunaThoughtIndicatorActor::StaticClass());
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Tuna thought indicator setup could not create BP_TunaThoughtIndicator."));
			return false;
		}

		const ATunaThoughtIndicatorActor* Defaults =
			Cast<ATunaThoughtIndicatorActor>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!Defaults || !Defaults->GetIndicatorWidgetComponent() || !Defaults->GetIndicatorTexture())
		{
			UE_LOG(LogTemp, Error, TEXT("BP_TunaThoughtIndicator has incomplete native defaults."));
			return false;
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("BP_TunaThoughtIndicator failed to compile."));
			return false;
		}

		Blueprint->MarkPackageDirty();
		if (!SaveAsset(Blueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("BP_TunaThoughtIndicator failed to save."));
			return false;
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("Created /Game/Blueprints/UI/BP_TunaThoughtIndicator with texture %s."),
			TextureObjectPath);
		return true;
	}
}
