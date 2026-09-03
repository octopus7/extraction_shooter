#include "TunaSweeperEditorSetupShared.h"

#include "Interaction/TunaSweeperCrowbarWallRackActor.h"

namespace TunaSweeperEditorSetup
{
	bool EnsureCrowbarWallRackBlueprint()
	{
		const FString AssetPath = TEXT("/Game/Interaction");
		const FString AssetName = TEXT("BP_CrowbarWallRack");
		UBlueprint* Blueprint = EnsureBlueprint(
			AssetPath,
			AssetName,
			ATunaSweeperCrowbarWallRackActor::StaticClass());
		if (!Blueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		ATunaSweeperCrowbarWallRackActor* Defaults = Cast<ATunaSweeperCrowbarWallRackActor>(
			Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr);
		if (!Defaults || !Defaults->GetPedestalMeshComponent() || !Defaults->GetCrowbarMeshComponent())
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("BP_CrowbarWallRack is missing required mesh components."));
			return false;
		}

		Blueprint->Modify();
		Defaults->Modify();
		Defaults->GetPedestalMeshComponent()->SetStaticMesh(nullptr);
		Defaults->GetCrowbarMeshComponent()->SetStaticMesh(nullptr);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Blueprint->MarkPackageDirty();

		const bool bSaved = SaveAsset(Blueprint);
		if (bSaved)
		{
			UE_LOG(LogTunaSweeperEditor, Log, TEXT("Crowbar wall rack Blueprint created/verified."));
		}
		else
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Crowbar wall rack Blueprint could not be saved."));
		}
		return bSaved;
	}
}
