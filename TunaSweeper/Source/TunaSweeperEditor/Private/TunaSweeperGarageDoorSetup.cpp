#include "TunaSweeperGarageDoorSetup.h"

#include "TunaSweeperEditorSetupShared.h"
#include "FoldingCanopyGarageDoorActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"

namespace
{
	const FString GarageDoorAssetPath = TEXT("/Game/Environment/Bunker/GarageDoor");
	const FString GarageDoorMeshAssetPath = GarageDoorAssetPath + TEXT("/Meshes");
	const FString GarageDoorTextureAssetPath = GarageDoorAssetPath + TEXT("/Textures");
	const FString GarageDoorMaterialAssetPath = GarageDoorAssetPath + TEXT("/Materials");
	const FString GarageDoorBlueprintAssetName = TEXT("BP_RaidBunkerGarageDoor");
	const FString GarageDoorActorLabel = TEXT("TS_RaidBunkerGarageDoor");
	const FString RaidMapPackagePath = TEXT("/Game/RaidMap");

	FString GetSourceArtPath(const FString& RelativePath)
	{
		return FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt/Environment/BunkerGarageDoor"), RelativePath);
	}

	template <typename AssetType>
	AssetType* ImportAsset(const FString& SourceFilename, const FString& DestinationPath)
	{
		const FString AssetName = FPaths::GetBaseFilename(SourceFilename);
		const FString ObjectPath = TunaSweeperEditorSetup::GetAssetObjectPath(DestinationPath, AssetName);
		if (!FPaths::FileExists(SourceFilename))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Garage door import source is missing: %s"), *SourceFilename);
			return nullptr;
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(SourceFilename);
		ImportData->bReplaceExisting = true;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.IsEmpty())
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import garage door source: %s"), *SourceFilename);
			return nullptr;
		}

		AssetType* ImportedAsset = LoadObject<AssetType>(nullptr, *ObjectPath);
		if (!ImportedAsset)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Imported garage door asset could not be loaded: %s"), *ObjectPath);
		}
		return ImportedAsset;
	}

	UMaterial* FindOrCreateMaterial(const FString& AssetName)
	{
		const FString ObjectPath = TunaSweeperEditorSetup::GetAssetObjectPath(GarageDoorMaterialAssetPath, AssetName);
		if (UMaterial* ExistingMaterial = LoadObject<UMaterial>(nullptr, *ObjectPath))
		{
			return ExistingMaterial;
		}

		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		return Cast<UMaterial>(AssetToolsModule.Get().CreateAsset(
			AssetName,
			GarageDoorMaterialAssetPath,
			UMaterial::StaticClass(),
			Factory));
	}

	UMaterial* EnsureGarageDoorMetalMaterial(UTexture2D* MetalTexture)
	{
		if (!MetalTexture)
		{
			return nullptr;
		}

		UMaterial* Material = FindOrCreateMaterial(TEXT("M_GarageDoorMetal"));
		if (!Material)
		{
			return nullptr;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->TwoSided = false;
		Material->SetShadingModel(MSM_DefaultLit);

		UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
		if (!EditorOnlyData)
		{
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinates = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinates->Material = Material;
		TextureCoordinates->CoordinateIndex = 0;
		TextureCoordinates->MaterialExpressionEditorX = -560;
		TextureCoordinates->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(TextureCoordinates);

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->Material = Material;
		TextureSample->ParameterName = TEXT("PartsAtlasBaseColor");
		TextureSample->Texture = MetalTexture;
		TextureSample->Coordinates.Connect(0, TextureCoordinates);
		TextureSample->AutoSetSampleType();
		TextureSample->MaterialExpressionEditorX = -330;
		TextureSample->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(TextureSample);

		EditorOnlyData->BaseColor.Connect(0, TextureSample);
		EditorOnlyData->Metallic.UseConstant = true;
		EditorOnlyData->Metallic.Constant = 0.82f;
		EditorOnlyData->Roughness.UseConstant = true;
		EditorOnlyData->Roughness.Constant = 0.54f;
		EditorOnlyData->Specular.UseConstant = true;
		EditorOnlyData->Specular.Constant = 0.28f;
		EditorOnlyData->EmissiveColor.UseConstant = true;
		EditorOnlyData->EmissiveColor.Constant = FLinearColor::Black;

		Material->PostEditChange();
		Material->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Material) ? Material : nullptr;
	}

	UMaterial* EnsureGarageDoorLedMaterial()
	{
		UMaterial* Material = FindOrCreateMaterial(TEXT("M_GarageDoorLED"));
		if (!Material)
		{
			return nullptr;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->TwoSided = false;
		Material->SetShadingModel(MSM_Unlit);

		UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
		if (!EditorOnlyData)
		{
			return nullptr;
		}

		UMaterialExpressionVectorParameter* LightColor = NewObject<UMaterialExpressionVectorParameter>(Material);
		LightColor->Material = Material;
		LightColor->ParameterName = TEXT("LightColor");
		LightColor->DefaultValue = FLinearColor(1.0f, 0.31f, 0.04f, 1.0f);
		LightColor->MaterialExpressionEditorX = -420;
		LightColor->MaterialExpressionEditorY = -80;
		Material->GetExpressionCollection().AddExpression(LightColor);

		UMaterialExpressionScalarParameter* Intensity = NewObject<UMaterialExpressionScalarParameter>(Material);
		Intensity->Material = Material;
		Intensity->ParameterName = TEXT("Intensity");
		Intensity->DefaultValue = 14.0f;
		Intensity->MaterialExpressionEditorX = -420;
		Intensity->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(Intensity);

		UMaterialExpressionMultiply* Emissive = NewObject<UMaterialExpressionMultiply>(Material);
		Emissive->Material = Material;
		Emissive->A.Connect(0, LightColor);
		Emissive->B.Connect(0, Intensity);
		Emissive->MaterialExpressionEditorX = -150;
		Emissive->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(Emissive);

		EditorOnlyData->BaseColor.Connect(0, LightColor);
		EditorOnlyData->EmissiveColor.Connect(0, Emissive);
		EditorOnlyData->Metallic.UseConstant = true;
		EditorOnlyData->Metallic.Constant = 0.0f;
		EditorOnlyData->Roughness.UseConstant = true;
		EditorOnlyData->Roughness.Constant = 0.24f;

		Material->PostEditChange();
		Material->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Material) ? Material : nullptr;
	}

	bool ConfigureGarageDoorMesh(UStaticMesh* Mesh, UMaterialInterface* Material)
	{
		if (!Mesh || !Material)
		{
			return false;
		}

		Mesh->Modify();
		Mesh->SetMaterial(0, Material);
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Mesh);
	}

	bool CreateAndConfigureGarageDoorBlueprint(
		UStaticMesh* FrameTop,
		UStaticMesh* FrameLeft,
		UStaticMesh* FrameRight,
		UStaticMesh* CanopyRailLeft,
		UStaticMesh* CanopyRailRight,
		UStaticMesh* TemporaryWallLeft,
		UStaticMesh* TemporaryWallRight,
		UStaticMesh* TemporaryRoof,
		UStaticMesh* UpperPanel,
		UStaticMesh* LowerPanel,
		UStaticMesh* LedBar,
		UMaterialInterface* MetalMaterial,
		UMaterialInterface* LedMaterial,
		UBlueprint*& OutBlueprint)
	{
		OutBlueprint = TunaSweeperEditorSetup::EnsureBlueprint(
			GarageDoorAssetPath,
			GarageDoorBlueprintAssetName,
			AFoldingCanopyGarageDoor::StaticClass());
		if (!OutBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(OutBlueprint);
		AFoldingCanopyGarageDoor* Defaults = OutBlueprint->GeneratedClass
			? Cast<AFoldingCanopyGarageDoor>(OutBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to get garage door Blueprint defaults."));
			return false;
		}

		Defaults->Modify();
		Defaults->ConfigureVisualDefaults(
			FrameTop,
			FrameLeft,
			FrameRight,
			CanopyRailLeft,
			CanopyRailRight,
			TemporaryWallLeft,
			TemporaryWallRight,
			TemporaryRoof,
			UpperPanel,
			LowerPanel,
			LedBar,
			MetalMaterial,
			LedMaterial);
		FBlueprintEditorUtils::MarkBlueprintAsModified(OutBlueprint);
		FKismetEditorUtilities::CompileBlueprint(OutBlueprint);
		OutBlueprint->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(OutBlueprint);
	}

	bool PlaceGarageDoorInRaidMap(UBlueprint* GarageDoorBlueprint)
	{
		if (!GarageDoorBlueprint || !GarageDoorBlueprint->GeneratedClass)
		{
			return false;
		}

		UWorld* RaidWorld = TunaSweeperEditorSetup::LoadEditorMapForSetup(RaidMapPackagePath);
		if (!RaidWorld)
		{
			return false;
		}

		// Keep the visual door clear of the RaidMap player start. This is also the
		// canonical placement used whenever the one-shot garage door setup is rerun.
		const FVector DoorLocation(219.9999f, 1474.0253f, 0.0f);
		// FRotator constructor order is Pitch(Y), Yaw(Z), Roll(X). The requested
		// editor Z rotation must therefore be passed as Yaw, not Roll.
		const FRotator DoorRotation(0.0f, -50.0f, 0.0f);
		AActor* GarageDoorActor = TunaSweeperEditorSetup::FindActorByLabel(RaidWorld, GarageDoorActorLabel);
		if (!GarageDoorActor)
		{
			RaidWorld->PersistentLevel->Modify();
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.OverrideLevel = RaidWorld->PersistentLevel;
			SpawnParameters.Name = MakeUniqueObjectName(
				RaidWorld->PersistentLevel,
				GarageDoorBlueprint->GeneratedClass,
				FName(*GarageDoorActorLabel));
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GarageDoorActor = RaidWorld->SpawnActor<AActor>(
				GarageDoorBlueprint->GeneratedClass,
				DoorLocation,
				DoorRotation,
				SpawnParameters);
			if (!GarageDoorActor)
			{
				return false;
			}
			GarageDoorActor->SetActorLabel(GarageDoorActorLabel);
		}

		GarageDoorActor->Modify();
		GarageDoorActor->SetActorLocationAndRotation(DoorLocation, DoorRotation);
		GarageDoorActor->RerunConstructionScripts();
		GarageDoorActor->MarkPackageDirty();
		return UEditorLoadingAndSavingUtils::SaveMap(RaidWorld, RaidMapPackagePath);
	}
}

bool TunaSweeperGarageDoorSetup::Run()
{
	UTexture2D* MetalTexture = ImportAsset<UTexture2D>(
		GetSourceArtPath(TEXT("Textures/T_GarageDoor_PartsAtlas_BaseColor.png")),
		GarageDoorTextureAssetPath);
	if (!MetalTexture)
	{
		return false;
	}

	MetalTexture->Modify();
	MetalTexture->SRGB = true;
	MetalTexture->LODGroup = TEXTUREGROUP_World;
	MetalTexture->PostEditChange();
	MetalTexture->MarkPackageDirty();
	if (!TunaSweeperEditorSetup::SaveAsset(MetalTexture))
	{
		return false;
	}

	UStaticMesh* FrameTop = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_FrameTop.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* FrameLeft = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_FrameLeft.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* FrameRight = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_FrameRight.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* CanopyRailLeft = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_CanopyRailLeft.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* CanopyRailRight = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_CanopyRailRight.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* TemporaryWallLeft = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_TemporaryWallLeft.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* TemporaryWallRight = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_TemporaryWallRight.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* TemporaryRoof = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_TemporaryRoof.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* UpperPanel = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_UpperPanel.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* LowerPanel = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_LowerEmbeddedPanel.obj")), GarageDoorMeshAssetPath);
	UStaticMesh* LedBar = ImportAsset<UStaticMesh>(GetSourceArtPath(TEXT("Models/SM_GarageDoor_LEDBar.obj")), GarageDoorMeshAssetPath);
	if (!FrameTop || !FrameLeft || !FrameRight || !CanopyRailLeft || !CanopyRailRight ||
		!TemporaryWallLeft || !TemporaryWallRight || !TemporaryRoof || !UpperPanel || !LowerPanel || !LedBar)
	{
		return false;
	}

	UMaterial* MetalMaterial = EnsureGarageDoorMetalMaterial(MetalTexture);
	UMaterial* LedMaterial = EnsureGarageDoorLedMaterial();
	if (!MetalMaterial || !LedMaterial ||
		!ConfigureGarageDoorMesh(FrameTop, MetalMaterial) ||
		!ConfigureGarageDoorMesh(FrameLeft, MetalMaterial) ||
		!ConfigureGarageDoorMesh(FrameRight, MetalMaterial) ||
		!ConfigureGarageDoorMesh(CanopyRailLeft, MetalMaterial) ||
		!ConfigureGarageDoorMesh(CanopyRailRight, MetalMaterial) ||
		!ConfigureGarageDoorMesh(TemporaryWallLeft, MetalMaterial) ||
		!ConfigureGarageDoorMesh(TemporaryWallRight, MetalMaterial) ||
		!ConfigureGarageDoorMesh(TemporaryRoof, MetalMaterial) ||
		!ConfigureGarageDoorMesh(UpperPanel, MetalMaterial) ||
		!ConfigureGarageDoorMesh(LowerPanel, MetalMaterial) ||
		!ConfigureGarageDoorMesh(LedBar, LedMaterial))
	{
		return false;
	}

	UBlueprint* GarageDoorBlueprint = nullptr;
	if (!CreateAndConfigureGarageDoorBlueprint(
		FrameTop,
		FrameLeft,
		FrameRight,
		CanopyRailLeft,
		CanopyRailRight,
		TemporaryWallLeft,
		TemporaryWallRight,
		TemporaryRoof,
		UpperPanel,
		LowerPanel,
		LedBar,
		MetalMaterial,
		LedMaterial,
		GarageDoorBlueprint))
	{
		return false;
	}

	const bool bPlaced = PlaceGarageDoorInRaidMap(GarageDoorBlueprint);
	if (bPlaced)
	{
		UE_LOG(LogTunaSweeperEditor, Log, TEXT("Garage door setup completed. Blueprint=%s"), *GetNameSafe(GarageDoorBlueprint));
	}
	else
	{
		UE_LOG(LogTunaSweeperEditor, Error, TEXT("Garage door setup failed during RaidMap placement. Blueprint=%s"), *GetNameSafe(GarageDoorBlueprint));
	}
	return bPlaced;
}
