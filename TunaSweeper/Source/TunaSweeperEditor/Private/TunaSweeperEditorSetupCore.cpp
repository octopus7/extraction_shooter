#include "TunaSweeperEditorSetupShared.h"

#include "Engine/StaticMeshSocket.h"
#include "HAL/IConsoleManager.h"
#include "StaticMeshResources.h"

namespace TunaSweeperEditorSetup
{
	FString GetGameInstanceObjectPath()
	{
		return FString::Printf(TEXT("%s/%s.%s"), *GameInstanceAssetPath, *GameInstanceAssetName, *GameInstanceAssetName);
	}

	FString GetGameInstanceClassPath()
	{
		return FString::Printf(TEXT("%s_C"), *GetGameInstanceObjectPath());
	}

	FString GetAssetObjectPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
	}

	FString GetAssetClassPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s_C"), *GetAssetObjectPath(AssetPath, AssetName));
	}

	bool SaveAsset(UObject* Asset);
	UMaterial* EnsureLedExpressionMaterial();
	UMaterial* EnsureEnemySensorDebugMaterial();

	void AddBoxQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector3f& D,
		const FVector3f& Normal)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVector3f Positions[] = { A, B, C, D };
		const FVector2f UVs[] = {
			FVector2f(0.0f, 0.0f),
			FVector2f(1.0f, 0.0f),
			FVector2f(1.0f, 1.0f),
			FVector2f(0.0f, 1.0f)
		};

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normal;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void AddBoxToMesh(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& Min,
		const FVector3f& Max)
	{
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(0.0f, 0.0f, -1.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(0.0f, 0.0f, 1.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(1.0f, 0.0f, 0.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(-1.0f, 0.0f, 0.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(0.0f, 1.0f, 0.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(0.0f, -1.0f, 0.0f));
	}


#include "EnemyVoxelBodyShape.inl"
#include "EnemyVoxelForwardMarkerShape.inl"
#include "BridgeVoxelBrokenShape.inl"
#include "BridgeVoxelRepairedShape.inl"

	void AddColoredBoxQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector3f& D,
		const FVector3f& Normal,
		const FLinearColor& Color)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();

		const FVector3f Positions[] = { A, B, C, D };
		const FVector2f UVs[] = {
			FVector2f(0.0f, 0.0f),
			FVector2f(1.0f, 0.0f),
			FVector2f(1.0f, 1.0f),
			FVector2f(0.0f, 1.0f)
		};
		const FVector4f VertexColor(Color.R, Color.G, Color.B, Color.A);

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normal;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstanceColors[VertexInstanceId] = VertexColor;
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	FVector3f ConvertVoxelGridPointToMeshPosition(
		int32 X,
		int32 Y,
		int32 Z,
		const FVector3f& Dimensions)
	{
		constexpr float VoxelResolution = 32.0f;
		return FVector3f(
			(static_cast<float>(X) / VoxelResolution - 0.5f) * Dimensions.X,
			(static_cast<float>(Y) / VoxelResolution - 0.5f) * Dimensions.Y,
			(static_cast<float>(Z) / VoxelResolution - 0.5f) * Dimensions.Z);
	}

	void AddVoxelBoxToMesh(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FEnemyVoxelBox& Box,
		const FVector3f& Dimensions)
	{
		const int32 X0 = FMath::Clamp(Box.X0, 0, 32);
		const int32 Y0 = FMath::Clamp(Box.Y0, 0, 32);
		const int32 Z0 = FMath::Clamp(Box.Z0, 0, 32);
		const int32 X1 = FMath::Clamp(Box.X1, 0, 32);
		const int32 Y1 = FMath::Clamp(Box.Y1, 0, 32);
		const int32 Z1 = FMath::Clamp(Box.Z1, 0, 32);
		if (X0 >= X1 || Y0 >= Y1 || Z0 >= Z1)
		{
			return;
		}

		const FVector3f Min = ConvertVoxelGridPointToMeshPosition(X0, Y0, Z0, Dimensions);
		const FVector3f Max = ConvertVoxelGridPointToMeshPosition(X1, Y1, Z1, Dimensions);

		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(0.0f, 0.0f, -1.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(0.0f, 0.0f, 1.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(1.0f, 0.0f, 0.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(-1.0f, 0.0f, 0.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(0.0f, 1.0f, 0.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(0.0f, -1.0f, 0.0f),
			Box.Color);
	}

	void BuildVoxelMeshDescription(
		FMeshDescription& MeshDescription,
		const FName& MaterialSlotName,
		const FVector3f& Dimensions,
		TFunctionRef<void(TArray<FEnemyVoxelBox>&)> AppendBoxes)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = MaterialSlotName;

		TArray<FEnemyVoxelBox> Boxes;
		AppendBoxes(Boxes);
		for (const FEnemyVoxelBox& Box : Boxes)
		{
			AddVoxelBoxToMesh(MeshDescription, Attributes, PolygonGroupId, Box, Dimensions);
		}
	}

	void BuildLevelTravelLadderMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Ladder"));

		constexpr float HalfLength = 130.0f;
		constexpr float RailHalfWidth = 5.0f;
		constexpr float RailCenterOffset = 38.0f;
		constexpr float RungHalfLength = 34.0f;
		constexpr float RungHalfWidth = 5.0f;
		constexpr float Thickness = 8.0f;

		AddBoxToMesh(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(-HalfLength, -RailCenterOffset - RailHalfWidth, 0.0f),
			FVector3f(HalfLength, -RailCenterOffset + RailHalfWidth, Thickness));
		AddBoxToMesh(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(-HalfLength, RailCenterOffset - RailHalfWidth, 0.0f),
			FVector3f(HalfLength, RailCenterOffset + RailHalfWidth, Thickness));

		constexpr int32 RungCount = 7;
		for (int32 RungIndex = 0; RungIndex < RungCount; ++RungIndex)
		{
			const float Alpha = RungCount == 1 ? 0.5f : static_cast<float>(RungIndex) / static_cast<float>(RungCount - 1);
			const float X = FMath::Lerp(-HalfLength + 24.0f, HalfLength - 24.0f, Alpha);
			AddBoxToMesh(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				FVector3f(X - RungHalfWidth, -RungHalfLength, Thickness),
				FVector3f(X + RungHalfWidth, RungHalfLength, Thickness * 2.0f));
		}
	}

	UStaticMesh* EnsureLevelTravelLadderMeshAsset()
	{
		const FString AssetObjectPath = GetAssetObjectPath(InteractionAssetPath, LevelTravelLadderMeshAssetName);
		if (UStaticMesh* ExistingMesh = LoadObject<UStaticMesh>(nullptr, *AssetObjectPath))
		{
			return ExistingMesh;
		}

		const FString PackageName = FString::Printf(TEXT("%s/%s"), *InteractionAssetPath, *LevelTravelLadderMeshAssetName);
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		UStaticMesh* StaticMesh = NewObject<UStaticMesh>(
			Package,
			*LevelTravelLadderMeshAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!StaticMesh)
		{
			return nullptr;
		}

		FMeshDescription MeshDescription;
		BuildLevelTravelLadderMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Add(FStaticMaterial());

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		FAssetRegistryModule::AssetCreated(StaticMesh);
		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}

	bool SetProjectGameInstanceToBlueprint()
	{
		UGameMapsSettings* GameMapsSettings = GetMutableDefault<UGameMapsSettings>();
		if (!GameMapsSettings)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Could not load GameMapsSettings."));
			return false;
		}

		const FSoftClassPath GameInstanceClassPath(GetGameInstanceClassPath());
		if (GameMapsSettings->GameInstanceClass.ToString() != GameInstanceClassPath.ToString())
		{
			GameMapsSettings->GameInstanceClass = GameInstanceClassPath;
			GameMapsSettings->SaveConfig();
		}

		const FString DefaultEngineIni = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameInstanceClass"),
			*GameInstanceClassPath.ToString(),
			DefaultEngineIni);
		GConfig->Flush(false, DefaultEngineIni);

		FString SavedGameInstanceClass;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameInstanceClass"),
			SavedGameInstanceClass,
			DefaultEngineIni);

		return SavedGameInstanceClass == GameInstanceClassPath.ToString();
	}

	UBlueprint* EnsureBlueprint(const FString& AssetPath, const FString& AssetName, UClass* ParentClass)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		if (UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath))
		{
			if (!ExistingBlueprint->ParentClass || !ExistingBlueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but it is not based on %s."), *ObjectPath, *GetNameSafe(ParentClass));
				return nullptr;
			}

			if (!ExistingBlueprint->GeneratedClass)
			{
				FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			}

			return ExistingBlueprint;
		}

		UBlueprintFactory* BlueprintFactory = NewObject<UBlueprintFactory>();
		BlueprintFactory->ParentClass = ParentClass;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			AssetName,
			AssetPath,
			UBlueprint::StaticClass(),
			BlueprintFactory);

		UBlueprint* CreatedBlueprint = Cast<UBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(CreatedBlueprint);
		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();

		if (!SaveAsset(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return CreatedBlueprint;
	}

	UMaterial* EnsureSolidColorMaterial(
		const FString& AssetPath,
		const FString& AssetName,
		const FLinearColor& BaseColor,
		float Roughness = 0.65f,
		float Metallic = 0.0f,
		float Specular = 0.25f)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				AssetName,
				AssetPath,
				UMaterial::StaticClass(),
				MaterialFactory);

			Material = Cast<UMaterial>(CreatedAsset);
			if (!Material)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionConstant3Vector* BaseColorExpression = NewObject<UMaterialExpressionConstant3Vector>(Material);
		BaseColorExpression->Material = Material;
		BaseColorExpression->Constant = BaseColor;
		BaseColorExpression->MaterialExpressionEditorX = -240;
		BaseColorExpression->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(BaseColorExpression);
		MaterialEditorOnly->BaseColor.Connect(0, BaseColorExpression);

		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = Roughness;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = Metallic;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = Specular;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UMaterial* EnsureVoxelVertexColorMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(VoxelAssetPath, VoxelVertexColorMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				VoxelVertexColorMaterialAssetName,
				VoxelAssetPath,
				UMaterial::StaticClass(),
				MaterialFactory);

			Material = Cast<UMaterial>(CreatedAsset);
			if (!Material)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -240;
		VertexColorExpression->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);
		MaterialEditorOnly->BaseColor.Connect(0, VertexColorExpression);

		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.8f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.2f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureVoxelStaticMeshAsset(
		const FString& AssetPath,
		const FString& AssetName,
		const FName& MaterialSlotName,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription,
		UMaterialInterface* VoxelMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *AssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FMeshDescription MeshDescription;
			BuildMeshDescription(MeshDescription);

			StaticMesh->GetStaticMaterials().Reset();
			StaticMesh->GetStaticMaterials().Add(FStaticMaterial(VoxelMaterial, MaterialSlotName));

			TArray<const FMeshDescription*> MeshDescriptions;
			MeshDescriptions.Add(&MeshDescription);
			StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
			FAssetRegistryModule::AssetCreated(StaticMesh);
		}
		else
		{
			StaticMesh->Modify();
			if (StaticMesh->GetStaticMaterials().Num() == 0)
			{
				StaticMesh->GetStaticMaterials().Add(FStaticMaterial(VoxelMaterial, MaterialSlotName));
			}
			else
			{
				StaticMesh->GetStaticMaterials()[0] = FStaticMaterial(VoxelMaterial, MaterialSlotName);
			}
		}

		StaticMesh->MarkPackageDirty();
		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	float ComputeSwingArcVertexAlpha(float U, float V)
	{
		const float AlongArcFade = FMath::Pow(FMath::Clamp(FMath::Sin(U * PI), 0.0f, 1.0f), 0.55f);
		const float WidthFade = FMath::Lerp(0.38f, 1.0f, FMath::Clamp(V, 0.0f, 1.0f));
		return FMath::Clamp(AlongArcFade * WidthFade, 0.0f, 1.0f);
	}

	FVertexInstanceID AddSwingArcVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector2f& UV,
		const FLinearColor& Color)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();

		const FVertexID VertexId = MeshDescription.CreateVertex();
		VertexPositions[VertexId] = Position;

		const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
		VertexInstanceNormals[VertexInstanceId] = FVector3f(0.0f, 0.0f, 1.0f);
		VertexInstanceUVs.Set(VertexInstanceId, 0, UV);
		VertexInstanceColors[VertexInstanceId] = FVector4f(Color.R, Color.G, Color.B, Color.A);
		return VertexInstanceId;
	}

	void AddSwingArcQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& InnerA,
		const FVector3f& OuterA,
		const FVector3f& OuterB,
		const FVector3f& InnerB,
		float U0,
		float U1)
	{
		const FLinearColor InnerColorA(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U0, 0.0f));
		const FLinearColor OuterColorA(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U0, 1.0f));
		const FLinearColor OuterColorB(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U1, 1.0f));
		const FLinearColor InnerColorB(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U1, 0.0f));

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(4);
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, InnerA, FVector2f(U0, 0.0f), InnerColorA));
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, OuterA, FVector2f(U0, 1.0f), OuterColorA));
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, OuterB, FVector2f(U1, 1.0f), OuterColorB));
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, InnerB, FVector2f(U1, 0.0f), InnerColorB));
		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void BuildLumberjackMeleeSwingArcMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		constexpr int32 SegmentCount = 18;
		constexpr float StartDegrees = -66.0f;
		constexpr float EndDegrees = 66.0f;
		constexpr float InnerRadius = 48.0f;
		constexpr float OuterRadius = 118.0f;
		constexpr float Height = 64.0f;
		const FVector2f ArcCenter(18.0f, 0.0f);

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = FMath::DegreesToRadians(FMath::Lerp(StartDegrees, EndDegrees, U0));
			const float Angle1 = FMath::DegreesToRadians(FMath::Lerp(StartDegrees, EndDegrees, U1));
			const FVector2f Direction0(FMath::Cos(Angle0), FMath::Sin(Angle0));
			const FVector2f Direction1(FMath::Cos(Angle1), FMath::Sin(Angle1));

			const FVector2f Inner0 = ArcCenter + Direction0 * InnerRadius;
			const FVector2f Outer0 = ArcCenter + Direction0 * OuterRadius;
			const FVector2f Outer1 = ArcCenter + Direction1 * OuterRadius;
			const FVector2f Inner1 = ArcCenter + Direction1 * InnerRadius;

			AddSwingArcQuad(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				FVector3f(Inner0.X, Inner0.Y, Height),
				FVector3f(Outer0.X, Outer0.Y, Height),
				FVector3f(Outer1.X, Outer1.Y, Height),
				FVector3f(Inner1.X, Inner1.Y, Height),
				U0,
				U1);
		}
	}

	UMaterial* EnsureLumberjackMeleeSwingArcMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LumberjackMeleeSwingArcMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				LumberjackMeleeSwingArcMaterialAssetName,
				EffectsAssetPath,
				UMaterial::StaticClass(),
				MaterialFactory);

			Material = Cast<UMaterial>(CreatedAsset);
			if (!Material)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendableLocation = BL_SceneColorAfterTonemapping;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = false;
		Material->bUsedWithNiagaraMeshParticles = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -520;
		VertexColorExpression->MaterialExpressionEditorY = -20;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionScalarParameter* IntensityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		IntensityParameter->Material = Material;
		IntensityParameter->ParameterName = TEXT("Intensity");
		IntensityParameter->DefaultValue = 5.2f;
		IntensityParameter->MaterialExpressionEditorX = -520;
		IntensityParameter->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(IntensityParameter);

		UMaterialExpressionMultiply* EmissiveMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMultiply->Material = Material;
		EmissiveMultiply->A.Connect(0, VertexColorExpression);
		EmissiveMultiply->B.Connect(0, IntensityParameter);
		EmissiveMultiply->MaterialExpressionEditorX = -220;
		EmissiveMultiply->MaterialExpressionEditorY = 60;
		Material->GetExpressionCollection().AddExpression(EmissiveMultiply);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 1.0f;
		OpacityParameter->MaterialExpressionEditorX = -520;
		OpacityParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionScalarParameter* DissolveParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		DissolveParameter->Material = Material;
		DissolveParameter->ParameterName = TEXT("Dissolve");
		DissolveParameter->DefaultValue = 0.0f;
		DissolveParameter->MaterialExpressionEditorX = -520;
		DissolveParameter->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(DissolveParameter);

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -820;
		TextureCoordinateExpression->MaterialExpressionEditorY = 600;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionComponentMask* UvUMask = NewObject<UMaterialExpressionComponentMask>(Material);
		UvUMask->Material = Material;
		UvUMask->Input.Connect(0, TextureCoordinateExpression);
		UvUMask->R = 1;
		UvUMask->G = 0;
		UvUMask->B = 0;
		UvUMask->A = 0;
		UvUMask->MaterialExpressionEditorX = -640;
		UvUMask->MaterialExpressionEditorY = 600;
		Material->GetExpressionCollection().AddExpression(UvUMask);

		UMaterialExpressionSubtract* UvDissolveSubtract = NewObject<UMaterialExpressionSubtract>(Material);
		UvDissolveSubtract->Material = Material;
		UvDissolveSubtract->A.Connect(0, UvUMask);
		UvDissolveSubtract->B.Connect(0, DissolveParameter);
		UvDissolveSubtract->MaterialExpressionEditorX = -340;
		UvDissolveSubtract->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(UvDissolveSubtract);

		UMaterialExpressionMultiply* UvWipeScale = NewObject<UMaterialExpressionMultiply>(Material);
		UvWipeScale->Material = Material;
		UvWipeScale->A.Connect(0, UvDissolveSubtract);
		UvWipeScale->ConstB = 6.0f;
		UvWipeScale->MaterialExpressionEditorX = -140;
		UvWipeScale->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(UvWipeScale);

		UMaterialExpressionSaturate* UvWipeMask = NewObject<UMaterialExpressionSaturate>(Material);
		UvWipeMask->Material = Material;
		UvWipeMask->Input.Connect(0, UvWipeScale);
		UvWipeMask->MaterialExpressionEditorX = 60;
		UvWipeMask->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(UvWipeMask);

		UMaterialExpressionMultiply* VertexOpacityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		VertexOpacityMultiply->Material = Material;
		VertexOpacityMultiply->A.Connect(4, VertexColorExpression);
		VertexOpacityMultiply->B.Connect(0, OpacityParameter);
		VertexOpacityMultiply->MaterialExpressionEditorX = -220;
		VertexOpacityMultiply->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(VertexOpacityMultiply);

		UMaterialExpressionMultiply* FinalOpacityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacityMultiply->Material = Material;
		FinalOpacityMultiply->A.Connect(0, VertexOpacityMultiply);
		FinalOpacityMultiply->B.Connect(0, UvWipeMask);
		FinalOpacityMultiply->MaterialExpressionEditorX = 240;
		FinalOpacityMultiply->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(FinalOpacityMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, VertexColorExpression);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMultiply);
		MaterialEditorOnly->Opacity.Connect(0, FinalOpacityMultiply);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureLumberjackMeleeSwingArcMeshAsset(UMaterialInterface* SwingArcMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LumberjackMeleeSwingArcMeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *EffectsAssetPath, *LumberjackMeleeSwingArcMeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*LumberjackMeleeSwingArcMeshAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();

		FMeshDescription MeshDescription;
		BuildLumberjackMeleeSwingArcMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(SwingArcMaterial, FName(TEXT("SwingArc"))));

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureLumberjackMeleeSwingArcAssets()
	{
		UMaterial* SwingArcMaterial = EnsureLumberjackMeleeSwingArcMaterial();
		if (!SwingArcMaterial)
		{
			return false;
		}

		return EnsureLumberjackMeleeSwingArcMeshAsset(SwingArcMaterial) != nullptr;
	}


	FVertexInstanceID AddBaseballBatVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector3f& Normal,
		const FVector2f& UV)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVertexID VertexId = MeshDescription.CreateVertex();
		VertexPositions[VertexId] = Position;

		const FVector3f SafeNormal = Normal.IsNearlyZero() ? FVector3f(0.0f, 0.0f, 1.0f) : Normal.GetSafeNormal();
		const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
		VertexInstanceNormals[VertexInstanceId] = SafeNormal;
		VertexInstanceUVs.Set(VertexInstanceId, 0, UV);
		return VertexInstanceId;
	}

	void AddBaseballBatSideQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FBaseballBatRing& LeftRing,
		const FBaseballBatRing& RightRing,
		float Angle0,
		float Angle1,
		float U0,
		float U1,
		float V0,
		float V1)
	{
		const FVector3f Left0(LeftRing.X, FMath::Cos(Angle0) * LeftRing.Radius, FMath::Sin(Angle0) * LeftRing.Radius);
		const FVector3f Left1(LeftRing.X, FMath::Cos(Angle1) * LeftRing.Radius, FMath::Sin(Angle1) * LeftRing.Radius);
		const FVector3f Right0(RightRing.X, FMath::Cos(Angle0) * RightRing.Radius, FMath::Sin(Angle0) * RightRing.Radius);
		const FVector3f Right1(RightRing.X, FMath::Cos(Angle1) * RightRing.Radius, FMath::Sin(Angle1) * RightRing.Radius);
		const FVector3f Normal0(0.0f, FMath::Cos(Angle0), FMath::Sin(Angle0));
		const FVector3f Normal1(0.0f, FMath::Cos(Angle1), FMath::Sin(Angle1));

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(4);
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Left0, Normal0, FVector2f(U0, V0)));
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Left1, Normal1, FVector2f(U0, V1)));
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Right1, Normal1, FVector2f(U1, V1)));
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Right0, Normal0, FVector2f(U1, V0)));
		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void AddBaseballBatCap(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FBaseballBatRing& Ring,
		bool bRightCap)
	{
		constexpr int32 SegmentCount = 24;
		const FVector3f CapNormal = bRightCap ? FVector3f(1.0f, 0.0f, 0.0f) : FVector3f(-1.0f, 0.0f, 0.0f);
		const FVector3f Center(Ring.X, 0.0f, 0.0f);

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float V0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float V1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = V0 * 2.0f * UE_PI;
			const float Angle1 = V1 * 2.0f * UE_PI;
			const FVector3f Edge0(Ring.X, FMath::Cos(Angle0) * Ring.Radius, FMath::Sin(Angle0) * Ring.Radius);
			const FVector3f Edge1(Ring.X, FMath::Cos(Angle1) * Ring.Radius, FMath::Sin(Angle1) * Ring.Radius);
			const FVector2f CenterUV(0.5f, 0.5f);
			const FVector2f EdgeUV0(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f);
			const FVector2f EdgeUV1(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f);

			TArray<FVertexInstanceID> VertexInstances;
			VertexInstances.Reserve(3);
			VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Center, CapNormal, CenterUV));
			if (bRightCap)
			{
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge0, CapNormal, EdgeUV0));
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge1, CapNormal, EdgeUV1));
			}
			else
			{
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge1, CapNormal, EdgeUV1));
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge0, CapNormal, EdgeUV0));
			}
			MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
		}
	}

	void BuildBaseballBatMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Wood"));

		const TArray<FBaseballBatRing> Rings = {
			{ -62.0f, 5.2f },
			{ -57.0f, 8.0f },
			{ -50.0f, 5.4f },
			{ -38.0f, 3.1f },
			{ 20.0f, 3.6f },
			{ 35.0f, 5.4f },
			{ 50.0f, 7.4f },
			{ 89.0f, 8.8f },
			{ 99.0f, 8.1f }
		};
		const float MinX = Rings[0].X;
		const float Length = FMath::Max(1.0f, Rings.Last().X - MinX);

		constexpr int32 SegmentCount = 24;
		for (int32 RingIndex = 0; RingIndex < Rings.Num() - 1; ++RingIndex)
		{
			const FBaseballBatRing& LeftRing = Rings[RingIndex];
			const FBaseballBatRing& RightRing = Rings[RingIndex + 1];
			const float U0 = (LeftRing.X - MinX) / Length;
			const float U1 = (RightRing.X - MinX) / Length;

			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const float V0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
				const float V1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
				const float Angle0 = V0 * 2.0f * UE_PI;
				const float Angle1 = V1 * 2.0f * UE_PI;
				AddBaseballBatSideQuad(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					LeftRing,
					RightRing,
					Angle0,
					Angle1,
					U0,
					U1,
					V0,
					V1);
			}
		}

		AddBaseballBatCap(MeshDescription, Attributes, PolygonGroupId, Rings[0], false);
		AddBaseballBatCap(MeshDescription, Attributes, PolygonGroupId, Rings.Last(), true);
	}

	UTexture2D* EnsureBaseballBatWoodTexture()
	{
		const FString ObjectPath = GetAssetObjectPath(WeaponAssetPath, BaseballBatWoodTextureAssetName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!Texture)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *WeaponAssetPath, *BaseballBatWoodTextureAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			Texture = NewObject<UTexture2D>(
				Package,
				*BaseballBatWoodTextureAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!Texture)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Texture);
		}

		constexpr int32 TextureSize = 256;
		TArray<uint8> Pixels;
		Pixels.SetNumZeroed(TextureSize * TextureSize * 4);
		for (int32 Y = 0; Y < TextureSize; ++Y)
		{
			for (int32 X = 0; X < TextureSize; ++X)
			{
				const float U = static_cast<float>(X) / static_cast<float>(TextureSize - 1);
				const float V = static_cast<float>(Y) / static_cast<float>(TextureSize - 1);
				const float GrainNoise = FMath::PerlinNoise2D(FVector2D(U * 7.0f, V * 5.5f)) * 0.5f + 0.5f;
				const float FineNoise = FMath::PerlinNoise2D(FVector2D(U * 44.0f + 31.0f, V * 12.0f - 17.0f)) * 0.5f + 0.5f;
				const float GrainWave = FMath::Sin((V * 17.0f + GrainNoise * 1.7f + U * 0.8f) * 2.0f * UE_PI) * 0.5f + 0.5f;
				const float RingLine = FMath::Clamp((GrainWave - 0.72f) / 0.28f, 0.0f, 1.0f);
				const float Shade = FMath::Clamp(0.54f + GrainWave * 0.25f + FineNoise * 0.15f, 0.0f, 1.0f);

				float Red = FMath::Lerp(132.0f, 236.0f, Shade);
				float Green = FMath::Lerp(76.0f, 174.0f, Shade);
				float Blue = FMath::Lerp(34.0f, 82.0f, Shade);
				Red = FMath::Lerp(Red, 92.0f, RingLine * 0.28f);
				Green = FMath::Lerp(Green, 48.0f, RingLine * 0.28f);
				Blue = FMath::Lerp(Blue, 20.0f, RingLine * 0.28f);

				const int32 PixelIndex = (Y * TextureSize + X) * 4;
				Pixels[PixelIndex + 0] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Blue), 0, 255));
				Pixels[PixelIndex + 1] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Green), 0, 255));
				Pixels[PixelIndex + 2] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Red), 0, 255));
				Pixels[PixelIndex + 3] = 255;
			}
		}

		Texture->Modify();
		Texture->Source.Init(TextureSize, TextureSize, 1, 1, TSF_BGRA8, Pixels.GetData());
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->LODGroup = TEXTUREGROUP_World;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();

		if (!SaveAsset(Texture))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Texture;
	}

	UMaterial* EnsureBaseballBatWoodMaterial(UTexture2D* WoodTexture)
	{
		if (!WoodTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(WeaponAssetPath, BaseballBatMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				BaseballBatMaterialAssetName,
				WeaponAssetPath,
				UMaterial::StaticClass(),
				MaterialFactory);

			Material = Cast<UMaterial>(CreatedAsset);
			if (!Material)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->UTiling = 1.6f;
		TextureCoordinateExpression->VTiling = 1.0f;
		TextureCoordinateExpression->MaterialExpressionEditorX = -720;
		TextureCoordinateExpression->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* WoodSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		WoodSample->Material = Material;
		WoodSample->ParameterName = TEXT("WoodTexture");
		WoodSample->Texture = WoodTexture;
		WoodSample->SamplerType = SAMPLERTYPE_Color;
		WoodSample->Coordinates.Connect(0, TextureCoordinateExpression);
		WoodSample->MaterialExpressionEditorX = -460;
		WoodSample->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(WoodSample);

		UMaterialExpressionScalarParameter* RoughnessParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		RoughnessParameter->Material = Material;
		RoughnessParameter->ParameterName = TEXT("Roughness");
		RoughnessParameter->DefaultValue = 0.68f;
		RoughnessParameter->MaterialExpressionEditorX = -460;
		RoughnessParameter->MaterialExpressionEditorY = 220;
		Material->GetExpressionCollection().AddExpression(RoughnessParameter);

		UMaterialExpressionScalarParameter* SpecularParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		SpecularParameter->Material = Material;
		SpecularParameter->ParameterName = TEXT("Specular");
		SpecularParameter->DefaultValue = 0.24f;
		SpecularParameter->MaterialExpressionEditorX = -460;
		SpecularParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(SpecularParameter);

		MaterialEditorOnly->BaseColor.Connect(0, WoodSample);
		MaterialEditorOnly->Roughness.Connect(0, RoughnessParameter);
		MaterialEditorOnly->Specular.Connect(0, SpecularParameter);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureBaseballBatStaticMeshAsset(UMaterialInterface* WoodMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(WeaponAssetPath, BaseballBatMeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *WeaponAssetPath, *BaseballBatMeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*BaseballBatMeshAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();

		FMeshDescription MeshDescription;
		BuildBaseballBatMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(WoodMaterial, FName(TEXT("Wood"))));

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureBaseballBatAssets()
	{
		UTexture2D* WoodTexture = EnsureBaseballBatWoodTexture();
		UMaterial* WoodMaterial = EnsureBaseballBatWoodMaterial(WoodTexture);
		if (!WoodTexture || !WoodMaterial)
		{
			return false;
		}

		return EnsureBaseballBatStaticMeshAsset(WoodMaterial) != nullptr;
	}

	UMaterial* EnsureLedExpressionMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LedExpressionMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				LedExpressionMaterialAssetName,
				EffectsAssetPath,
				UMaterial::StaticClass(),
				MaterialFactory);

			Material = Cast<UMaterial>(CreatedAsset);
			if (!Material)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Additive;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -520;
		VertexColorExpression->MaterialExpressionEditorY = -20;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionScalarParameter* IntensityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		IntensityParameter->Material = Material;
		IntensityParameter->ParameterName = TEXT("Intensity");
		IntensityParameter->DefaultValue = 3.0f;
		IntensityParameter->MaterialExpressionEditorX = -520;
		IntensityParameter->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(IntensityParameter);

		UMaterialExpressionMultiply* EmissiveMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMultiply->Material = Material;
		EmissiveMultiply->A.Connect(0, VertexColorExpression);
		EmissiveMultiply->B.Connect(0, IntensityParameter);
		EmissiveMultiply->MaterialExpressionEditorX = -220;
		EmissiveMultiply->MaterialExpressionEditorY = 60;
		Material->GetExpressionCollection().AddExpression(EmissiveMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, VertexColorExpression);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMultiply);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	template <typename AssetType>
	AssetType* EnsureDataAsset(const FString& AssetPath, const FString& AssetName)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		if (AssetType* ExistingAsset = LoadObject<AssetType>(nullptr, *ObjectPath))
		{
			return ExistingAsset;
		}

		const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *AssetName);
		UPackage* Package = CreatePackage(*PackageName);
		AssetType* CreatedAsset = NewObject<AssetType>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!CreatedAsset)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(CreatedAsset);
		CreatedAsset->MarkPackageDirty();
		return CreatedAsset;
	}

	UInputAction* EnsureInputAction(const FString& AssetName, EInputActionValueType ValueType, EInputActionAccumulationBehavior AccumulationBehavior)
	{
		UInputAction* Action = EnsureDataAsset<UInputAction>(InputAssetPath, AssetName);
		if (!Action)
		{
			return nullptr;
		}

		Action->ValueType = ValueType;
		Action->AccumulationBehavior = AccumulationBehavior;
		Action->MarkPackageDirty();
		SaveAsset(Action);
		return Action;
	}

	FString GetQuickSlotActionName(int32 SlotNumber)
	{
		return FString::Printf(TEXT("%s%d"), *QuickSlotActionNamePrefix, SlotNumber);
	}

	void AddSwizzleModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer, EInputAxisSwizzle Order)
	{
		UInputModifierSwizzleAxis* SwizzleModifier = NewObject<UInputModifierSwizzleAxis>(Outer, NAME_None, RF_Transactional);
		SwizzleModifier->Order = Order;
		Mapping.Modifiers.Add(SwizzleModifier);
	}

	void AddNegateModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer)
	{
		UInputModifierNegate* NegateModifier = NewObject<UInputModifierNegate>(Outer, NAME_None, RF_Transactional);
		NegateModifier->bX = true;
		NegateModifier->bY = false;
		NegateModifier->bZ = false;
		Mapping.Modifiers.Add(NegateModifier);
	}

	UInputMappingContext* EnsureInputMappingContext(UInputAction* MoveAction, UInputAction* FireAction, UInputAction* AimAction)
	{
		if (!MoveAction || !FireAction || !AimAction)
		{
			return nullptr;
		}

		UInputMappingContext* MappingContext = EnsureDataAsset<UInputMappingContext>(InputAssetPath, MappingContextName);
		if (!MappingContext)
		{
			return nullptr;
		}

		MappingContext->UnmapAll();

		FEnhancedActionKeyMapping& WMapping = MappingContext->MapKey(MoveAction, EKeys::W);
		AddSwizzleModifier(WMapping, MappingContext, EInputAxisSwizzle::YXZ);

		FEnhancedActionKeyMapping& SMapping = MappingContext->MapKey(MoveAction, EKeys::S);
		AddNegateModifier(SMapping, MappingContext);
		AddSwizzleModifier(SMapping, MappingContext, EInputAxisSwizzle::YXZ);

		FEnhancedActionKeyMapping& AMapping = MappingContext->MapKey(MoveAction, EKeys::A);
		AddNegateModifier(AMapping, MappingContext);

		MappingContext->MapKey(MoveAction, EKeys::D);
		MappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
		MappingContext->MapKey(AimAction, EKeys::RightMouseButton);

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, fire, and aim input."));
		MappingContext->MarkPackageDirty();
		SaveAsset(MappingContext);
		return MappingContext;
	}

	bool HasInputMapping(const UInputMappingContext* MappingContext, const UInputAction* Action, const FKey& Key)
	{
		if (!MappingContext || !Action)
		{
			return false;
		}

		for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
		{
			if (Mapping.Action == Action && Mapping.Key == Key)
			{
				return true;
			}
		}

		return false;
	}

	bool EnsureInteractionInputAssets()
	{
		UInputAction* InteractAction = EnsureInputAction(
			InteractActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputAction* InteractionFocusAction = EnsureInputAction(
			InteractionFocusActionName,
			EInputActionValueType::Axis1D,
			EInputActionAccumulationBehavior::Cumulative);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!InteractAction || !InteractionFocusAction || !MappingContext)
		{
			return false;
		}

		MappingContext->UnmapKey(InteractAction, EKeys::E);

		if (!HasInputMapping(MappingContext, InteractAction, EKeys::F))
		{
			MappingContext->MapKey(InteractAction, EKeys::F);
		}

		if (!HasInputMapping(MappingContext, InteractionFocusAction, EKeys::MouseWheelAxis))
		{
			MappingContext->MapKey(InteractionFocusAction, EKeys::MouseWheelAxis);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, fire, aim, interaction, and interaction focus input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureInventoryInputAssets()
	{
		UInputAction* InventoryAction = EnsureInputAction(
			InventoryActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!InventoryAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, InventoryAction, EKeys::Tab))
		{
			MappingContext->MapKey(InventoryAction, EKeys::Tab);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, and inventory input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureQuickSlotInputAssets()
	{
		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!MappingContext)
		{
			return false;
		}

		static const FKey QuickSlotKeys[8] = {
			EKeys::One,
			EKeys::Two,
			EKeys::Three,
			EKeys::Four,
			EKeys::Five,
			EKeys::Six,
			EKeys::Seven,
			EKeys::Eight
		};

		bool bAllActionsCreated = true;
		for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(QuickSlotKeys); ++SlotIndex)
		{
			const int32 SlotNumber = SlotIndex + 1;
			UInputAction* QuickSlotAction = EnsureInputAction(
				GetQuickSlotActionName(SlotNumber),
				EInputActionValueType::Boolean,
				EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

			bAllActionsCreated = bAllActionsCreated && QuickSlotAction;
			if (QuickSlotAction && !HasInputMapping(MappingContext, QuickSlotAction, QuickSlotKeys[SlotIndex]))
			{
				MappingContext->MapKey(QuickSlotAction, QuickSlotKeys[SlotIndex]);
			}
		}

		UInputAction* MeleeQuickSlotAction = EnsureInputAction(
			MeleeQuickSlotActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		bAllActionsCreated = bAllActionsCreated && MeleeQuickSlotAction;
		if (MeleeQuickSlotAction && !HasInputMapping(MappingContext, MeleeQuickSlotAction, EKeys::V))
		{
			MappingContext->MapKey(MeleeQuickSlotAction, EKeys::V);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, and quick slot input."));
		MappingContext->MarkPackageDirty();
		return bAllActionsCreated && SaveAsset(MappingContext);
	}

	bool EnsureDropInputAssets()
	{
		UInputAction* DropAction = EnsureInputAction(
			DropActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!DropAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, DropAction, EKeys::X))
		{
			MappingContext->MapKey(DropAction, EKeys::X);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, and item drop input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureAmmoReloadInputAssets()
	{
		UInputAction* ReloadAction = EnsureInputAction(
			ReloadActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputAction* AmmoSelectAction = EnsureInputAction(
			AmmoSelectActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputAction* AmmoFocusAction = EnsureInputAction(
			AmmoFocusActionName,
			EInputActionValueType::Axis1D,
			EInputActionAccumulationBehavior::Cumulative);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!ReloadAction || !AmmoSelectAction || !AmmoFocusAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, ReloadAction, EKeys::R))
		{
			MappingContext->MapKey(ReloadAction, EKeys::R);
		}

		if (!HasInputMapping(MappingContext, AmmoSelectAction, EKeys::T))
		{
			MappingContext->MapKey(AmmoSelectAction, EKeys::T);
		}

		if (!HasInputMapping(MappingContext, AmmoFocusAction, EKeys::MouseWheelAxis))
		{
			MappingContext->MapKey(AmmoFocusAction, EKeys::MouseWheelAxis);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, and reload input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureCameraModeInputAssets()
	{
		UInputAction* CameraModeAction = EnsureInputAction(
			CameraModeActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!CameraModeAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, CameraModeAction, EKeys::Y))
		{
			MappingContext->MapKey(CameraModeAction, EKeys::Y);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, and camera mode input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureSprintInputAssets()
	{
		UInputAction* SprintAction = EnsureInputAction(
			SprintActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!SprintAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, SprintAction, EKeys::LeftShift))
		{
			MappingContext->MapKey(SprintAction, EKeys::LeftShift);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, camera mode, and sprint input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureRollInputAssets()
	{
		UInputAction* RollAction = EnsureInputAction(
			RollActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!RollAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, RollAction, EKeys::SpaceBar))
		{
			MappingContext->MapKey(RollAction, EKeys::SpaceBar);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, camera mode, sprint, and roll input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureJumpInputAssets()
	{
		UInputAction* JumpAction = EnsureInputAction(
			JumpActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!JumpAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, JumpAction, EKeys::J))
		{
			MappingContext->MapKey(JumpAction, EKeys::J);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, camera mode, sprint, roll, jump, and map input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureMapInputAssets()
	{
		UInputAction* MapAction = EnsureInputAction(
			MapActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!MapAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, MapAction, EKeys::M))
		{
			MappingContext->MapKey(MapAction, EKeys::M);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, camera mode, sprint, roll, and map input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool ConfigureGameModeBlueprint(UBlueprint* GameModeBlueprint, UBlueprint* PlayerBlueprint)
	{
		if (!GameModeBlueprint || !PlayerBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(PlayerBlueprint);
		FKismetEditorUtilities::CompileBlueprint(GameModeBlueprint);

		UClass* PlayerClass = PlayerBlueprint->GeneratedClass;
		UClass* PlayerControllerClass = ATunaSweeperPlayerController::StaticClass();
		AGameModeBase* GameModeDefaults = GameModeBlueprint->GeneratedClass
			? Cast<AGameModeBase>(GameModeBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;

		if (!PlayerClass || !PlayerControllerClass || !GameModeDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure BP_TunaSweeperGameMode defaults."));
			return false;
		}

		GameModeBlueprint->Modify();
		GameModeDefaults->Modify();
		GameModeDefaults->DefaultPawnClass = PlayerClass;
		GameModeDefaults->PlayerControllerClass = PlayerControllerClass;
		GameModeBlueprint->MarkPackageDirty();

		return SaveAsset(GameModeBlueprint);
	}

	bool SetProjectGameModeToBlueprint()
	{
		const FString GameModeClassPath = GetAssetClassPath(GameInstanceAssetPath, GameModeAssetName);
		UGameMapsSettings::SetGlobalDefaultGameMode(GameModeClassPath);

		if (UGameMapsSettings* GameMapsSettings = GetMutableDefault<UGameMapsSettings>())
		{
			GameMapsSettings->SaveConfig();
		}

		const FString DefaultEngineIni = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GlobalDefaultGameMode"),
			*GameModeClassPath,
			DefaultEngineIni);
		GConfig->Flush(false, DefaultEngineIni);

		FString SavedGameModeClass;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GlobalDefaultGameMode"),
			SavedGameModeClass,
			DefaultEngineIni);

		return SavedGameModeClass == GameModeClassPath;
	}

	bool SetProjectStartupMapsToIntro()
	{
		const FString IntroMapObjectPath = FString::Printf(TEXT("%s.IntroMap"), *IntroMapPackagePath);

		if (UGameMapsSettings* GameMapsSettings = GetMutableDefault<UGameMapsSettings>())
		{
			UGameMapsSettings::SetGameDefaultMap(IntroMapObjectPath);
			GameMapsSettings->EditorStartupMap = FSoftObjectPath(IntroMapObjectPath);
			GameMapsSettings->SaveConfig();
		}

		const FString DefaultEngineIni = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameDefaultMap"),
			*IntroMapObjectPath,
			DefaultEngineIni);
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("EditorStartupMap"),
			*IntroMapObjectPath,
			DefaultEngineIni);
		GConfig->Flush(false, DefaultEngineIni);

		FString SavedGameDefaultMap;
		FString SavedEditorStartupMap;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameDefaultMap"),
			SavedGameDefaultMap,
			DefaultEngineIni);
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("EditorStartupMap"),
			SavedEditorStartupMap,
			DefaultEngineIni);

		return SavedGameDefaultMap == IntroMapObjectPath && SavedEditorStartupMap == IntroMapObjectPath;
	}

	bool EnsureGameInstanceBlueprint()
	{
		const FString ObjectPath = GetGameInstanceObjectPath();

		if (UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath))
		{
			if (ExistingBlueprint->ParentClass != UTunaSweeperGameInstance::StaticClass())
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but its parent class is not UTunaSweeperGameInstance."), *ObjectPath);
				return false;
			}

			return SetProjectGameInstanceToBlueprint();
		}

		UBlueprintFactory* BlueprintFactory = NewObject<UBlueprintFactory>();
		BlueprintFactory->ParentClass = UTunaSweeperGameInstance::StaticClass();

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			GameInstanceAssetName,
			GameInstanceAssetPath,
			UBlueprint::StaticClass(),
			BlueprintFactory);

		UBlueprint* CreatedBlueprint = Cast<UBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(CreatedBlueprint);
		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();

		if (!SaveAsset(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return false;
		}

		return SetProjectGameInstanceToBlueprint();
	}

	bool EnsureProjectileHitEffectAssets()
	{
		UBlueprint* HitEffectBlueprint = EnsureBlueprint(
			EffectsAssetPath,
			ProjectileHitRedBurstActorAssetName,
			ATunaSweeperProjectileHitBurstActor::StaticClass());
		UTunaSweeperProjectileHitEffectDataAsset* HitEffectDataAsset =
			EnsureDataAsset<UTunaSweeperProjectileHitEffectDataAsset>(
				EffectsAssetPath,
				ProjectileHitEffectDataAssetName);
		if (!HitEffectBlueprint || !HitEffectDataAsset)
		{
			return false;
		}

		HitEffectDataAsset->Modify();
		HitEffectDataAsset->HitEffects.Reset();

		FTunaSweeperProjectileHitEffectDefinition RedBurstDefinition;
		RedBurstDefinition.EffectId = FName(TEXT("hit.red_burst"));
		RedBurstDefinition.EffectActorClass =
			TSoftClassPtr<ATunaSweeperProjectileHitBurstActor>(
				FSoftObjectPath(GetAssetClassPath(EffectsAssetPath, ProjectileHitRedBurstActorAssetName)));
		RedBurstDefinition.BurstColor = FLinearColor(1.0f, 0.03f, 0.0f, 1.0f);
		RedBurstDefinition.SpawnScale = FVector::OneVector;
		RedBurstDefinition.SurfaceOffsetCm = 1.0f;
		HitEffectDataAsset->HitEffects.Add(RedBurstDefinition);
		HitEffectDataAsset->MarkPackageDirty();
		if (!SaveAsset(HitEffectDataAsset))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *GetAssetObjectPath(EffectsAssetPath, ProjectileHitEffectDataAssetName));
			return false;
		}

		UBlueprint* GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint && !EnsureGameInstanceBlueprint())
		{
			return false;
		}

		GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint)
		{
			return false;
		}

		if (!GameInstanceBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);
		}

		UTunaSweeperGameInstance* GameInstanceDefaults = GameInstanceBlueprint->GeneratedClass
			? Cast<UTunaSweeperGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!GameInstanceDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure projectile hit effect mapping on %s."), *GetGameInstanceObjectPath());
			return false;
		}

		GameInstanceBlueprint->Modify();
		GameInstanceDefaults->Modify();
		GameInstanceDefaults->ProjectileHitEffectDataAsset =
			TSoftObjectPtr<UTunaSweeperProjectileHitEffectDataAsset>(
				FSoftObjectPath(GetAssetObjectPath(EffectsAssetPath, ProjectileHitEffectDataAssetName)));
		GameInstanceBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

		return SaveAsset(GameInstanceBlueprint);
	}

	bool EnsureImpactPhysicalMaterialAssets()
	{
		struct FPhysicalMaterialDefinition
		{
			const TCHAR* AssetName;
			EPhysicalSurface SurfaceType;
		};

		constexpr FPhysicalMaterialDefinition Definitions[] = {
			{ TEXT("PM_Flesh"), SurfaceType1 },
			{ TEXT("PM_Wood"), SurfaceType4 },
			{ TEXT("PM_Stone"), SurfaceType8 },
			{ TEXT("PM_Dirt"), SurfaceType9 },
			{ TEXT("PM_Metal"), SurfaceType2 },
			{ TEXT("PM_Water"), SurfaceType6 }
		};

		const FString PhysicalMaterialAssetPath = TEXT("/Game/Physics/PhysicalMaterials");
		for (const FPhysicalMaterialDefinition& Definition : Definitions)
		{
			UPhysicalMaterial* PhysicalMaterial = EnsureDataAsset<UPhysicalMaterial>(
				PhysicalMaterialAssetPath,
				Definition.AssetName);
			if (!PhysicalMaterial)
			{
				return false;
			}

			PhysicalMaterial->Modify();
			PhysicalMaterial->SurfaceType = Definition.SurfaceType;
			PhysicalMaterial->MarkPackageDirty();
			if (!SaveAsset(PhysicalMaterial))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *PhysicalMaterial->GetPathName());
				return false;
			}
		}

		return true;
	}

	bool EnsureWeaponSpreadRecoilAssets()
	{
		UTunaSweeperWeaponSpreadRecoilDataAsset* RecoilDataAsset =
			EnsureDataAsset<UTunaSweeperWeaponSpreadRecoilDataAsset>(
				WeaponAssetPath,
				WeaponSpreadRecoilDataAssetName);
		if (!RecoilDataAsset)
		{
			return false;
		}

		RecoilDataAsset->Modify();
		RecoilDataAsset->WeaponTypeDefinitions.Reset();
		auto AddRecoilDefinition = [RecoilDataAsset](
			const TCHAR* WeaponTypeTag,
			float IncreasePerShot,
			float MinimumSpreadHalfAngleDegrees,
			float MaximumSpreadHalfAngleDegrees,
			float AimedSpreadMultiplier,
			float DecreasePerSecond)
		{
			FTunaSweeperWeaponSpreadRecoilDefinition Definition;
			Definition.WeaponTypeTag = FName(WeaponTypeTag);
			Definition.IncreasePerShot = IncreasePerShot;
			Definition.MinimumSpreadHalfAngleDegrees = MinimumSpreadHalfAngleDegrees;
			Definition.MaximumSpreadHalfAngleDegrees = MaximumSpreadHalfAngleDegrees;
			Definition.AimedSpreadMultiplier = AimedSpreadMultiplier;
			Definition.DecreasePerSecond = DecreasePerSecond;
			RecoilDataAsset->WeaponTypeDefinitions.Add(Definition);
		};

		AddRecoilDefinition(TEXT("weapon.type.pistol"), 1.2f, 1.4f, 7.0f, 0.5f, 5.0f);
		AddRecoilDefinition(TEXT("weapon.type.rifle"), 0.8f, 1.0f, 6.0f, 0.45f, 6.5f);
		AddRecoilDefinition(TEXT("weapon.type.shotgun"), 2.0f, 4.5f, 12.0f, 0.65f, 4.5f);
		AddRecoilDefinition(TEXT("weapon.type.smg"), 1.0f, 1.8f, 8.5f, 0.5f, 7.5f);

		RecoilDataAsset->MarkPackageDirty();
		if (!SaveAsset(RecoilDataAsset))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *GetAssetObjectPath(WeaponAssetPath, WeaponSpreadRecoilDataAssetName));
			return false;
		}

		UBlueprint* GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint && !EnsureGameInstanceBlueprint())
		{
			return false;
		}

		GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint)
		{
			return false;
		}

		if (!GameInstanceBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);
		}

		UTunaSweeperGameInstance* GameInstanceDefaults = GameInstanceBlueprint->GeneratedClass
			? Cast<UTunaSweeperGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!GameInstanceDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure weapon spread recoil mapping on %s."), *GetGameInstanceObjectPath());
			return false;
		}

		GameInstanceBlueprint->Modify();
		GameInstanceDefaults->Modify();
		GameInstanceDefaults->WeaponSpreadRecoilDataAsset =
			TSoftObjectPtr<UTunaSweeperWeaponSpreadRecoilDataAsset>(
				FSoftObjectPath(GetAssetObjectPath(WeaponAssetPath, WeaponSpreadRecoilDataAssetName)));
		GameInstanceBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

		return SaveAsset(GameInstanceBlueprint);
	}

	bool EnsureEnemyCombatDebugInputAssets()
	{
		UInputAction* ToggleAction = EnsureInputAction(
			ToggleEnemyCombatDebugActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));
		if (!ToggleAction || !MappingContext)
		{
			return false;
		}
		if (!HasInputMapping(MappingContext, ToggleAction, EKeys::F8))
		{
			MappingContext->MapKey(ToggleAction, EKeys::F8);
		}
		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player input including enemy combat debug toggle."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsurePlayerFootstepPresentationAssets()
	{
		USoundBase* BasicFootstepSound = LoadObject<USoundBase>(nullptr, *BasicFootstepSoundObjectPath);
		if (!BasicFootstepSound)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load %s."), *BasicFootstepSoundObjectPath);
			return false;
		}

		UTunaSweeperFootstepPresentationDataAsset* PresentationDataAsset =
			EnsureDataAsset<UTunaSweeperFootstepPresentationDataAsset>(
				PlayerFootstepPresentationAssetPath,
				PlayerFootstepPresentationDataAssetName);
		if (!PresentationDataAsset)
		{
			return false;
		}

		PresentationDataAsset->Modify();
		PresentationDataAsset->BasicFootstepSound = TSoftObjectPtr<USoundBase>(BasicFootstepSound);
		PresentationDataAsset->MarkPackageDirty();
		if (!SaveAsset(PresentationDataAsset))
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to save %s."),
				*GetAssetObjectPath(PlayerFootstepPresentationAssetPath, PlayerFootstepPresentationDataAssetName));
			return false;
		}

		UBlueprint* GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint && !EnsureGameInstanceBlueprint())
		{
			return false;
		}

		GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint)
		{
			return false;
		}

		if (!GameInstanceBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);
		}

		UTunaSweeperGameInstance* GameInstanceDefaults = GameInstanceBlueprint->GeneratedClass
			? Cast<UTunaSweeperGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!GameInstanceDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure footstep presentation on %s."), *GetGameInstanceObjectPath());
			return false;
		}

		GameInstanceBlueprint->Modify();
		GameInstanceDefaults->Modify();
		GameInstanceDefaults->FootstepPresentationDataAsset =
			TSoftObjectPtr<UTunaSweeperFootstepPresentationDataAsset>(
				FSoftObjectPath(GetAssetObjectPath(
					PlayerFootstepPresentationAssetPath,
					PlayerFootstepPresentationDataAssetName)));
		GameInstanceBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

		return SaveAsset(GameInstanceBlueprint);
	}

	bool EnsureWeaponPresentationAssets()
	{
		FString FireWavPath;
		FString ReloadStartWavPath;
		FString ReloadCompleteWavPath;
		if (!FTunaSweeperFMSoundTool::RenderWeaponPresentationWavs(
			FireWavPath,
			ReloadStartWavPath,
			ReloadCompleteWavPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to render rifle weapon-presentation WAV files."));
			return false;
		}

		auto ImportWeaponSound = [](const FString& SourceFile, const FString& AssetName, USoundWave*& OutSoundWave)
		{
			FAudioImportArgs ImportArgs;
			ImportArgs.SourceFile = SourceFile;
			ImportArgs.DestinationPath = WeaponPresentationAudioAssetPath;
			ImportArgs.AssetName = AssetName;
			ImportArgs.bReplaceExisting = true;
			ImportArgs.bLooping = false;
			if (!ImportAudioAsset(ImportArgs, &OutSoundWave) || !OutSoundWave)
			{
				return false;
			}

			OutSoundWave->Modify();
			OutSoundWave->SoundGroup = SOUNDGROUP_Effects;
			OutSoundWave->PostEditChange();
			OutSoundWave->MarkPackageDirty();
			return SaveAsset(OutSoundWave);
		};

		USoundWave* FireSound = nullptr;
		USoundWave* ReloadStartSound = nullptr;
		USoundWave* ReloadCompleteSound = nullptr;
		if (!ImportWeaponSound(FireWavPath, RifleFireSoundAssetName, FireSound) ||
			!ImportWeaponSound(ReloadStartWavPath, RifleReloadStartSoundAssetName, ReloadStartSound) ||
			!ImportWeaponSound(ReloadCompleteWavPath, RifleReloadCompleteSoundAssetName, ReloadCompleteSound))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import rifle weapon-presentation SoundWave assets."));
			return false;
		}

		UTunaSweeperWeaponPresentationDataAsset* PresentationDataAsset =
			EnsureDataAsset<UTunaSweeperWeaponPresentationDataAsset>(
				WeaponPresentationAssetPath,
				WeaponPresentationRifleDataAssetName);
		if (!PresentationDataAsset)
		{
			return false;
		}

		PresentationDataAsset->Modify();
		PresentationDataAsset->WeaponTypeTag = FName(TEXT("weapon.type.rifle"));
		PresentationDataAsset->MuzzleFlashEffect = TSoftObjectPtr<UNiagaraSystem>(
			FSoftObjectPath(TEXT("/Game/BallisticsVFX/Particles/Muzzle/MuzzleFlash/NS_Muzzle_Flash_Med2.NS_Muzzle_Flash_Med2")));
		PresentationDataAsset->FireSound = TSoftObjectPtr<USoundBase>(FireSound);
		PresentationDataAsset->ReloadStartSound = TSoftObjectPtr<USoundBase>(ReloadStartSound);
		PresentationDataAsset->ReloadCompleteSound = TSoftObjectPtr<USoundBase>(ReloadCompleteSound);
		PresentationDataAsset->MarkPackageDirty();
		if (!SaveAsset(PresentationDataAsset))
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to save %s."),
				*GetAssetObjectPath(WeaponPresentationAssetPath, WeaponPresentationRifleDataAssetName));
			return false;
		}

		UBlueprint* AssaultRifleBlueprint = LoadObject<UBlueprint>(
			nullptr,
			TEXT("/Game/Weapons/BP_AssaultRifle.BP_AssaultRifle"));
		if (!AssaultRifleBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load /Game/Weapons/BP_AssaultRifle."));
			return false;
		}

		if (!AssaultRifleBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(AssaultRifleBlueprint);
		}

		ATunaSweeperWeapon* AssaultRifleDefaults = AssaultRifleBlueprint->GeneratedClass
			? Cast<ATunaSweeperWeapon>(AssaultRifleBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!AssaultRifleDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to access BP_AssaultRifle defaults."));
			return false;
		}

		AssaultRifleBlueprint->Modify();
		AssaultRifleDefaults->Modify();
		AssaultRifleDefaults->SetWeaponPresentationDataAsset(
			TSoftObjectPtr<UTunaSweeperWeaponPresentationDataAsset>(
				FSoftObjectPath(GetAssetObjectPath(WeaponPresentationAssetPath, WeaponPresentationRifleDataAssetName))));
		AssaultRifleBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(AssaultRifleBlueprint);

		return SaveAsset(AssaultRifleBlueprint);
	}

	bool EnsureWeaponPresentationEmptyFireSoundAsset()
	{
		UTunaSweeperWeaponPresentationDataAsset* PresentationDataAsset =
			LoadObject<UTunaSweeperWeaponPresentationDataAsset>(
				nullptr,
				*GetAssetObjectPath(WeaponPresentationAssetPath, WeaponPresentationRifleDataAssetName));
		if (!PresentationDataAsset)
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to load %s."),
				*GetAssetObjectPath(WeaponPresentationAssetPath, WeaponPresentationRifleDataAssetName));
			return false;
		}

		if (!LoadObject<USoundBase>(nullptr, *RifleEmptyFireSoundObjectPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load %s."), *RifleEmptyFireSoundObjectPath);
			return false;
		}

		PresentationDataAsset->Modify();
		PresentationDataAsset->EmptyFireSound = TSoftObjectPtr<USoundBase>(
			FSoftObjectPath(RifleEmptyFireSoundObjectPath));
		PresentationDataAsset->MarkPackageDirty();
		return SaveAsset(PresentationDataAsset);
	}

	bool EnsureEnemyWeaponFallbackPresentationAsset()
	{
		USoundBase* FireSound = LoadObject<USoundBase>(
			nullptr,
			*GetAssetObjectPath(WeaponPresentationAudioAssetPath, RifleFireSoundAssetName));
		if (!FireSound)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load enemy fallback fire sound."));
			return false;
		}

		UTunaSweeperWeaponPresentationDataAsset* PresentationDataAsset =
			EnsureDataAsset<UTunaSweeperWeaponPresentationDataAsset>(
				WeaponPresentationAssetPath,
				EnemyWeaponFallbackPresentationDataAssetName);
		if (!PresentationDataAsset)
		{
			return false;
		}

		PresentationDataAsset->Modify();
		PresentationDataAsset->WeaponTypeTag = NAME_None;
		PresentationDataAsset->MuzzleFlashEffect.Reset();
		PresentationDataAsset->FireSound = TSoftObjectPtr<USoundBase>(FireSound);
		PresentationDataAsset->ReloadStartSound.Reset();
		PresentationDataAsset->ReloadCompleteSound.Reset();
		PresentationDataAsset->EmptyFireSound.Reset();
		PresentationDataAsset->MarkPackageDirty();
		if (!SaveAsset(PresentationDataAsset))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save enemy fallback presentation data asset."));
			return false;
		}

		UBlueprint* GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint && !EnsureGameInstanceBlueprint())
		{
			return false;
		}

		GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint)
		{
			return false;
		}

		if (!GameInstanceBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);
		}

		UTunaSweeperGameInstance* GameInstanceDefaults = GameInstanceBlueprint->GeneratedClass
			? Cast<UTunaSweeperGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!GameInstanceDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure enemy fallback weapon presentation on %s."), *GetGameInstanceObjectPath());
			return false;
		}

		GameInstanceBlueprint->Modify();
		GameInstanceDefaults->Modify();
		GameInstanceDefaults->EnemyWeaponFallbackPresentationDataAsset =
			TSoftObjectPtr<UTunaSweeperWeaponPresentationDataAsset>(
				FSoftObjectPath(GetAssetObjectPath(
					WeaponPresentationAssetPath,
					EnemyWeaponFallbackPresentationDataAssetName)));
		GameInstanceBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

		return SaveAsset(GameInstanceBlueprint);
	}

	FVector ResolveMuzzleSocketLocation(const UStaticMesh& WeaponMesh)
	{
		const FBox MeshBounds = WeaponMesh.GetBoundingBox();
		FVector SocketLocation = MeshBounds.GetCenter();
		const float TipTolerance = FMath::Max(0.5f, MeshBounds.GetSize().X * 0.01f);

		if (const FStaticMeshRenderData* RenderData = WeaponMesh.GetRenderData();
			RenderData && !RenderData->LODResources.IsEmpty())
		{
			const FPositionVertexBuffer& PositionBuffer = RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
			FVector TipVertexSum = FVector::ZeroVector;
			int32 TipVertexCount = 0;
			for (uint32 VertexIndex = 0; VertexIndex < PositionBuffer.GetNumVertices(); ++VertexIndex)
			{
				const FVector VertexPosition(PositionBuffer.VertexPosition(VertexIndex));
				if (VertexPosition.X >= MeshBounds.Max.X - TipTolerance)
				{
					TipVertexSum += VertexPosition;
					++TipVertexCount;
				}
			}

			if (TipVertexCount > 0)
			{
				SocketLocation = TipVertexSum / static_cast<float>(TipVertexCount);
			}
		}

		// Place the emitter just outside the barrel, while preserving the barrel's real Y/Z centerline.
		SocketLocation.X = MeshBounds.Max.X + 0.5f;
		return SocketLocation;
	}

	bool EnsureAssaultRifleWeaponSockets()
	{
		UBlueprint* AssaultRifleBlueprint = LoadObject<UBlueprint>(
			nullptr,
			TEXT("/Game/Weapons/BP_AssaultRifle.BP_AssaultRifle"));
		if (!AssaultRifleBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load /Game/Weapons/BP_AssaultRifle while configuring weapon sockets."));
			return false;
		}

		if (!AssaultRifleBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(AssaultRifleBlueprint);
		}

		ATunaSweeperWeapon* AssaultRifleDefaults = AssaultRifleBlueprint->GeneratedClass
			? Cast<ATunaSweeperWeapon>(AssaultRifleBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		UStaticMeshComponent* WeaponMeshComponent = AssaultRifleDefaults
			? AssaultRifleDefaults->FindComponentByClass<UStaticMeshComponent>()
			: nullptr;
		UStaticMesh* RifleMesh = WeaponMeshComponent ? WeaponMeshComponent->GetStaticMesh() : nullptr;
		if (!RifleMesh)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("BP_AssaultRifle has no static WeaponMesh to receive weapon sockets."));
			return false;
		}

		const FVector MuzzleSocketLocation = ResolveMuzzleSocketLocation(*RifleMesh);
		const FBox MeshBounds = RifleMesh->GetBoundingBox();
		const FVector ShellEjectionSocketLocation(
			FMath::Lerp(MeshBounds.Min.X, MuzzleSocketLocation.X, 0.62f),
			MeshBounds.Max.Y + 0.5f,
			MuzzleSocketLocation.Z);
		const FVector LaserSightSocketLocation(
			FMath::Lerp(MeshBounds.Min.X, MuzzleSocketLocation.X, 0.58f),
			MeshBounds.GetCenter().Y,
			MeshBounds.Max.Z + 0.5f);

		bool bMeshChanged = false;
		auto EnsureSocket = [&RifleMesh, &bMeshChanged](FName SocketName, const FVector& Location)
		{
			if (RifleMesh->FindSocket(SocketName))
			{
				return;
			}

			if (!bMeshChanged)
			{
				RifleMesh->Modify();
				RifleMesh->PreEditChange(nullptr);
				bMeshChanged = true;
			}

			UStaticMeshSocket* Socket = NewObject<UStaticMeshSocket>(RifleMesh, SocketName, RF_Transactional);
			Socket->SocketName = SocketName;
			Socket->RelativeLocation = Location;
			Socket->RelativeRotation = FRotator::ZeroRotator;
			Socket->RelativeScale = FVector::OneVector;
			RifleMesh->AddSocket(Socket);
		};

		EnsureSocket(TEXT("MuzzleSocket"), MuzzleSocketLocation);
		EnsureSocket(TEXT("LaserSightSocket"), LaserSightSocketLocation);
		EnsureSocket(TEXT("ShellEjectionSocket"), ShellEjectionSocketLocation);

		if (!bMeshChanged)
		{
			return true;
		}

		RifleMesh->PostEditChange();
		RifleMesh->MarkPackageDirty();
		if (!SaveAsset(RifleMesh))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save rifle mesh sockets on %s."), *RifleMesh->GetPathName());
			return false;
		}

		UE_LOG(
			LogTunaSweeperEditor,
			Display,
			TEXT("Added MuzzleSocket=%s, LaserSightSocket=%s, and ShellEjectionSocket=%s to %s."),
			*MuzzleSocketLocation.ToString(),
			*LaserSightSocketLocation.ToString(),
			*ShellEjectionSocketLocation.ToString(),
			*RifleMesh->GetPathName());
		return true;
	}

	bool EnsureShellCasingAssets()
	{
		// Weapon sockets are independent of the shell casing import.  Ensure them first so
		// a pre-existing or read-only casing asset cannot prevent the rifle setup.
		const bool bWeaponSocketsReady = EnsureAssaultRifleWeaponSockets();

		const FString SourceFile = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt/Weapons/SM_WeaponShellCasing.obj")));
		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing shell casing OBJ source: %s"), *SourceFile);
			return false;
		}

		const FString MeshObjectPath = GetAssetObjectPath(WeaponEffectsAssetPath, ShellCasingMeshAssetName);
		UStaticMesh* ShellCasingMesh = LoadObject<UStaticMesh>(nullptr, *MeshObjectPath);
		if (!ShellCasingMesh)
		{
			const FString InterchangeDefaultObjectPath = GetAssetObjectPath(WeaponEffectsAssetPath, TEXT("casing"));
			ShellCasingMesh = FindObject<UStaticMesh>(nullptr, *InterchangeDefaultObjectPath);
		}

		if (!ShellCasingMesh)
		{
			UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
			ImportData->DestinationPath = WeaponEffectsAssetPath;
			ImportData->Filenames.Add(SourceFile);
			ImportData->bReplaceExisting = false;
			ImportData->bSkipReadOnly = true;

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
			for (UObject* ImportedAsset : ImportedAssets)
			{
				if (UStaticMesh* ImportedMesh = Cast<UStaticMesh>(ImportedAsset))
				{
					ShellCasingMesh = ImportedMesh;
					break;
				}
			}
			if (!ShellCasingMesh)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import shell casing OBJ: %s"), *SourceFile);
				return false;
			}
		}

		if (ShellCasingMesh->GetName() != ShellCasingMeshAssetName)
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			TArray<FAssetRenameData> RenameData;
			RenameData.Emplace(ShellCasingMesh, WeaponEffectsAssetPath, ShellCasingMeshAssetName);
			if (!AssetToolsModule.Get().RenameAssets(RenameData))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to rename imported shell casing mesh to: %s"), *MeshObjectPath);
				return false;
			}

			ShellCasingMesh = LoadObject<UStaticMesh>(nullptr, *MeshObjectPath);
			if (!ShellCasingMesh)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load renamed shell casing mesh: %s"), *MeshObjectPath);
				return false;
			}
		}

		const FString MaterialObjectPath = GetAssetObjectPath(WeaponEffectsAssetPath, ShellCasingMaterialAssetName);
		UMaterial* ShellCasingMaterial = LoadObject<UMaterial>(nullptr, *MaterialObjectPath);
		if (!ShellCasingMaterial)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			ShellCasingMaterial = Cast<UMaterial>(AssetToolsModule.Get().CreateAsset(
				ShellCasingMaterialAssetName,
				WeaponEffectsAssetPath,
				UMaterial::StaticClass(),
				MaterialFactory));
			if (!ShellCasingMaterial)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create shell casing material: %s"), *MaterialObjectPath);
				return false;
			}
		}

		ShellCasingMaterial->Modify();
		ShellCasingMaterial->GetExpressionCollection().Empty();
		ShellCasingMaterial->BlendMode = BLEND_Opaque;
		ShellCasingMaterial->SetShadingModel(MSM_DefaultLit);
		ShellCasingMaterial->TwoSided = false;

		UMaterialEditorOnlyData* MaterialEditorOnly = ShellCasingMaterial->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit shell casing material: %s"), *MaterialObjectPath);
			return false;
		}

		UMaterialExpressionConstant3Vector* BrassColor = NewObject<UMaterialExpressionConstant3Vector>(ShellCasingMaterial);
		BrassColor->Material = ShellCasingMaterial;
		BrassColor->Constant = FLinearColor(0.52f, 0.19f, 0.035f, 1.0f);
		BrassColor->MaterialExpressionEditorX = -280;
		BrassColor->MaterialExpressionEditorY = 0;
		ShellCasingMaterial->GetExpressionCollection().AddExpression(BrassColor);

		MaterialEditorOnly->BaseColor.Connect(0, BrassColor);
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.9f;
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.24f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.45f;
		ShellCasingMaterial->PostEditChange();
		ShellCasingMaterial->MarkPackageDirty();
		if (!SaveAsset(ShellCasingMaterial))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save shell casing material: %s"), *MaterialObjectPath);
			return false;
		}

		ShellCasingMesh->Modify();
		ShellCasingMesh->SetMaterial(0, ShellCasingMaterial);
		ShellCasingMesh->PostEditChange();
		ShellCasingMesh->MarkPackageDirty();
		if (!SaveAsset(ShellCasingMesh))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save shell casing mesh: %s"), *MeshObjectPath);
			return false;
		}

		return bWeaponSocketsReady;
	}

	static FAutoConsoleCommand CreateShellCasingAssetsConsoleCommand(
		TEXT("TunaSweeper.CreateShellCasingAssets"),
		TEXT("Imports the TunaSweeper shell casing OBJ, creates its brass material, and ensures rifle muzzle/ejection sockets."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			EnsureShellCasingAssets();
		}));

	bool EnsureTopDownShooterAssets()
	{
		UInputAction* MoveAction = EnsureInputAction(MoveActionName, EInputActionValueType::Axis2D, EInputActionAccumulationBehavior::Cumulative);
		UInputAction* FireAction = EnsureInputAction(FireActionName, EInputActionValueType::Boolean, EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputAction* AimAction = EnsureInputAction(AimActionName, EInputActionValueType::Boolean, EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputMappingContext* MappingContext = EnsureInputMappingContext(MoveAction, FireAction, AimAction);

		UBlueprint* ProjectileBlueprint = EnsureBlueprint(WeaponAssetPath, ProjectileAssetName, ATunaSweeperProjectile::StaticClass());
		UBlueprint* WeaponBlueprint = EnsureBlueprint(WeaponAssetPath, WeaponAssetName, ATunaSweeperWeapon::StaticClass());
		UBlueprint* PlayerBlueprint = EnsureBlueprint(PlayerAssetPath, PlayerAssetName, ATunaSweeperTopDownCharacter::StaticClass());
		UBlueprint* EnemyBlueprint = EnsureBlueprint(EnemyAssetPath, EnemyAssetName, ATunaSweeperEnemyCharacter::StaticClass());
		UBlueprint* GameModeBlueprint = EnsureBlueprint(GameInstanceAssetPath, GameModeAssetName, ATunaSweeperGameMode::StaticClass());

		const bool bAssetsCreated =
			MoveAction &&
			FireAction &&
			AimAction &&
			MappingContext &&
			ProjectileBlueprint &&
			WeaponBlueprint &&
			PlayerBlueprint &&
			EnemyBlueprint &&
			GameModeBlueprint;

		if (!bAssetsCreated)
		{
			return false;
		}

		return ConfigureGameModeBlueprint(GameModeBlueprint, PlayerBlueprint) && SetProjectGameModeToBlueprint();
	}

	bool EnsureMoleBlueprint()
	{
		return EnsureBlueprint(MoleAssetPath, MoleAssetName, ATunaSweeperMoleCompanionActor::StaticClass()) != nullptr;
	}

	bool EnsureEnemyVisualMaterialAssets()
	{
		UMaterial* RedMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemyBodyMaterialAssetName,
			FLinearColor(0.9f, 0.02f, 0.015f, 1.0f),
			0.7f);
		UMaterial* GreenMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemyGreenMaterialAssetName,
			FLinearColor(0.05f, 0.72f, 0.16f, 1.0f),
			0.7f);
		UMaterial* BlueMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemyBlueMaterialAssetName,
			FLinearColor(0.04f, 0.24f, 0.95f, 1.0f),
			0.7f);
		UMaterial* SightlineMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemySightlineMaterialAssetName,
			FLinearColor(0.35f, 0.0f, 0.0f, 1.0f),
			0.75f);
		UMaterial* CardboardMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			CardboardContainerMaterialAssetName,
			FLinearColor(0.64f, 0.42f, 0.22f, 1.0f),
			0.85f);
		UMaterial* WoodMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			WoodContainerMaterialAssetName,
			FLinearColor(0.45f, 0.24f, 0.11f, 1.0f),
			0.8f);
		UMaterial* MetalMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			MetalContainerMaterialAssetName,
			FLinearColor(0.55f, 0.57f, 0.6f, 1.0f),
			0.35f,
			0.65f);
		UMaterial* SupplyMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			SupplyContainerMaterialAssetName,
			FLinearColor(0.22f, 0.32f, 0.24f, 1.0f),
			0.85f);

		return RedMaterial && GreenMaterial && BlueMaterial && SightlineMaterial &&
			CardboardMaterial && WoodMaterial && MetalMaterial && SupplyMaterial;
	}

	UMaterial* EnsureEnemySensorDebugMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(EnemyAssetPath, EnemySensorDebugMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			Material = Cast<UMaterial>(AssetToolsModule.Get().CreateAsset(
				EnemySensorDebugMaterialAssetName, EnemyAssetPath, UMaterial::StaticClass(), MaterialFactory));
			if (!Material)
			{
				return nullptr;
			}
			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			return nullptr;
		}
		UMaterialExpressionVertexColor* VertexColor = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColor->Material = Material;
		VertexColor->MaterialExpressionEditorX = -320;
		Material->GetExpressionCollection().AddExpression(VertexColor);
		MaterialEditorOnly->BaseColor.Connect(0, VertexColor);
		MaterialEditorOnly->EmissiveColor.Connect(0, VertexColor);
		MaterialEditorOnly->Opacity.Connect(4, VertexColor);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	bool EnsureRollingBomberBodyGrayMaterial()
	{
		return EnsureSolidColorMaterial(
			EnemyAssetPath,
			RollingBomberBodyGrayMaterialAssetName,
			FLinearColor(0.5f, 0.5f, 0.5f, 1.0f),
			0.82f,
			0.0f) != nullptr;
	}

	bool EnsureRollingBomberLegMetalMaterial()
	{
		return EnsureSolidColorMaterial(
			EnemyAssetPath,
			RollingBomberLegMetalMaterialAssetName,
			FLinearColor(0.48f, 0.52f, 0.54f, 1.0f),
			0.32f,
			1.0f) != nullptr;
	}

	FVector3f MakeBarrelVertex(float AngleRadians, float Radius, float Height)
	{
		return FVector3f(
			FMath::Cos(AngleRadians) * Radius,
			FMath::Sin(AngleRadians) * Radius,
			Height);
	}

	FVector3f MakeBarrelRadialNormal(const FVector3f& Position)
	{
		const FVector3f Radial(Position.X, Position.Y, 0.0f);
		return Radial.IsNearlyZero() ? FVector3f(1.0f, 0.0f, 0.0f) : Radial.GetSafeNormal();
	}

	FVector3f MakeBarrelRadialTangent(const FVector3f& Position)
	{
		const FVector3f Tangent(-Position.Y, Position.X, 0.0f);
		return Tangent.IsNearlyZero() ? FVector3f(0.0f, 1.0f, 0.0f) : Tangent.GetSafeNormal();
	}

	FVector3f MakeBarrelSafeTangent(const FVector3f& Normal, const FVector3f& PreferredTangent)
	{
		FVector3f Tangent = PreferredTangent - Normal * FVector3f::DotProduct(Normal, PreferredTangent);
		if (Tangent.IsNearlyZero())
		{
			const FVector3f FallbackTangent = FMath::Abs(Normal.Z) < 0.8f
				? FVector3f(0.0f, 0.0f, 1.0f)
				: FVector3f(1.0f, 0.0f, 0.0f);
			Tangent = FallbackTangent - Normal * FVector3f::DotProduct(Normal, FallbackTangent);
		}

		return Tangent.IsNearlyZero() ? FVector3f(1.0f, 0.0f, 0.0f) : Tangent.GetSafeNormal();
	}

	void AddBarrelTriangle(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector2f& UvA,
		const FVector2f& UvB,
		const FVector2f& UvC,
		const FVector3f& SurfaceNormal)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVector3f Positions[] = { A, C, B };
		const FVector2f UVs[] = { UvA, UvC, UvB };
		const FVector3f SurfaceTangent = MakeBarrelSafeTangent(SurfaceNormal, FVector3f(1.0f, 0.0f, 0.0f));

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = SurfaceNormal;
			VertexInstanceTangents[VertexInstanceId] = SurfaceTangent;
			VertexInstanceBinormalSigns[VertexInstanceId] = 1.0f;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void AddBarrelQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector3f& D,
		float U0,
		float U1,
		float V0,
		float V1)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVector3f Positions[] = { A, D, C, B };
		const FVector3f Normals[] = {
			MakeBarrelRadialNormal(A),
			MakeBarrelRadialNormal(D),
			MakeBarrelRadialNormal(C),
			MakeBarrelRadialNormal(B)
		};
		const FVector3f Tangents[] = {
			MakeBarrelRadialTangent(A),
			MakeBarrelRadialTangent(D),
			MakeBarrelRadialTangent(C),
			MakeBarrelRadialTangent(B)
		};
		const FVector2f UVs[] = {
			FVector2f(U0, V0),
			FVector2f(U0, V1),
			FVector2f(U1, V1),
			FVector2f(U1, V0)
		};

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normals[Index];
			VertexInstanceTangents[VertexInstanceId] = MakeBarrelSafeTangent(Normals[Index], Tangents[Index]);
			VertexInstanceBinormalSigns[VertexInstanceId] = 1.0f;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

#if 0 // Removed temporary procedural barrel mesh, texture, and material generation.
	void BuildExplosiveBarrelIntactMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Barrel"));
		const FPolygonGroupID DetailPolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[DetailPolygonGroupId] = FName(TEXT("BarrelDetail"));

		constexpr int32 SegmentCount = 32;
		const float Heights[] = { 0.0f, 6.0f, 13.0f, 25.0f, 58.0f, 95.0f, 107.0f, 114.0f, 120.0f };
		const float Radii[] = { 31.0f, 35.0f, 35.0f, 32.5f, 34.0f, 32.5f, 35.0f, 35.0f, 31.0f };
		constexpr int32 RingCount = UE_ARRAY_COUNT(Heights);
		for (int32 RingIndex = 0; RingIndex < RingCount - 1; ++RingIndex)
		{
			const float V0 = Heights[RingIndex] / Heights[RingCount - 1];
			const float V1 = Heights[RingIndex + 1] / Heights[RingCount - 1];
			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
				const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
				const float Angle0 = U0 * 2.0f * UE_PI;
				const float Angle1 = U1 * 2.0f * UE_PI;
				AddBarrelQuad(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					MakeBarrelVertex(Angle0, Radii[RingIndex], Heights[RingIndex]),
					MakeBarrelVertex(Angle1, Radii[RingIndex], Heights[RingIndex]),
					MakeBarrelVertex(Angle1, Radii[RingIndex + 1], Heights[RingIndex + 1]),
					MakeBarrelVertex(Angle0, Radii[RingIndex + 1], Heights[RingIndex + 1]),
					U0,
					U1,
					V0,
					V1);
			}
		}

		const FVector3f BottomCenter(0.0f, 0.0f, Heights[0]);
		const FVector3f TopCenter(0.0f, 0.0f, Heights[RingCount - 1]);
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			const FVector3f Bottom0 = MakeBarrelVertex(Angle0, Radii[0], Heights[0]);
			const FVector3f Bottom1 = MakeBarrelVertex(Angle1, Radii[0], Heights[0]);
			const FVector3f Top0 = MakeBarrelVertex(Angle0, Radii[RingCount - 1], Heights[RingCount - 1]);
			const FVector3f Top1 = MakeBarrelVertex(Angle1, Radii[RingCount - 1], Heights[RingCount - 1]);
			AddBarrelTriangle(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				BottomCenter,
				Bottom1,
				Bottom0,
				FVector2f(0.5f, 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f),
				FVector3f(0.0f, 0.0f, -1.0f));
			AddBarrelTriangle(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				TopCenter,
				Top0,
				Top1,
				FVector2f(0.5f, 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f),
				FVector3f(0.0f, 0.0f, 1.0f));
		}
	}

	void BuildExplosiveBarrelDestroyedMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Barrel"));
		const FPolygonGroupID DetailPolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[DetailPolygonGroupId] = FName(TEXT("BarrelDetail"));

		constexpr int32 SegmentCount = 32;
		const float BaseRadius = 32.0f;
		const float RimRadius = 35.0f;
		const float BottomHeight = 0.0f;
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const int32 NextSegmentIndex = SegmentIndex + 1;
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(NextSegmentIndex) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			const float TopHeight0 = 20.0f + static_cast<float>((SegmentIndex * 7) % 9) * 1.7f;
			const float TopHeight1 = 20.0f + static_cast<float>((NextSegmentIndex * 7) % 9) * 1.7f;

			AddBarrelQuad(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				MakeBarrelVertex(Angle0, BaseRadius, BottomHeight),
				MakeBarrelVertex(Angle1, BaseRadius, BottomHeight),
				MakeBarrelVertex(Angle1, RimRadius, TopHeight1),
				MakeBarrelVertex(Angle0, RimRadius, TopHeight0),
				U0,
				U1,
				0.0f,
				1.0f);
		}

		const FVector3f BottomCenter(0.0f, 0.0f, BottomHeight);
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			AddBarrelTriangle(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				BottomCenter,
				MakeBarrelVertex(Angle1, BaseRadius, BottomHeight),
				MakeBarrelVertex(Angle0, BaseRadius, BottomHeight),
				FVector2f(0.5f, 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f),
				FVector3f(0.0f, 0.0f, -1.0f));
		}
	}

	void BuildExplosiveBarrelDamagedMeshDescription(FMeshDescription& MeshDescription, float DamageAmount)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);
		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Barrel"));
		const FPolygonGroupID DetailPolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[DetailPolygonGroupId] = FName(TEXT("BarrelDetail"));

		constexpr int32 SegmentCount = 32;
		const float Heights[] = { 0.0f, 6.0f, 13.0f, 25.0f, 58.0f, 95.0f, 107.0f, 114.0f, 120.0f };
		const float Radii[] = { 31.0f, 35.0f, 35.0f, 32.5f, 34.0f, 32.5f, 35.0f, 35.0f, 31.0f };
		for (int32 RingIndex = 0; RingIndex < UE_ARRAY_COUNT(Heights) - 1; ++RingIndex)
		{
			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const float U0 = static_cast<float>(SegmentIndex) / SegmentCount;
				const float U1 = static_cast<float>(SegmentIndex + 1) / SegmentCount;
				auto MakeDamagedVertex = [&](float Angle, float Radius, float Height)
				{
					const float Dent = FMath::Pow(FMath::Max(0.0f, FMath::Cos(Angle - 0.55f)), 5.0f) * DamageAmount;
					const float Crush = 1.0f - Dent * 0.43f;
					FVector3f Vertex = MakeBarrelVertex(Angle, Radius * Crush, Height * (1.0f - DamageAmount * 0.07f));
					Vertex.X += Dent * DamageAmount * 14.0f;
					return Vertex;
				};
				const float Angle0 = U0 * 2.0f * UE_PI;
				const float Angle1 = U1 * 2.0f * UE_PI;
				AddBarrelQuad(MeshDescription, Attributes, PolygonGroupId,
					MakeDamagedVertex(Angle0, Radii[RingIndex], Heights[RingIndex]),
					MakeDamagedVertex(Angle1, Radii[RingIndex], Heights[RingIndex]),
					MakeDamagedVertex(Angle1, Radii[RingIndex + 1], Heights[RingIndex + 1]),
					MakeDamagedVertex(Angle0, Radii[RingIndex + 1], Heights[RingIndex + 1]),
					U0, U1, Heights[RingIndex] / 120.0f, Heights[RingIndex + 1] / 120.0f);
			}
		}

		// Keep the damaged drum closed: the lid follows the same dent profile as the upper wall.
		auto MakeDamagedTopVertex = [DamageAmount](float Angle)
		{
			const float Dent = FMath::Pow(FMath::Max(0.0f, FMath::Cos(Angle - 0.55f)), 5.0f) * DamageAmount;
			const float Crush = 1.0f - Dent * 0.43f;
			FVector3f Vertex = MakeBarrelVertex(Angle, 31.0f * Crush, 120.0f * (1.0f - DamageAmount * 0.07f));
			Vertex.X += Dent * DamageAmount * 14.0f;
			return Vertex;
		};
		const FVector3f TopCenter(DamageAmount * DamageAmount * 6.0f, 0.0f, 118.0f * (1.0f - DamageAmount * 0.07f));
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / SegmentCount;
			const float U1 = static_cast<float>(SegmentIndex + 1) / SegmentCount;
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			AddBarrelTriangle(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				TopCenter,
				MakeDamagedTopVertex(Angle0),
				MakeDamagedTopVertex(Angle1),
				FVector2f(0.5f, 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f),
				FVector3f(0.0f, 0.0f, 1.0f));
		}
	}

	UStaticMesh* EnsureExplosiveBarrelStaticMesh(
		const FString& AssetName,
		UMaterialInterface* BarrelMaterial,
		UMaterialInterface* DetailMaterial,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription)
	{
		if (!BarrelMaterial)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, AssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *InteractionAssetPath, *AssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();
		FMeshDescription MeshDescription;
		BuildMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(BarrelMaterial, FName(TEXT("Barrel"))));
		if (DetailMaterial)
		{
			StaticMesh->GetStaticMaterials().Add(FStaticMaterial(DetailMaterial, FName(TEXT("BarrelDetail"))));
		}

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bFastBuild = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = true;
		BuildParams.bUseHashAsGuid = false;
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams);

		if (StaticMesh->GetNumSourceModels() > 0)
		{
			FMeshBuildSettings& BuildSettings = StaticMesh->GetSourceModel(0).BuildSettings;
			BuildSettings.bRecomputeNormals = false;
			BuildSettings.bRecomputeTangents = false;
			BuildSettings.bComputeWeightedNormals = false;
			BuildSettings.bUseMikkTSpace = false;
		}

		StaticMesh->PostEditChange();
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	FString GetExplosiveBarrelTextureSourcePath(const FString& FileName)
	{
		return GetWorkspaceFilePath(FString::Printf(TEXT("GeneratedImages/ExplosiveBarrel/%s"), *FileName));
	}

	UMaterial* EnsureExplosiveBarrelTexturedMaterial(const FString& MaterialAssetName, UTexture2D* Texture, float Roughness)
	{
		if (!Texture) return nullptr;
		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, MaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
			Material = Cast<UMaterial>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(MaterialAssetName, InteractionAssetPath, UMaterial::StaticClass(), Factory));
			if (!Material) return nullptr;
			FAssetRegistryModule::AssetCreated(Material);
		}
		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		if (!Data) return nullptr;
		auto* Coordinates = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		Coordinates->Material = Material;
		Coordinates->UTiling = 1.0f;
		Coordinates->VTiling = 1.0f;
		Material->GetExpressionCollection().AddExpression(Coordinates);
		auto* Sample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		Sample->Material = Material;
		Sample->ParameterName = TEXT("BarrelTexture");
		Sample->Texture = Texture;
		Sample->SamplerType = SAMPLERTYPE_Color;
		Sample->Coordinates.Connect(0, Coordinates);
		Material->GetExpressionCollection().AddExpression(Sample);
		auto* RoughnessParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		RoughnessParameter->Material = Material;
		RoughnessParameter->ParameterName = TEXT("Roughness");
		RoughnessParameter->DefaultValue = Roughness;
		Material->GetExpressionCollection().AddExpression(RoughnessParameter);
		Data->BaseColor.Connect(0, Sample);
		Data->Roughness.Connect(0, RoughnessParameter);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

#endif

	UNiagaraSystem* EnsureExplosiveBarrelSmokeEffect(const FString& AssetName, float Strength, bool bBlackSmokeOnly = false)
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, AssetName);
		if (UNiagaraSystem* Existing = LoadObject<UNiagaraSystem>(nullptr, *ObjectPath))
		{
			return ConfigureExplosiveBarrelSmokeNiagaraSystem(Existing, Strength, bBlackSmokeOnly) ? Existing : nullptr;
		}
		UNiagaraSystem* Source = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/FX/NS_ExtractionSmoke.NS_ExtractionSmoke"));
		if (!Source) return nullptr;
		UNiagaraSystem* Effect = Cast<UNiagaraSystem>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().DuplicateAsset(AssetName, EffectsAssetPath, Source));
		return Effect && ConfigureExplosiveBarrelSmokeNiagaraSystem(Effect, Strength, bBlackSmokeOnly) ? Effect : nullptr;
	}

	UMaterial* EnsureExplosiveBarrelDamageRadiusMaterial()
	{
		const FString AssetName(TEXT("M_ExplosiveBarrel_DamageRadius"));
		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, AssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			Material = Cast<UMaterial>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(AssetName, InteractionAssetPath, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
			if (!Material) return nullptr;
			FAssetRegistryModule::AssetCreated(Material);
		}
		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_DeferredDecal;
		Material->BlendMode = BLEND_Translucent;
		Material->DecalBlendMode = DBM_Translucent;
		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		if (!Data) return nullptr;
		auto* Coordinates = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		Coordinates->Material = Material;
		Material->GetExpressionCollection().AddExpression(Coordinates);
		auto* Centre = NewObject<UMaterialExpressionConstant2Vector>(Material);
		Centre->Material = Material;
		Centre->R = 0.5f;
		Centre->G = 0.5f;
		Material->GetExpressionCollection().AddExpression(Centre);
		auto* Offset = NewObject<UMaterialExpressionSubtract>(Material);
		Offset->Material = Material;
		Offset->A.Connect(0, Coordinates);
		Offset->B.Connect(0, Centre);
		Material->GetExpressionCollection().AddExpression(Offset);
		auto* Distance = NewObject<UMaterialExpressionLength>(Material);
		Distance->Material = Material;
		Distance->Input.Connect(0, Offset);
		Material->GetExpressionCollection().AddExpression(Distance);
		auto* RadiusScale = NewObject<UMaterialExpressionConstant>(Material);
		RadiusScale->Material = Material;
		RadiusScale->R = 2.0f;
		Material->GetExpressionCollection().AddExpression(RadiusScale);
		auto* NormalizedDistance = NewObject<UMaterialExpressionMultiply>(Material);
		NormalizedDistance->Material = Material;
		NormalizedDistance->A.Connect(0, Distance);
		NormalizedDistance->B.Connect(0, RadiusScale);
		Material->GetExpressionCollection().AddExpression(NormalizedDistance);
		auto* Zero = NewObject<UMaterialExpressionConstant>(Material);
		Zero->Material = Material;
		Zero->R = 0.0f;
		Material->GetExpressionCollection().AddExpression(Zero);
		auto* One = NewObject<UMaterialExpressionConstant>(Material);
		One->Material = Material;
		One->R = 1.0f;
		Material->GetExpressionCollection().AddExpression(One);
		auto* CircleMask = NewObject<UMaterialExpressionIf>(Material);
		CircleMask->Material = Material;
		CircleMask->A.Connect(0, NormalizedDistance);
		CircleMask->ConstB = 1.0f;
		CircleMask->AGreaterThanB.Connect(0, Zero);
		CircleMask->AEqualsB.Connect(0, Zero);
		CircleMask->ALessThanB.Connect(0, One);
		Material->GetExpressionCollection().AddExpression(CircleMask);
		auto* RingMask = NewObject<UMaterialExpressionIf>(Material);
		RingMask->Material = Material;
		RingMask->A.Connect(0, NormalizedDistance);
		RingMask->ConstB = 0.94f;
		RingMask->AGreaterThanB.Connect(0, One);
		RingMask->AEqualsB.Connect(0, One);
		RingMask->ALessThanB.Connect(0, Zero);
		Material->GetExpressionCollection().AddExpression(RingMask);
		auto* InteriorStrength = NewObject<UMaterialExpressionConstant>(Material);
		InteriorStrength->Material = Material;
		InteriorStrength->R = 0.34f;
		Material->GetExpressionCollection().AddExpression(InteriorStrength);
		auto* InteriorOpacity = NewObject<UMaterialExpressionMultiply>(Material);
		InteriorOpacity->Material = Material;
		InteriorOpacity->A.Connect(0, NormalizedDistance);
		InteriorOpacity->B.Connect(0, InteriorStrength);
		Material->GetExpressionCollection().AddExpression(InteriorOpacity);
		auto* CombinedOpacity = NewObject<UMaterialExpressionAdd>(Material);
		CombinedOpacity->Material = Material;
		CombinedOpacity->A.Connect(0, InteriorOpacity);
		CombinedOpacity->B.Connect(0, RingMask);
		Material->GetExpressionCollection().AddExpression(CombinedOpacity);
		auto* ClampedOpacity = NewObject<UMaterialExpressionSaturate>(Material);
		ClampedOpacity->Material = Material;
		ClampedOpacity->Input.Connect(0, CombinedOpacity);
		Material->GetExpressionCollection().AddExpression(ClampedOpacity);
		auto* Mask = NewObject<UMaterialExpressionMultiply>(Material);
		Mask->Material = Material;
		Mask->A.Connect(0, ClampedOpacity);
		Mask->B.Connect(0, CircleMask);
		Material->GetExpressionCollection().AddExpression(Mask);
		auto* Color = NewObject<UMaterialExpressionVectorParameter>(Material);
		Color->Material = Material;
		Color->ParameterName = TEXT("PreviewColor");
		Color->DefaultValue = FLinearColor(1.0f, 0.12f, 0.02f, 1.0f);
		Material->GetExpressionCollection().AddExpression(Color);
		auto* Opacity = NewObject<UMaterialExpressionScalarParameter>(Material);
		Opacity->Material = Material;
		Opacity->ParameterName = TEXT("PreviewOpacity");
		Opacity->DefaultValue = 0.22f;
		Material->GetExpressionCollection().AddExpression(Opacity);
		auto* FinalOpacity = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacity->Material = Material;
		FinalOpacity->A.Connect(0, Mask);
		FinalOpacity->B.Connect(0, Opacity);
		Material->GetExpressionCollection().AddExpression(FinalOpacity);
		Data->BaseColor.Connect(0, Color);
		Data->Opacity.Connect(0, FinalOpacity);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	bool ConfigureExplosiveBarrelBlueprint(UBlueprint* ExplosiveBarrelBlueprint)
	{
		if (!ExplosiveBarrelBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(ExplosiveBarrelBlueprint);

		ATunaSweeperExplosiveBarrelActor* Defaults = ExplosiveBarrelBlueprint->GeneratedClass
			? Cast<ATunaSweeperExplosiveBarrelActor>(ExplosiveBarrelBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(ExplosiveBarrelBlueprint));
			return false;
		}

		ExplosiveBarrelBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureExplosiveBarrelDefaults(
			NAME_None,
			30.0f,
			TSoftObjectPtr<UStaticMesh>(),
			TSoftObjectPtr<UStaticMesh>(),
			TSoftObjectPtr<UNiagaraSystem>(),
			TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>(
				FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperLocalExplosionEffectActor"))),
			420.0f,
			0.72f);
		FBlueprintEditorUtils::MarkBlueprintAsModified(ExplosiveBarrelBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ExplosiveBarrelBlueprint);
		ExplosiveBarrelBlueprint->MarkPackageDirty();
		return SaveAsset(ExplosiveBarrelBlueprint);
	}

	bool EnsureExplosiveBarrelAssets()
	{
		UNiagaraSystem* LightSmoke = EnsureExplosiveBarrelSmokeEffect(TEXT("NS_ExplosiveBarrel_SmokeLight"), 0.65f);
		UNiagaraSystem* HeavySmoke = EnsureExplosiveBarrelSmokeEffect(TEXT("NS_ExplosiveBarrel_SmokeHeavy"), 1.05f);
		UNiagaraSystem* BurningEffect = EnsureExplosiveBarrelSmokeEffect(TEXT("NS_ExplosiveBarrel_Burning"), 0.45f, true);
		UMaterial* DamageRadiusMaterial = EnsureExplosiveBarrelDamageRadiusMaterial();
		UBlueprint* ExplosiveBarrelBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			ExplosiveBarrelAssetName,
			ATunaSweeperExplosiveBarrelActor::StaticClass());

		return DamageRadiusMaterial && LightSmoke && HeavySmoke && BurningEffect &&
			ConfigureExplosiveBarrelBlueprint(ExplosiveBarrelBlueprint);
	}

	bool EnsureCookableChickenBlueprint()
	{
		UBlueprint* CookableChickenBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			CookableChickenAssetName,
			ATunaSweeperCookableChickenActor::StaticClass());
		if (!CookableChickenBlueprint)
		{
			return false;
		}

		// Mesh properties deliberately remain empty for content authors to configure.
		FKismetEditorUtilities::CompileBlueprint(CookableChickenBlueprint);
		CookableChickenBlueprint->MarkPackageDirty();
		return SaveAsset(CookableChickenBlueprint);
	}

	UGeometryCollection* EnsureBreakableAppleCrateGeometryCollection()
	{
		const FString PackageName = FString::Printf(
			TEXT("%s/%s"),
			*InteractionAssetPath,
			*CrateGeometryCollectionAssetName);
		const FString ObjectPath = FString::Printf(
			TEXT("%s.%s"),
			*PackageName,
			*CrateGeometryCollectionAssetName);

		UGeometryCollection* GeometryCollection = LoadObject<UGeometryCollection>(nullptr, *ObjectPath);
		const bool bCreatedGeometryCollection = GeometryCollection == nullptr;
		UPackage* Package = GeometryCollection ? GeometryCollection->GetOutermost() : CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create package for %s."), *ObjectPath);
			return nullptr;
		}

		if (!GeometryCollection)
		{
			GeometryCollection = NewObject<UGeometryCollection>(
				Package,
				*CrateGeometryCollectionAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
		}
		if (!GeometryCollection)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		UStaticMesh* SourceMesh = LoadObject<UStaticMesh>(
			nullptr,
			TEXT("/Game/Nature/Wood/SM_CrateB.SM_CrateB"));
		if (!SourceMesh)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load SM_CrateB for crate fracture."));
			return nullptr;
		}

		FManagedArrayCollection Collection;
		TArray<TObjectPtr<UMaterialInterface>> Materials;
		TArray<FGeometryCollectionAutoInstanceMesh> AutoInstanceMeshes;
		TObjectPtr<UStaticMesh> SourceMeshObject = SourceMesh;
		FGeometryCollectionEngineConversion::ConvertStaticMeshToGeometryCollection(
			SourceMeshObject,
			FTransform::Identity,
			Collection,
			Materials,
			AutoInstanceMeshes,
			false,
			false);

		FDataflowTransformSelection TransformSelection;
		TransformSelection.InitializeFromCollection(Collection, true);

		FUniformFractureSettings FractureSettings{};
		FractureSettings.Transform = FTransform::Identity;
		FractureSettings.MinVoronoiSites = 36;
		FractureSettings.MaxVoronoiSites = 48;
		FractureSettings.InternalMaterialID = 0;
		FractureSettings.RandomSeed = 5707;
		FractureSettings.ChanceToFracture = 1.0f;
		FractureSettings.GroupFracture = false;
		FractureSettings.SplitIslands = false;
		FractureSettings.Grout = 0.0f;
		FractureSettings.NoiseSettings.Amplitude = 0.0f;
		FractureSettings.NoiseSettings.Frequency = 0.1f;
		FractureSettings.NoiseSettings.Octaves = 1;
		FractureSettings.NoiseSettings.PointSpacing = 2.0f;
		FractureSettings.NoiseSettings.Lacunarity = 2.0f;
		FractureSettings.NoiseSettings.Persistence = 0.5f;
		FractureSettings.AddSamplesForCollision = true;
		FractureSettings.CollisionSampleSpacing = 8.0f;

		const int32 FractureResult = FFractureEngineFracturing::UniformFracture(
			Collection,
			TransformSelection,
			FractureSettings);
		if (FractureResult == INDEX_NONE)
		{
			UE_LOG(LogTunaSweeperEditor, Warning, TEXT("Uniform fracture returned no result for %s."), *ObjectPath);
		}

		GeometryCollection->Modify();
		GeometryCollection->ResetFrom(Collection, Materials, false);
		GeometryCollection->SetAutoInstanceMeshes(AutoInstanceMeshes);
		GeometryCollection->EnableClustering = false;
		GeometryCollection->ClusterGroupIndex = 0;
		GeometryCollection->MaxClusterLevel = 0;
		GeometryCollection->DamageModel = EDamageModelTypeEnum::Chaos_Damage_Model_UserDefined_Damage_Threshold;
		GeometryCollection->DamageThreshold = { 0.0f };
		GeometryCollection->bUseSizeSpecificDamageThreshold = false;
		GeometryCollection->bUseMaterialDamageModifiers = false;
		GeometryCollection->PerClusterOnlyDamageThreshold = false;
		GeometryCollection->bMassAsDensity = false;
		GeometryCollection->Mass = 12.0f;
		GeometryCollection->MinimumMassClamp = 0.05f;
		GeometryCollection->bImportCollisionFromSource = false;
		GeometryCollection->bOptimizeConvexes = true;
		GeometryCollection->SizeSpecificData.Reset();
		FGeometryCollectionSizeSpecificData SizeData =
			UGeometryCollection::GeometryCollectionSizeSpecificDataDefaults();
		SizeData.DamageThreshold = 0;
		if (SizeData.CollisionShapes.Num() > 0)
		{
			SizeData.CollisionShapes[0].CollisionType = ECollisionTypeEnum::Chaos_Volumetric;
			SizeData.CollisionShapes[0].ImplicitType = EImplicitTypeEnum::Chaos_Implicit_Convex;
			SizeData.CollisionShapes[0].CollisionObjectReductionPercentage = 0.0f;
		}
		GeometryCollection->SizeSpecificData.Add(SizeData);

#if WITH_EDITORONLY_DATA
		TArray<TObjectPtr<UMaterialInterface>> SourceMaterials;
		for (const FStaticMaterial& StaticMaterial : SourceMesh->GetStaticMaterials())
		{
			SourceMaterials.Add(StaticMaterial.MaterialInterface);
		}
		GeometryCollection->GeometrySource.Reset();
		GeometryCollection->GeometrySource.Emplace(
			FSoftObjectPath(SourceMesh),
			FTransform::Identity,
			SourceMaterials,
			false,
			false);
		GeometryCollection->SetRootProxiesFromGeometrySources();
#endif

		if (GeometryCollection->GetGeometryCollection().IsValid())
		{
			::GeometryCollection::GenerateTemporaryGuids(
				GeometryCollection->GetGeometryCollection().Get(),
				0,
				true);
		}
		GeometryCollection->InvalidateCollection();
		GeometryCollection->CreateSimulationData();
		GeometryCollection->RebuildRenderData();
		GeometryCollection->MarkPackageDirty();
		Package->SetDirtyFlag(true);

		if (bCreatedGeometryCollection)
		{
			FAssetRegistryModule::AssetCreated(GeometryCollection);
		}

		return SaveAsset(GeometryCollection) ? GeometryCollection : nullptr;
	}

	bool EnsureBreakableAppleCrateAssets()
	{
		UGeometryCollection* CrateGeometryCollection = EnsureBreakableAppleCrateGeometryCollection();
		UBlueprint* AppleBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			PhysicsAppleAssetName,
			ATunaSweeperPhysicsAppleActor::StaticClass());
		UBlueprint* CrateFragmentBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			CrateFragmentAssetName,
			ATunaSweeperPhysicsCrateFragmentActor::StaticClass());
		UBlueprint* AppleCrateBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			BreakableAppleCrateAssetName,
			ATunaSweeperBreakableAppleCrateActor::StaticClass());
		if (!CrateGeometryCollection || !AppleBlueprint || !CrateFragmentBlueprint || !AppleCrateBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(AppleBlueprint);
		ATunaSweeperPhysicsAppleActor* AppleDefaults = AppleBlueprint->GeneratedClass
			? Cast<ATunaSweeperPhysicsAppleActor>(AppleBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!AppleDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(AppleBlueprint));
			return false;
		}

		AppleBlueprint->Modify();
		AppleDefaults->Modify();
		AppleDefaults->ConfigurePhysicsAppleDefaults(
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/Props/Apple/SM_Apple.SM_Apple"))),
			14.0f,
			1.0f,
			12.0f);
		FBlueprintEditorUtils::MarkBlueprintAsModified(AppleBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AppleBlueprint);
		AppleBlueprint->MarkPackageDirty();

		FKismetEditorUtilities::CompileBlueprint(CrateFragmentBlueprint);
		ATunaSweeperPhysicsCrateFragmentActor* FragmentDefaults = CrateFragmentBlueprint->GeneratedClass
			? Cast<ATunaSweeperPhysicsCrateFragmentActor>(CrateFragmentBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!FragmentDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(CrateFragmentBlueprint));
			return false;
		}

		CrateFragmentBlueprint->Modify();
		FragmentDefaults->Modify();
		FragmentDefaults->ConfigureCrateFragmentDefaults(
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube"))),
			nullptr,
			FVector(6.0f, 18.0f, 44.0f),
			8.0f);
		FBlueprintEditorUtils::MarkBlueprintAsModified(CrateFragmentBlueprint);
		FKismetEditorUtilities::CompileBlueprint(CrateFragmentBlueprint);
		CrateFragmentBlueprint->MarkPackageDirty();

		FKismetEditorUtilities::CompileBlueprint(AppleCrateBlueprint);
		ATunaSweeperBreakableAppleCrateActor* CrateDefaults = AppleCrateBlueprint->GeneratedClass
			? Cast<ATunaSweeperBreakableAppleCrateActor>(AppleCrateBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!CrateDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(AppleCrateBlueprint));
			return false;
		}

		TSubclassOf<ATunaSweeperPhysicsAppleActor> AppleActorClass = ATunaSweeperPhysicsAppleActor::StaticClass();
		if (AppleBlueprint->GeneratedClass &&
			AppleBlueprint->GeneratedClass->IsChildOf(ATunaSweeperPhysicsAppleActor::StaticClass()))
		{
			AppleActorClass = AppleBlueprint->GeneratedClass;
		}
		TSubclassOf<ATunaSweeperPhysicsCrateFragmentActor> CrateFragmentActorClass =
			ATunaSweeperPhysicsCrateFragmentActor::StaticClass();
		if (CrateFragmentBlueprint->GeneratedClass &&
			CrateFragmentBlueprint->GeneratedClass->IsChildOf(ATunaSweeperPhysicsCrateFragmentActor::StaticClass()))
		{
			CrateFragmentActorClass = CrateFragmentBlueprint->GeneratedClass;
		}
		AppleCrateBlueprint->Modify();
		CrateDefaults->Modify();
		CrateDefaults->ConfigureBreakableAppleCrateDefaults(
			FName(TEXT("TS_BreakableAppleCrate_Default")),
			1.0f,
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Nature/Wood/SM_CrateB.SM_CrateB"))),
			TSoftObjectPtr<UGeometryCollection>(
				FSoftObjectPath(TEXT("/Game/Interaction/GC_CrateB_Fractured.GC_CrateB_Fractured"))),
			AppleActorClass,
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/Props/Apple/SM_Apple.SM_Apple"))),
			CrateFragmentActorClass,
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube"))));
		FBlueprintEditorUtils::MarkBlueprintAsModified(AppleCrateBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AppleCrateBlueprint);
		AppleCrateBlueprint->MarkPackageDirty();

		return SaveAsset(AppleBlueprint) && SaveAsset(CrateFragmentBlueprint) && SaveAsset(AppleCrateBlueprint);
	}

	UMaterial* EnsureTomatoFleshInteriorMaterial()
	{
		const FString MaterialAssetPath = TEXT("/Game/Meshes/Props/TomatoHead/Materials");
		const FString ObjectPath = GetAssetObjectPath(MaterialAssetPath, TomatoFleshMaterialAssetName);
		UTexture2D* FleshTexture = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/Game/Meshes/Props/TomatoHead/Textures/T_TomatoFlesh.T_TomatoFlesh"));
		if (!FleshTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing tomato flesh texture for %s."), *ObjectPath);
			return nullptr;
		}

		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			Material = Cast<UMaterial>(
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(
					TomatoFleshMaterialAssetName,
					MaterialAssetPath,
					UMaterial::StaticClass(),
					NewObject<UMaterialFactoryNew>()));
			if (!Material)
			{
				return nullptr;
			}
			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = true;
		UMaterialEditorOnlyData* MaterialData = Material->GetEditorOnlyData();
		if (!MaterialData)
		{
			return nullptr;
		}

		auto* Coordinates = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		Coordinates->Material = Material;
		Material->GetExpressionCollection().AddExpression(Coordinates);
		auto* FleshSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		FleshSample->Material = Material;
		FleshSample->ParameterName = TEXT("TomatoFleshTexture");
		FleshSample->Texture = FleshTexture;
		FleshSample->SamplerType = SAMPLERTYPE_Color;
		FleshSample->Coordinates.Connect(0, Coordinates);
		Material->GetExpressionCollection().AddExpression(FleshSample);
		auto* Roughness = NewObject<UMaterialExpressionScalarParameter>(Material);
		Roughness->Material = Material;
		Roughness->ParameterName = TEXT("Roughness");
		Roughness->DefaultValue = 0.38f;
		Material->GetExpressionCollection().AddExpression(Roughness);
		auto* Specular = NewObject<UMaterialExpressionScalarParameter>(Material);
		Specular->Material = Material;
		Specular->ParameterName = TEXT("Specular");
		Specular->DefaultValue = 0.52f;
		Material->GetExpressionCollection().AddExpression(Specular);
		MaterialData->BaseColor.Connect(0, FleshSample);
		MaterialData->Roughness.Connect(0, Roughness);
		MaterialData->Specular.Connect(0, Specular);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	UMaterial* EnsureTomatoGooMaterial(const FString& AssetName, bool bDeferredDecal)
	{
		const FString MaterialAssetPath = TEXT("/Game/Meshes/Props/TomatoHead/Materials");
		const FString ObjectPath = GetAssetObjectPath(MaterialAssetPath, AssetName);
		UTexture2D* FleshTexture = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/Game/Meshes/Props/TomatoHead/Textures/T_TomatoFlesh.T_TomatoFlesh"));
		if (!FleshTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing tomato flesh texture for %s."), *ObjectPath);
			return nullptr;
		}

		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			Material = Cast<UMaterial>(
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(
					AssetName,
					MaterialAssetPath,
					UMaterial::StaticClass(),
					NewObject<UMaterialFactoryNew>()));
			if (!Material)
			{
				return nullptr;
			}
			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = bDeferredDecal ? MD_DeferredDecal : MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->DecalBlendMode = bDeferredDecal ? DBM_Translucent : DBM_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bUsedWithNiagaraSprites = !bDeferredDecal;
		UMaterialEditorOnlyData* MaterialData = Material->GetEditorOnlyData();
		if (!MaterialData)
		{
			return nullptr;
		}

		auto* Coordinates = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		Coordinates->Material = Material;
		Material->GetExpressionCollection().AddExpression(Coordinates);
		auto* FleshSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		FleshSample->Material = Material;
		FleshSample->ParameterName = TEXT("TomatoFleshTexture");
		FleshSample->Texture = FleshTexture;
		FleshSample->SamplerType = SAMPLERTYPE_Color;
		FleshSample->Coordinates.Connect(0, Coordinates);
		Material->GetExpressionCollection().AddExpression(FleshSample);
		auto* Center = NewObject<UMaterialExpressionConstant2Vector>(Material);
		Center->Material = Material;
		Center->R = 0.5f;
		Center->G = 0.5f;
		Material->GetExpressionCollection().AddExpression(Center);
		auto* Offset = NewObject<UMaterialExpressionSubtract>(Material);
		Offset->Material = Material;
		Offset->A.Connect(0, Coordinates);
		Offset->B.Connect(0, Center);
		Material->GetExpressionCollection().AddExpression(Offset);
		auto* Distance = NewObject<UMaterialExpressionLength>(Material);
		Distance->Material = Material;
		Distance->Input.Connect(0, Offset);
		Material->GetExpressionCollection().AddExpression(Distance);
		auto* RadiusScale = NewObject<UMaterialExpressionConstant>(Material);
		RadiusScale->Material = Material;
		RadiusScale->R = 2.0f;
		Material->GetExpressionCollection().AddExpression(RadiusScale);
		auto* NormalizedDistance = NewObject<UMaterialExpressionMultiply>(Material);
		NormalizedDistance->Material = Material;
		NormalizedDistance->A.Connect(0, Distance);
		NormalizedDistance->B.Connect(0, RadiusScale);
		Material->GetExpressionCollection().AddExpression(NormalizedDistance);
		auto* OneMinusDistance = NewObject<UMaterialExpressionOneMinus>(Material);
		OneMinusDistance->Material = Material;
		OneMinusDistance->Input.Connect(0, NormalizedDistance);
		Material->GetExpressionCollection().AddExpression(OneMinusDistance);
		auto* CircleMask = NewObject<UMaterialExpressionSaturate>(Material);
		CircleMask->Material = Material;
		CircleMask->Input.Connect(0, OneMinusDistance);
		Material->GetExpressionCollection().AddExpression(CircleMask);

		if (bDeferredDecal)
		{
			MaterialData->BaseColor.Connect(0, FleshSample);
		}
		else
		{
			MaterialData->EmissiveColor.Connect(0, FleshSample);
		}
		MaterialData->Opacity.Connect(0, CircleMask);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	bool ConfigureTomatoStickySplatterSystem(UNiagaraSystem* System, UMaterialInterface* ParticleMaterial)
	{
		if (!System || !ParticleMaterial)
		{
			return false;
		}

		bool bConfiguredSpriteRenderer = false;
		System->Modify();
		for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			if (FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData())
			{
				EmitterData->bLocalSpace = false;
				EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
				EmitterData->FixedBounds = FBox(FVector(-260.0f, -260.0f, -80.0f), FVector(260.0f, 260.0f, 300.0f));
				for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
				{
					if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
					{
						SpriteRenderer->Modify();
						SpriteRenderer->Material = ParticleMaterial;
						bConfiguredSpriteRenderer = true;
					}
				}
			}
		}
		System->InvalidateCachedData();
		System->RequestCompile(true);
		System->PollForCompilationComplete(true);
		System->PostEditChange();
		System->MarkPackageDirty();
		return bConfiguredSpriteRenderer && SaveAsset(System);
	}

	UNiagaraSystem* EnsureTomatoStickySplatterSystem()
	{
		UMaterial* ParticleMaterial = EnsureTomatoGooMaterial(TomatoGooParticleMaterialAssetName, false);
		if (!ParticleMaterial)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, TomatoStickySplatterSystemAssetName);
		UNiagaraSystem* Effect = LoadObject<UNiagaraSystem>(nullptr, *ObjectPath);
		if (!Effect)
		{
			UNiagaraSystem* Source = LoadObject<UNiagaraSystem>(
				nullptr,
				TEXT("/Game/BallisticsVFX/Particles/Impacts/DynamicImpacts/_generic/NS_SizzleImpact_Liquid.NS_SizzleImpact_Liquid"));
			if (!Source)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing liquid Niagara source for %s."), *ObjectPath);
				return nullptr;
			}
			Effect = Cast<UNiagaraSystem>(
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().DuplicateAsset(
					TomatoStickySplatterSystemAssetName,
					EffectsAssetPath,
					Source));
		}

		return ConfigureTomatoStickySplatterSystem(Effect, ParticleMaterial) ? Effect : nullptr;
	}

	UGeometryCollection* EnsureBreakableTomatoGeometryCollection()
	{
		const FString PackageName = FString::Printf(TEXT("%s/%s"), *InteractionAssetPath, *TomatoGeometryCollectionAssetName);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *TomatoGeometryCollectionAssetName);
		UGeometryCollection* GeometryCollection = LoadObject<UGeometryCollection>(nullptr, *ObjectPath);
		const bool bCreatedGeometryCollection = GeometryCollection == nullptr;
		UPackage* Package = GeometryCollection ? GeometryCollection->GetOutermost() : CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}
		if (!GeometryCollection)
		{
			GeometryCollection = NewObject<UGeometryCollection>(
				Package,
				*TomatoGeometryCollectionAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
		}

		UStaticMesh* SourceMesh = LoadObject<UStaticMesh>(
			nullptr,
			TEXT("/Game/Meshes/Props/TomatoHead/SM_Tomato.SM_Tomato"));
		UMaterial* FleshMaterial = EnsureTomatoFleshInteriorMaterial();
		if (!GeometryCollection || !SourceMesh || !FleshMaterial)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to prepare tomato fracture source assets."));
			return nullptr;
		}

		FManagedArrayCollection Collection;
		TArray<TObjectPtr<UMaterialInterface>> Materials;
		TArray<FGeometryCollectionAutoInstanceMesh> AutoInstanceMeshes;
		TObjectPtr<UStaticMesh> SourceMeshObject = SourceMesh;
		FGeometryCollectionEngineConversion::ConvertStaticMeshToGeometryCollection(
			SourceMeshObject,
			FTransform::Identity,
			Collection,
			Materials,
			AutoInstanceMeshes,
			false,
			false);
		const int32 FleshMaterialId = Materials.AddUnique(FleshMaterial);

		FDataflowTransformSelection TransformSelection;
		TransformSelection.InitializeFromCollection(Collection, true);
		FUniformFractureSettings FractureSettings{};
		FractureSettings.Transform = FTransform::Identity;
		FractureSettings.MinVoronoiSites = 10;
		FractureSettings.MaxVoronoiSites = 14;
		FractureSettings.InternalMaterialID = FleshMaterialId;
		FractureSettings.RandomSeed = 7162026;
		FractureSettings.ChanceToFracture = 1.0f;
		FractureSettings.GroupFracture = false;
		FractureSettings.SplitIslands = false;
		FractureSettings.Grout = 0.0f;
		FractureSettings.NoiseSettings.Amplitude = 0.0f;
		FractureSettings.AddSamplesForCollision = true;
		FractureSettings.CollisionSampleSpacing = 6.0f;
		FFractureEngineFracturing::UniformFracture(Collection, TransformSelection, FractureSettings);

		GeometryCollection->Modify();
		GeometryCollection->ResetFrom(Collection, Materials, false);
		GeometryCollection->SetAutoInstanceMeshes(AutoInstanceMeshes);
		GeometryCollection->EnableClustering = false;
		GeometryCollection->ClusterGroupIndex = 0;
		GeometryCollection->MaxClusterLevel = 0;
		GeometryCollection->DamageModel = EDamageModelTypeEnum::Chaos_Damage_Model_UserDefined_Damage_Threshold;
		GeometryCollection->DamageThreshold = { 0.0f };
		GeometryCollection->bUseSizeSpecificDamageThreshold = false;
		GeometryCollection->bUseMaterialDamageModifiers = false;
		GeometryCollection->PerClusterOnlyDamageThreshold = false;
		GeometryCollection->bMassAsDensity = false;
		GeometryCollection->Mass = 1.8f;
		GeometryCollection->MinimumMassClamp = 0.02f;
		GeometryCollection->bImportCollisionFromSource = false;
		GeometryCollection->bOptimizeConvexes = true;
		GeometryCollection->SizeSpecificData.Reset();
		FGeometryCollectionSizeSpecificData SizeData = UGeometryCollection::GeometryCollectionSizeSpecificDataDefaults();
		SizeData.DamageThreshold = 0;
		if (SizeData.CollisionShapes.Num() > 0)
		{
			SizeData.CollisionShapes[0].CollisionType = ECollisionTypeEnum::Chaos_Volumetric;
			SizeData.CollisionShapes[0].ImplicitType = EImplicitTypeEnum::Chaos_Implicit_Convex;
			SizeData.CollisionShapes[0].CollisionObjectReductionPercentage = 0.0f;
		}
		GeometryCollection->SizeSpecificData.Add(SizeData);

#if WITH_EDITORONLY_DATA
		TArray<TObjectPtr<UMaterialInterface>> SourceMaterials;
		for (const FStaticMaterial& StaticMaterial : SourceMesh->GetStaticMaterials())
		{
			SourceMaterials.Add(StaticMaterial.MaterialInterface);
		}
		GeometryCollection->GeometrySource.Reset();
		GeometryCollection->GeometrySource.Emplace(FSoftObjectPath(SourceMesh), FTransform::Identity, SourceMaterials, false, false);
		GeometryCollection->SetRootProxiesFromGeometrySources();
#endif

		if (GeometryCollection->GetGeometryCollection().IsValid())
		{
			::GeometryCollection::GenerateTemporaryGuids(GeometryCollection->GetGeometryCollection().Get(), 0, true);
		}
		GeometryCollection->InvalidateCollection();
		GeometryCollection->CreateSimulationData();
		GeometryCollection->RebuildRenderData();
		GeometryCollection->MarkPackageDirty();
		Package->SetDirtyFlag(true);
		if (bCreatedGeometryCollection)
		{
			FAssetRegistryModule::AssetCreated(GeometryCollection);
		}
		return SaveAsset(GeometryCollection) ? GeometryCollection : nullptr;
	}

	bool EnsureBreakableTomatoAssets()
	{
		if (!EnsureImpactPhysicalMaterialAssets())
		{
			return false;
		}
		UGeometryCollection* TomatoGeometryCollection = EnsureBreakableTomatoGeometryCollection();
		UMaterial* GooSplatMaterial = EnsureTomatoGooMaterial(TomatoGooSplatMaterialAssetName, true);
		UNiagaraSystem* StickySplatterSystem = EnsureTomatoStickySplatterSystem();
		UBlueprint* TomatoBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			BreakableTomatoAssetName,
			ATunaSweeperBreakableTomatoActor::StaticClass());
		if (!TomatoGeometryCollection || !GooSplatMaterial || !StickySplatterSystem || !TomatoBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(TomatoBlueprint);
		ATunaSweeperBreakableTomatoActor* TomatoDefaults = TomatoBlueprint->GeneratedClass
			? Cast<ATunaSweeperBreakableTomatoActor>(TomatoBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!TomatoDefaults)
		{
			return false;
		}
		TomatoBlueprint->Modify();
		TomatoDefaults->Modify();
		TomatoDefaults->ConfigureBreakableTomatoDefaults(
			FName(TEXT("TS_BreakableTomato_Default")),
			1.0f,
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/Props/TomatoHead/SM_Tomato.SM_Tomato"))),
			TSoftObjectPtr<UGeometryCollection>(FSoftObjectPath(TEXT("/Game/Interaction/GC_Tomato_Fractured.GC_Tomato_Fractured"))),
			TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Effects/NS_Tomato_StickySplatter.NS_Tomato_StickySplatter"))),
			TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Meshes/Props/TomatoHead/Materials/M_TomatoGooSplat.M_TomatoGooSplat"))),
			TSoftObjectPtr<UPhysicalMaterial>(FSoftObjectPath(TEXT("/Game/Physics/PhysicalMaterials/PM_Flesh.PM_Flesh"))),
			FVector(34.0f, 34.0f, 34.0f),
			FVector(0.0f, 0.0f, 34.0f));
		FBlueprintEditorUtils::MarkBlueprintAsModified(TomatoBlueprint);
		FKismetEditorUtilities::CompileBlueprint(TomatoBlueprint);
		TomatoBlueprint->MarkPackageDirty();
		return SaveAsset(TomatoBlueprint);
	}

	bool EnsureSharedVoxelMeshAssets()
	{
		UMaterial* VoxelMaterial = EnsureVoxelVertexColorMaterial();
		if (!VoxelMaterial)
		{
			return false;
		}

		UStaticMesh* EnemyBodyMesh = EnsureVoxelStaticMeshAsset(
			EnemyAssetPath,
			EnemyVoxelBodyMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(100.0f, 100.0f, 100.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendEnemyBodyVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		UStaticMesh* EnemyForwardMarkerMesh = EnsureVoxelStaticMeshAsset(
			EnemyAssetPath,
			EnemyVoxelForwardMarkerMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(70.0f, 28.0f, 18.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendEnemyForwardMarkerVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		UStaticMesh* BrokenBridgeMesh = EnsureVoxelStaticMeshAsset(
			InteractionAssetPath,
			BrokenBridgeVoxelMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(540.0f, 110.0f, 80.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendBrokenBridgeVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		UStaticMesh* RepairedBridgeMesh = EnsureVoxelStaticMeshAsset(
			InteractionAssetPath,
			RepairedBridgeVoxelMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(540.0f, 110.0f, 80.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendRepairedBridgeVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		return EnemyBodyMesh && EnemyForwardMarkerMesh && BrokenBridgeMesh && RepairedBridgeMesh;
	}

	bool EnsureCoverPointAssets()
	{
		UBlueprint* CoverPointBlueprint = EnsureBlueprint(
			CoverAssetPath,
			CoverPointAssetName,
			ATunaSweeperCoverPointActor::StaticClass());

		return CoverPointBlueprint != nullptr;
	}

}
