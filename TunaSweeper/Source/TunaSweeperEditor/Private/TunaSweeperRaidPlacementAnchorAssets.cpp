#include "TunaSweeperEditorSetupShared.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Raid/TunaSweeperLootAnchorPreviewDataAsset.h"
#include "Raid/TunaSweeperRaidPlacementAnchor.h"
#include "Engine/StaticMesh.h"

namespace TunaSweeperRaidPlacementAnchorAssets
{
	const FString AssetPath = TEXT("/Game/Raid/Placement");
	const FString PreviewDataAssetName = TEXT("DA_LootAnchorPreviews");
	const FString AnchorBlueprintName = TEXT("BP_RaidPlacementAnchor");

	struct FInitialPreviewMesh
	{
		FName PreviewId;
		const TCHAR* SourceMeshPath;
		const TCHAR* CreatedMeshName;
	};

	const FInitialPreviewMesh InitialPreviewMeshes[] =
	{
		{TEXT("Small"), TEXT("/Game/Meshes/Props/ToolBox/SM_ToolBox.SM_ToolBox"), TEXT("SM_LootAnchorPreview_Small")},
		{TEXT("Medium"), TEXT("/Game/Nature/Wood/SM_CrateA.SM_CrateA"), TEXT("SM_LootAnchorPreview_Medium")},
		{TEXT("Large"), TEXT("/Game/Nature/Wood/SM_CrateB.SM_CrateB"), TEXT("SM_LootAnchorPreview_Large")}
	};

	UStaticMesh* EnsurePreviewMesh(const FInitialPreviewMesh& Definition)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AssetPath, Definition.CreatedMeshName, Definition.CreatedMeshName);
		if (UStaticMesh* Existing = LoadObject<UStaticMesh>(nullptr, *ObjectPath))
		{
			return Existing;
		}

		UStaticMesh* SourceMesh = LoadObject<UStaticMesh>(nullptr, Definition.SourceMeshPath);
		UPackage* Package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *AssetPath, Definition.CreatedMeshName));
		if (!SourceMesh || !Package)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to prepare loot anchor preview mesh %s."), Definition.CreatedMeshName);
			return nullptr;
		}

		UStaticMesh* CreatedMesh = Cast<UStaticMesh>(StaticDuplicateObject(
			SourceMesh,
			Package,
			FName(Definition.CreatedMeshName),
			RF_Public | RF_Standalone | RF_Transactional));
		if (!CreatedMesh)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to duplicate loot anchor preview mesh %s."), Definition.CreatedMeshName);
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(CreatedMesh);
		CreatedMesh->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(CreatedMesh) ? CreatedMesh : nullptr;
	}

	UTunaSweeperLootAnchorPreviewDataAsset* EnsurePreviewDataAsset()
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *PreviewDataAssetName, *PreviewDataAssetName);
		UTunaSweeperLootAnchorPreviewDataAsset* DataAsset = LoadObject<UTunaSweeperLootAnchorPreviewDataAsset>(nullptr, *ObjectPath);
		const bool bCreated = !DataAsset;
		if (bCreated)
		{
			UPackage* Package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *AssetPath, *PreviewDataAssetName));
			DataAsset = Package ? NewObject<UTunaSweeperLootAnchorPreviewDataAsset>(Package, *PreviewDataAssetName, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
			if (!DataAsset)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}
			FAssetRegistryModule::AssetCreated(DataAsset);
		}

		if (bCreated)
		{
			DataAsset->Modify();
			for (const FInitialPreviewMesh& InitialDefinition : InitialPreviewMeshes)
			{
				UStaticMesh* Mesh = EnsurePreviewMesh(InitialDefinition);
				if (!Mesh)
				{
					return nullptr;
				}
				FTunaSweeperLootAnchorPreviewDefinition& Preview = DataAsset->PreviewDefinitions.AddDefaulted_GetRef();
				Preview.PreviewId = InitialDefinition.PreviewId;
				Preview.PreviewMesh = Mesh;
			}
			DataAsset->MarkPackageDirty();
			if (!TunaSweeperEditorSetup::SaveAsset(DataAsset))
			{
				return nullptr;
			}
		}
		return DataAsset;
	}

	UBlueprint* EnsureAnchorBlueprint(UTunaSweeperLootAnchorPreviewDataAsset* PreviewDataAsset)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AnchorBlueprintName, *AnchorBlueprintName);
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath);
		if (!Blueprint)
		{
			UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
			Factory->ParentClass = ATunaSweeperRaidPlacementAnchor::StaticClass();
			Blueprint = Cast<UBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(
				AnchorBlueprintName, AssetPath, UBlueprint::StaticClass(), Factory));
			if (!Blueprint)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}
			FAssetRegistryModule::AssetCreated(Blueprint);
		}
		if (!Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(ATunaSweeperRaidPlacementAnchor::StaticClass()))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not based on ATunaSweeperRaidPlacementAnchor."), *ObjectPath);
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		ATunaSweeperRaidPlacementAnchor* Defaults = Cast<ATunaSweeperRaidPlacementAnchor>(Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr);
		if (!Defaults)
		{
			return nullptr;
		}
		Blueprint->Modify();
		Defaults->Modify();
		Defaults->SetLootPreviewDataAssetForEditor(TSoftObjectPtr<UTunaSweeperLootAnchorPreviewDataAsset>(PreviewDataAsset));
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Blueprint->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Blueprint) ? Blueprint : nullptr;
	}

	bool CreateAssets()
	{
		UTunaSweeperLootAnchorPreviewDataAsset* PreviewDataAsset = EnsurePreviewDataAsset();
		UBlueprint* AnchorBlueprint = PreviewDataAsset ? EnsureAnchorBlueprint(PreviewDataAsset) : nullptr;
		const bool bSucceeded = PreviewDataAsset && AnchorBlueprint;
		if (bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Log, TEXT("Raid placement anchor preview assets created/verified."));
		}
		else
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Raid placement anchor preview assets failed."));
		}
		return bSucceeded;
	}

	void CreateAssetsCommand()
	{
		CreateAssets();
	}

	static FAutoConsoleCommand CreateAssetsConsoleCommand(
		TEXT("TunaSweeper.CreateRaidPlacementAnchorAssets"),
		TEXT("Creates DA_LootAnchorPreviews, three representative meshes, and BP_RaidPlacementAnchor."),
		FConsoleCommandDelegate::CreateStatic(&CreateAssetsCommand));
}
