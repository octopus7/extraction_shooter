#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{
	FString GetItemIconSourcePath(const FString& IconAssetName)
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("GeneratedImages"),
			TEXT("ItemIcons"),
			TEXT("Split"),
			IconAssetName + TEXT(".png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	FString GetGeneratedUiImageSourcePath(const FString& ImageFileName)
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("GeneratedImages"),
			TEXT("UI"),
			ImageFileName));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	void ConfigureImportedIconTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->SRGB = true;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	void ConfigureImportedUiTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->SRGB = true;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	void ConfigureImportedWorldTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_World;
		Texture->SRGB = true;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	void ConfigureImportedEffectTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->SRGB = true;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	void ConfigureImportedMaskTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_Masks;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->SRGB = false;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	bool ImportWorldTexture(
		const FString& InSourceFile,
		const FString& DestinationPath,
		const FString& AssetName,
		UTexture2D** OutTexture = nullptr)
	{
		if (OutTexture)
		{
			*OutTexture = nullptr;
		}

		FString SourceFile = InSourceFile;
		FPaths::NormalizeFilename(SourceFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		FPaths::CollapseRelativeDirectories(SourceFile);

		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing world texture source: %s"), *SourceFile);
			return false;
		}

		if (AssetName.IsEmpty() || !FPackageName::IsValidLongPackageName(DestinationPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Invalid world texture destination: path=%s asset=%s"), *DestinationPath, *AssetName);
			return false;
		}

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		FString ImportFile = SourceFile;
		if (FPaths::GetBaseFilename(SourceFile) != AssetName)
		{
			const FString ImportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TunaSweeperWorldTextureImport"));
			IFileManager::Get().MakeDirectory(*ImportDirectory, true);
			ImportFile = FPaths::Combine(ImportDirectory, AssetName + TEXT(".") + FPaths::GetExtension(SourceFile));
			if (IFileManager::Get().Copy(*ImportFile, *SourceFile, true, true) != COPY_OK)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to stage world texture import source: %s -> %s"), *SourceFile, *ImportFile);
				return false;
			}
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(ImportFile);
		ImportData->bReplaceExisting = true;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import world texture: %s"), *ImportFile);
			return false;
		}

		UTexture2D* ImportedTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!ImportedTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported world texture: %s"), *ObjectPath);
			return false;
		}

		ConfigureImportedWorldTexture(ImportedTexture);
		if (OutTexture)
		{
			*OutTexture = ImportedTexture;
		}
		return true;
	}

	FString GetRollingBomberChargeCylinderMaskSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("GeneratedImages"),
			TEXT("Effects"),
			TEXT("T_RollingBomberChargeCylinderMask.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	void AddRollingBomberChargeCylinderQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		float Angle0,
		float Angle1,
		float U0,
		float U1)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		constexpr float HalfLength = 50.0f;
		constexpr float Radius = 50.0f;
		const FVector3f Radial0(0.0f, FMath::Cos(Angle0), FMath::Sin(Angle0));
		const FVector3f Radial1(0.0f, FMath::Cos(Angle1), FMath::Sin(Angle1));
		const FVector3f Positions[] = {
			FVector3f(-HalfLength, Radial0.Y * Radius, Radial0.Z * Radius),
			FVector3f(-HalfLength, Radial1.Y * Radius, Radial1.Z * Radius),
			FVector3f(HalfLength, Radial1.Y * Radius, Radial1.Z * Radius),
			FVector3f(HalfLength, Radial0.Y * Radius, Radial0.Z * Radius)
		};
		const FVector3f Normals[] = { Radial0, Radial1, Radial1, Radial0 };
		const FVector2f UVs[] = {
			FVector2f(U0, 0.0f),
			FVector2f(U1, 0.0f),
			FVector2f(U1, 1.0f),
			FVector2f(U0, 1.0f)
		};

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normals[Index];
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void BuildRollingBomberChargeCylinderMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("ChargeCylinder"));

		constexpr int32 SegmentCount = 32;
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			AddRollingBomberChargeCylinderQuad(MeshDescription, Attributes, PolygonGroupId, Angle0, Angle1, U0, U1);
		}
	}

	UMaterial* EnsureRollingBomberChargeCylinderMaterial(UTexture2D* MaskTexture)
	{
		if (!MaskTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, RollingBomberChargeCylinderMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				RollingBomberChargeCylinderMaterialAssetName,
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

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -900;
		TextureCoordinateExpression->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionPanner* PannerExpression = NewObject<UMaterialExpressionPanner>(Material);
		PannerExpression->Material = Material;
		PannerExpression->SpeedX = 2.8f;
		PannerExpression->SpeedY = 0.0f;
		PannerExpression->Coordinate.Connect(0, TextureCoordinateExpression);
		PannerExpression->MaterialExpressionEditorX = -680;
		PannerExpression->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(PannerExpression);

		UMaterialExpressionTextureSampleParameter2D* MaskSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		MaskSample->Material = Material;
		MaskSample->ParameterName = TEXT("MaskTexture");
		MaskSample->Texture = MaskTexture;
		MaskSample->SamplerType = SAMPLERTYPE_Masks;
		MaskSample->Coordinates.Connect(0, PannerExpression);
		MaskSample->MaterialExpressionEditorX = -440;
		MaskSample->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(MaskSample);

		UMaterialExpressionComponentMask* MaskRedChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		MaskRedChannel->Material = Material;
		MaskRedChannel->Input.Connect(0, MaskSample);
		MaskRedChannel->R = 1;
		MaskRedChannel->G = 0;
		MaskRedChannel->B = 0;
		MaskRedChannel->A = 0;
		MaskRedChannel->MaterialExpressionEditorX = -180;
		MaskRedChannel->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(MaskRedChannel);

		UMaterialExpressionVectorParameter* ColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParameter->Material = Material;
		ColorParameter->ParameterName = TEXT("ChargeColor");
		ColorParameter->DefaultValue = FLinearColor(1.0f, 0.035f, 0.0f, 1.0f);
		ColorParameter->MaterialExpressionEditorX = -440;
		ColorParameter->MaterialExpressionEditorY = -180;
		Material->GetExpressionCollection().AddExpression(ColorParameter);

		UMaterialExpressionScalarParameter* IntensityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		IntensityParameter->Material = Material;
		IntensityParameter->ParameterName = TEXT("Intensity");
		IntensityParameter->DefaultValue = 7.0f;
		IntensityParameter->MaterialExpressionEditorX = -440;
		IntensityParameter->MaterialExpressionEditorY = -20;
		Material->GetExpressionCollection().AddExpression(IntensityParameter);

		UMaterialExpressionMultiply* ColorIntensityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		ColorIntensityMultiply->Material = Material;
		ColorIntensityMultiply->A.Connect(0, ColorParameter);
		ColorIntensityMultiply->B.Connect(0, IntensityParameter);
		ColorIntensityMultiply->MaterialExpressionEditorX = -180;
		ColorIntensityMultiply->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(ColorIntensityMultiply);

		UMaterialExpressionMultiply* EmissiveMaskMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMaskMultiply->Material = Material;
		EmissiveMaskMultiply->A.Connect(0, ColorIntensityMultiply);
		EmissiveMaskMultiply->B.Connect(0, MaskRedChannel);
		EmissiveMaskMultiply->MaterialExpressionEditorX = 100;
		EmissiveMaskMultiply->MaterialExpressionEditorY = -80;
		Material->GetExpressionCollection().AddExpression(EmissiveMaskMultiply);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 0.7f;
		OpacityParameter->MaterialExpressionEditorX = -180;
		OpacityParameter->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionMultiply* OpacityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		OpacityMultiply->Material = Material;
		OpacityMultiply->A.Connect(0, MaskRedChannel);
		OpacityMultiply->B.Connect(0, OpacityParameter);
		OpacityMultiply->MaterialExpressionEditorX = 100;
		OpacityMultiply->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(OpacityMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, ColorParameter);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMaskMultiply);
		MaterialEditorOnly->Opacity.Connect(0, OpacityMultiply);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureRollingBomberChargeCylinderMesh(UMaterialInterface* ChargeMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, RollingBomberChargeCylinderMeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *EffectsAssetPath, *RollingBomberChargeCylinderMeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*RollingBomberChargeCylinderMeshAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();

		FMeshDescription MeshDescription;
		BuildRollingBomberChargeCylinderMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(ChargeMaterial, FName(TEXT("ChargeCylinder"))));

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureRollingBomberChargeCylinderEffectAssets()
	{
		UTexture2D* MaskTexture = nullptr;
		const FString SourcePath = GetRollingBomberChargeCylinderMaskSourcePath();
		if (FPaths::FileExists(SourcePath))
		{
			if (!ImportWorldTexture(SourcePath, EffectsAssetPath, RollingBomberChargeCylinderMaskTextureAssetName, &MaskTexture))
			{
				return false;
			}
		}
		else
		{
			MaskTexture = LoadObject<UTexture2D>(
				nullptr,
				*GetAssetObjectPath(EffectsAssetPath, RollingBomberChargeCylinderMaskTextureAssetName));
		}

		if (!MaskTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing RollingBomber charge cylinder mask source: %s"), *SourcePath);
			return false;
		}

		ConfigureImportedMaskTexture(MaskTexture);
		UMaterial* ChargeMaterial = EnsureRollingBomberChargeCylinderMaterial(MaskTexture);
		return ChargeMaterial && EnsureRollingBomberChargeCylinderMesh(ChargeMaterial);
	}

	FString GetLocalExplosionFlipbookSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("GeneratedImages"),
			TEXT("Effects"),
			TEXT("T_LocalExplosionFlipbook.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	UMaterial* EnsureLocalExplosionFlipbookMaterial(
		UTexture2D* FlipbookTexture,
		const FString& MaterialAssetName,
		bool bSmokeMaterial)
	{
		if (!FlipbookTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, MaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				MaterialAssetName,
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
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
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
		TextureCoordinateExpression->MaterialExpressionEditorX = -1040;
		TextureCoordinateExpression->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionScalarParameter* FrameScaleParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		FrameScaleParameter->Material = Material;
		FrameScaleParameter->ParameterName = TEXT("FrameScale");
		FrameScaleParameter->DefaultValue = 0.25f;
		FrameScaleParameter->MaterialExpressionEditorX = -1040;
		FrameScaleParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(FrameScaleParameter);

		UMaterialExpressionMultiply* ScaledUv = NewObject<UMaterialExpressionMultiply>(Material);
		ScaledUv->Material = Material;
		ScaledUv->A.Connect(0, TextureCoordinateExpression);
		ScaledUv->B.Connect(0, FrameScaleParameter);
		ScaledUv->MaterialExpressionEditorX = -800;
		ScaledUv->MaterialExpressionEditorY = 160;
		Material->GetExpressionCollection().AddExpression(ScaledUv);

		UMaterialExpressionScalarParameter* FrameUParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		FrameUParameter->Material = Material;
		FrameUParameter->ParameterName = TEXT("FrameU");
		FrameUParameter->DefaultValue = 0.0f;
		FrameUParameter->MaterialExpressionEditorX = -1040;
		FrameUParameter->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(FrameUParameter);

		UMaterialExpressionScalarParameter* FrameVParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		FrameVParameter->Material = Material;
		FrameVParameter->ParameterName = TEXT("FrameV");
		FrameVParameter->DefaultValue = 0.0f;
		FrameVParameter->MaterialExpressionEditorX = -1040;
		FrameVParameter->MaterialExpressionEditorY = 640;
		Material->GetExpressionCollection().AddExpression(FrameVParameter);

		UMaterialExpressionAppendVector* FrameOffset = NewObject<UMaterialExpressionAppendVector>(Material);
		FrameOffset->Material = Material;
		FrameOffset->A.Connect(0, FrameUParameter);
		FrameOffset->B.Connect(0, FrameVParameter);
		FrameOffset->MaterialExpressionEditorX = -800;
		FrameOffset->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(FrameOffset);

		UMaterialExpressionAdd* FrameUv = NewObject<UMaterialExpressionAdd>(Material);
		FrameUv->Material = Material;
		FrameUv->A.Connect(0, ScaledUv);
		FrameUv->B.Connect(0, FrameOffset);
		FrameUv->MaterialExpressionEditorX = -560;
		FrameUv->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(FrameUv);

		UMaterialExpressionTextureSampleParameter2D* FlipbookSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		FlipbookSample->Material = Material;
		FlipbookSample->ParameterName = TEXT("ExplosionTexture");
		FlipbookSample->Texture = FlipbookTexture;
		FlipbookSample->SamplerType = SAMPLERTYPE_Color;
		FlipbookSample->Coordinates.Connect(0, FrameUv);
		FlipbookSample->MaterialExpressionEditorX = -300;
		FlipbookSample->MaterialExpressionEditorY = 200;
		Material->GetExpressionCollection().AddExpression(FlipbookSample);

		UMaterialExpressionVectorParameter* TintColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		TintColorParameter->Material = Material;
		TintColorParameter->ParameterName = TEXT("TintColor");
		TintColorParameter->DefaultValue = bSmokeMaterial
			? FLinearColor(0.48f, 0.43f, 0.36f, 1.0f)
			: FLinearColor(1.0f, 0.72f, 0.42f, 1.0f);
		TintColorParameter->MaterialExpressionEditorX = -300;
		TintColorParameter->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(TintColorParameter);

		UMaterialExpressionMultiply* TintedTexture = NewObject<UMaterialExpressionMultiply>(Material);
		TintedTexture->Material = Material;
		TintedTexture->A.Connect(0, FlipbookSample);
		TintedTexture->B.Connect(0, TintColorParameter);
		TintedTexture->MaterialExpressionEditorX = -20;
		TintedTexture->MaterialExpressionEditorY = 20;
		Material->GetExpressionCollection().AddExpression(TintedTexture);

		UMaterialExpressionScalarParameter* EmissiveStrengthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		EmissiveStrengthParameter->Material = Material;
		EmissiveStrengthParameter->ParameterName = TEXT("EmissiveStrength");
		EmissiveStrengthParameter->DefaultValue = bSmokeMaterial ? 0.0f : 3.5f;
		EmissiveStrengthParameter->MaterialExpressionEditorX = -20;
		EmissiveStrengthParameter->MaterialExpressionEditorY = -200;
		Material->GetExpressionCollection().AddExpression(EmissiveStrengthParameter);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 1.0f;
		OpacityParameter->MaterialExpressionEditorX = -20;
		OpacityParameter->MaterialExpressionEditorY = 500;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionMultiply* EmissiveStrengthMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveStrengthMultiply->Material = Material;
		EmissiveStrengthMultiply->A.Connect(0, TintedTexture);
		EmissiveStrengthMultiply->B.Connect(0, EmissiveStrengthParameter);
		EmissiveStrengthMultiply->MaterialExpressionEditorX = 220;
		EmissiveStrengthMultiply->MaterialExpressionEditorY = -80;
		Material->GetExpressionCollection().AddExpression(EmissiveStrengthMultiply);

		UMaterialExpressionMultiply* FinalEmissive = NewObject<UMaterialExpressionMultiply>(Material);
		FinalEmissive->Material = Material;
		FinalEmissive->A.Connect(0, EmissiveStrengthMultiply);
		FinalEmissive->B.Connect(0, OpacityParameter);
		FinalEmissive->MaterialExpressionEditorX = 460;
		FinalEmissive->MaterialExpressionEditorY = -60;
		Material->GetExpressionCollection().AddExpression(FinalEmissive);

		UMaterialExpressionComponentMask* RedChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		RedChannel->Material = Material;
		RedChannel->Input.Connect(0, FlipbookSample);
		RedChannel->R = 1;
		RedChannel->G = 0;
		RedChannel->B = 0;
		RedChannel->A = 0;
		RedChannel->MaterialExpressionEditorX = -20;
		RedChannel->MaterialExpressionEditorY = 700;
		Material->GetExpressionCollection().AddExpression(RedChannel);

		UMaterialExpressionComponentMask* GreenChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		GreenChannel->Material = Material;
		GreenChannel->Input.Connect(0, FlipbookSample);
		GreenChannel->R = 0;
		GreenChannel->G = 1;
		GreenChannel->B = 0;
		GreenChannel->A = 0;
		GreenChannel->MaterialExpressionEditorX = -20;
		GreenChannel->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(GreenChannel);

		UMaterialExpressionComponentMask* BlueChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		BlueChannel->Material = Material;
		BlueChannel->Input.Connect(0, FlipbookSample);
		BlueChannel->R = 0;
		BlueChannel->G = 0;
		BlueChannel->B = 1;
		BlueChannel->A = 0;
		BlueChannel->MaterialExpressionEditorX = -20;
		BlueChannel->MaterialExpressionEditorY = 1020;
		Material->GetExpressionCollection().AddExpression(BlueChannel);

		UMaterialExpressionAdd* RedGreenSum = NewObject<UMaterialExpressionAdd>(Material);
		RedGreenSum->Material = Material;
		RedGreenSum->A.Connect(0, RedChannel);
		RedGreenSum->B.Connect(0, GreenChannel);
		RedGreenSum->MaterialExpressionEditorX = 220;
		RedGreenSum->MaterialExpressionEditorY = 780;
		Material->GetExpressionCollection().AddExpression(RedGreenSum);

		UMaterialExpressionAdd* RgbSum = NewObject<UMaterialExpressionAdd>(Material);
		RgbSum->Material = Material;
		RgbSum->A.Connect(0, RedGreenSum);
		RgbSum->B.Connect(0, BlueChannel);
		RgbSum->MaterialExpressionEditorX = 460;
		RgbSum->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(RgbSum);

		UMaterialExpressionMultiply* LuminanceScale = NewObject<UMaterialExpressionMultiply>(Material);
		LuminanceScale->Material = Material;
		LuminanceScale->A.Connect(0, RgbSum);
		LuminanceScale->ConstB = 0.42f;
		LuminanceScale->MaterialExpressionEditorX = 700;
		LuminanceScale->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(LuminanceScale);

		UMaterialExpressionSaturate* LuminanceMask = NewObject<UMaterialExpressionSaturate>(Material);
		LuminanceMask->Material = Material;
		LuminanceMask->Input.Connect(0, LuminanceScale);
		LuminanceMask->MaterialExpressionEditorX = 940;
		LuminanceMask->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(LuminanceMask);

		UMaterialExpressionScalarParameter* AlphaBoostParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		AlphaBoostParameter->Material = Material;
		AlphaBoostParameter->ParameterName = TEXT("AlphaBoost");
		AlphaBoostParameter->DefaultValue = bSmokeMaterial ? 2.65f : 1.0f;
		AlphaBoostParameter->MaterialExpressionEditorX = 940;
		AlphaBoostParameter->MaterialExpressionEditorY = 1040;
		Material->GetExpressionCollection().AddExpression(AlphaBoostParameter);

		UMaterialExpressionMultiply* BoostedLuminanceMask = NewObject<UMaterialExpressionMultiply>(Material);
		BoostedLuminanceMask->Material = Material;
		BoostedLuminanceMask->A.Connect(0, LuminanceMask);
		BoostedLuminanceMask->B.Connect(0, AlphaBoostParameter);
		BoostedLuminanceMask->MaterialExpressionEditorX = 1180;
		BoostedLuminanceMask->MaterialExpressionEditorY = 920;
		Material->GetExpressionCollection().AddExpression(BoostedLuminanceMask);

		UMaterialExpressionSaturate* BoostedOpacityMask = NewObject<UMaterialExpressionSaturate>(Material);
		BoostedOpacityMask->Material = Material;
		BoostedOpacityMask->Input.Connect(0, BoostedLuminanceMask);
		BoostedOpacityMask->MaterialExpressionEditorX = 1420;
		BoostedOpacityMask->MaterialExpressionEditorY = 920;
		Material->GetExpressionCollection().AddExpression(BoostedOpacityMask);

		UMaterialExpressionMultiply* FinalOpacity = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacity->Material = Material;
		FinalOpacity->A.Connect(0, BoostedOpacityMask);
		FinalOpacity->B.Connect(0, OpacityParameter);
		FinalOpacity->MaterialExpressionEditorX = 1660;
		FinalOpacity->MaterialExpressionEditorY = 780;
		Material->GetExpressionCollection().AddExpression(FinalOpacity);

		MaterialEditorOnly->BaseColor.Connect(0, TintedTexture);
		MaterialEditorOnly->EmissiveColor.Connect(0, FinalEmissive);
		MaterialEditorOnly->Opacity.Connect(0, FinalOpacity);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UMaterial* EnsureLocalExplosionDistortionMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LocalExplosionDistortionMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				LocalExplosionDistortionMaterialAssetName,
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
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bTangentSpaceNormal = true;
		Material->RefractionMethod = RM_PixelNormalOffset;
		Material->RefractionDepthBias = 8.0f;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -1240;
		TextureCoordinateExpression->MaterialExpressionEditorY = 40;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionConstant2Vector* CenterVector = NewObject<UMaterialExpressionConstant2Vector>(Material);
		CenterVector->Material = Material;
		CenterVector->R = 0.5f;
		CenterVector->G = 0.5f;
		CenterVector->MaterialExpressionEditorX = -1240;
		CenterVector->MaterialExpressionEditorY = 240;
		Material->GetExpressionCollection().AddExpression(CenterVector);

		UMaterialExpressionSubtract* CenteredUv = NewObject<UMaterialExpressionSubtract>(Material);
		CenteredUv->Material = Material;
		CenteredUv->A.Connect(0, TextureCoordinateExpression);
		CenteredUv->B.Connect(0, CenterVector);
		CenteredUv->MaterialExpressionEditorX = -980;
		CenteredUv->MaterialExpressionEditorY = 100;
		Material->GetExpressionCollection().AddExpression(CenteredUv);

		UMaterialExpressionLength* RadialDistance = NewObject<UMaterialExpressionLength>(Material);
		RadialDistance->Material = Material;
		RadialDistance->Input.Connect(0, CenteredUv);
		RadialDistance->MaterialExpressionEditorX = -740;
		RadialDistance->MaterialExpressionEditorY = 100;
		Material->GetExpressionCollection().AddExpression(RadialDistance);

		UMaterialExpressionScalarParameter* WavePositionParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		WavePositionParameter->Material = Material;
		WavePositionParameter->ParameterName = TEXT("WavePosition");
		WavePositionParameter->DefaultValue = 0.22f;
		WavePositionParameter->MaterialExpressionEditorX = -740;
		WavePositionParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(WavePositionParameter);

		UMaterialExpressionSubtract* RadiusFromWave = NewObject<UMaterialExpressionSubtract>(Material);
		RadiusFromWave->Material = Material;
		RadiusFromWave->A.Connect(0, RadialDistance);
		RadiusFromWave->B.Connect(0, WavePositionParameter);
		RadiusFromWave->MaterialExpressionEditorX = -500;
		RadiusFromWave->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(RadiusFromWave);

		UMaterialExpressionAbs* WaveDelta = NewObject<UMaterialExpressionAbs>(Material);
		WaveDelta->Material = Material;
		WaveDelta->Input.Connect(0, RadiusFromWave);
		WaveDelta->MaterialExpressionEditorX = -260;
		WaveDelta->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(WaveDelta);

		UMaterialExpressionScalarParameter* WaveWidthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		WaveWidthParameter->Material = Material;
		WaveWidthParameter->ParameterName = TEXT("WaveWidth");
		WaveWidthParameter->DefaultValue = 0.15f;
		WaveWidthParameter->MaterialExpressionEditorX = -260;
		WaveWidthParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(WaveWidthParameter);

		UMaterialExpressionDivide* NormalizedWaveDelta = NewObject<UMaterialExpressionDivide>(Material);
		NormalizedWaveDelta->Material = Material;
		NormalizedWaveDelta->A.Connect(0, WaveDelta);
		NormalizedWaveDelta->B.Connect(0, WaveWidthParameter);
		NormalizedWaveDelta->MaterialExpressionEditorX = -20;
		NormalizedWaveDelta->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(NormalizedWaveDelta);

		UMaterialExpressionOneMinus* WaveFalloff = NewObject<UMaterialExpressionOneMinus>(Material);
		WaveFalloff->Material = Material;
		WaveFalloff->Input.Connect(0, NormalizedWaveDelta);
		WaveFalloff->MaterialExpressionEditorX = 220;
		WaveFalloff->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(WaveFalloff);

		UMaterialExpressionSaturate* RingMask = NewObject<UMaterialExpressionSaturate>(Material);
		RingMask->Material = Material;
		RingMask->Input.Connect(0, WaveFalloff);
		RingMask->MaterialExpressionEditorX = 460;
		RingMask->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(RingMask);

		UMaterialExpressionScalarParameter* DistortionStrengthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		DistortionStrengthParameter->Material = Material;
		DistortionStrengthParameter->ParameterName = TEXT("DistortionStrength");
		DistortionStrengthParameter->DefaultValue = 0.32f;
		DistortionStrengthParameter->MaterialExpressionEditorX = 220;
		DistortionStrengthParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(DistortionStrengthParameter);

		UMaterialExpressionMultiply* MaskedStrength = NewObject<UMaterialExpressionMultiply>(Material);
		MaskedStrength->Material = Material;
		MaskedStrength->A.Connect(0, RingMask);
		MaskedStrength->B.Connect(0, DistortionStrengthParameter);
		MaskedStrength->MaterialExpressionEditorX = 700;
		MaskedStrength->MaterialExpressionEditorY = 220;
		Material->GetExpressionCollection().AddExpression(MaskedStrength);

		UMaterialExpressionMultiply* DistortedNormalXY = NewObject<UMaterialExpressionMultiply>(Material);
		DistortedNormalXY->Material = Material;
		DistortedNormalXY->A.Connect(0, CenteredUv);
		DistortedNormalXY->B.Connect(0, MaskedStrength);
		DistortedNormalXY->MaterialExpressionEditorX = 940;
		DistortedNormalXY->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(DistortedNormalXY);

		UMaterialExpressionConstant* NormalZ = NewObject<UMaterialExpressionConstant>(Material);
		NormalZ->Material = Material;
		NormalZ->R = 1.0f;
		NormalZ->MaterialExpressionEditorX = 940;
		NormalZ->MaterialExpressionEditorY = 320;
		Material->GetExpressionCollection().AddExpression(NormalZ);

		UMaterialExpressionAppendVector* DistortedNormalVector = NewObject<UMaterialExpressionAppendVector>(Material);
		DistortedNormalVector->Material = Material;
		DistortedNormalVector->A.Connect(0, DistortedNormalXY);
		DistortedNormalVector->B.Connect(0, NormalZ);
		DistortedNormalVector->MaterialExpressionEditorX = 1180;
		DistortedNormalVector->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(DistortedNormalVector);

		UMaterialExpressionNormalize* DistortionNormal = NewObject<UMaterialExpressionNormalize>(Material);
		DistortionNormal->Material = Material;
		DistortionNormal->VectorInput.Connect(0, DistortedNormalVector);
		DistortionNormal->MaterialExpressionEditorX = 1420;
		DistortionNormal->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(DistortionNormal);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 0.26f;
		OpacityParameter->MaterialExpressionEditorX = 700;
		OpacityParameter->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionMultiply* FinalOpacity = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacity->Material = Material;
		FinalOpacity->A.Connect(0, RingMask);
		FinalOpacity->B.Connect(0, OpacityParameter);
		FinalOpacity->MaterialExpressionEditorX = 940;
		FinalOpacity->MaterialExpressionEditorY = 500;
		Material->GetExpressionCollection().AddExpression(FinalOpacity);

		UMaterialExpressionScalarParameter* RefractionAmountParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		RefractionAmountParameter->Material = Material;
		RefractionAmountParameter->ParameterName = TEXT("RefractionAmount");
		RefractionAmountParameter->DefaultValue = 1.18f;
		RefractionAmountParameter->MaterialExpressionEditorX = 1420;
		RefractionAmountParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(RefractionAmountParameter);

		UMaterialExpressionConstant3Vector* BaseColor = NewObject<UMaterialExpressionConstant3Vector>(Material);
		BaseColor->Material = Material;
		BaseColor->Constant = FLinearColor::Black;
		BaseColor->MaterialExpressionEditorX = 1420;
		BaseColor->MaterialExpressionEditorY = 560;
		Material->GetExpressionCollection().AddExpression(BaseColor);

		MaterialEditorOnly->BaseColor.Connect(0, BaseColor);
		MaterialEditorOnly->Normal.Connect(0, DistortionNormal);
		MaterialEditorOnly->Opacity.Connect(0, FinalOpacity);
		MaterialEditorOnly->Refraction.Connect(0, RefractionAmountParameter);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool EnsureLocalExplosionEffectAssets()
	{
		UTexture2D* FlipbookTexture = nullptr;
		const FString SourcePath = GetLocalExplosionFlipbookSourcePath();
		if (FPaths::FileExists(SourcePath))
		{
			if (!ImportWorldTexture(SourcePath, EffectsAssetPath, LocalExplosionFlipbookTextureAssetName, &FlipbookTexture))
			{
				return false;
			}
		}
		else
		{
			FlipbookTexture = LoadObject<UTexture2D>(
				nullptr,
				*GetAssetObjectPath(EffectsAssetPath, LocalExplosionFlipbookTextureAssetName));
		}

		if (!FlipbookTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing local explosion flipbook source: %s"), *SourcePath);
			return false;
		}

		ConfigureImportedEffectTexture(FlipbookTexture);
		return EnsureLocalExplosionFlipbookMaterial(FlipbookTexture, LocalExplosionFlipbookMaterialAssetName, false) != nullptr
			&& EnsureLocalExplosionFlipbookMaterial(FlipbookTexture, LocalExplosionSmokeMaterialAssetName, true) != nullptr
			&& EnsureLocalExplosionDistortionMaterial() != nullptr;
	}

	bool SetNiagaraStoreBoolByName(FNiagaraParameterStore& Store, FName ParameterName, bool bValue)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() == ParameterName &&
				Parameter.GetType() == FNiagaraTypeDefinition::GetBoolDef())
			{
				bApplied |= Store.SetParameterValue(FNiagaraBool(bValue), Parameter);
			}
		}
		return bApplied;
	}

	bool SetNiagaraStoreFloatByName(FNiagaraParameterStore& Store, FName ParameterName, float Value)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() == ParameterName &&
				Parameter.GetType() == FNiagaraTypeDefinition::GetFloatDef())
			{
				bApplied |= Store.SetParameterValue(Value, Parameter);
			}
		}
		return bApplied;
	}

	bool SetNiagaraStoreVec3ByName(FNiagaraParameterStore& Store, FName ParameterName, const FVector& Value)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() != ParameterName)
			{
				continue;
			}

			if (Parameter.GetType() == FNiagaraTypeDefinition::GetPositionDef())
			{
				bApplied |= Store.SetPositionParameterValue(Value, ParameterName);
			}
			else if (Parameter.GetType() == FNiagaraTypeDefinition::GetVec3Def())
			{
				bApplied |= Store.SetParameterValue(FVector3f(Value), Parameter);
			}
		}
		return bApplied;
	}

	bool SetNiagaraStoreColorByName(FNiagaraParameterStore& Store, FName ParameterName, const FLinearColor& Value)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() == ParameterName &&
				Parameter.GetType() == FNiagaraTypeDefinition::GetColorDef())
			{
				bApplied |= Store.SetParameterValue(Value, Parameter);
			}
		}
		return bApplied;
	}

	int32 ConfigureExtractionSmokeSignalNiagaraScript(UNiagaraScript* Script)
	{
		if (!Script)
		{
			return 0;
		}

		FNiagaraParameterStore& Store = Script->RapidIterationParameters;
		const FVector WorldSpaceSize(420.0f, 305.0f, 460.0f);
		const FVector SourceOffset(0.0f, 0.0f, 8.0f);
		const FVector SourceScale(2.8f, 1.45f, 0.16f);
		const FVector SourceVelocity(30.0f, 11.0f, 185.0f);
		const FLinearColor SmokeBaseColor(0.02f, 1.0f, 0.18f, 1.0f);
		const FLinearColor SmokeTopColor(0.035f, 0.04f, 0.035f, 1.0f);

		int32 AppliedCount = 0;
		auto ApplyBool = [&Store, &AppliedCount](const TCHAR* Name, bool bValue)
		{
			AppliedCount += SetNiagaraStoreBoolByName(Store, FName(Name), bValue) ? 1 : 0;
		};
		auto ApplyFloat = [&Store, &AppliedCount](const TCHAR* Name, float Value)
		{
			AppliedCount += SetNiagaraStoreFloatByName(Store, FName(Name), Value) ? 1 : 0;
		};
		auto ApplyVec3 = [&Store, &AppliedCount](const TCHAR* Name, const FVector& Value)
		{
			AppliedCount += SetNiagaraStoreVec3ByName(Store, FName(Name), Value) ? 1 : 0;
		};
		auto ApplyColor = [&Store, &AppliedCount](const TCHAR* Name, const FLinearColor& Value)
		{
			AppliedCount += SetNiagaraStoreColorByName(Store, FName(Name), Value) ? 1 : 0;
		};

		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Debug Draw"), false);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.Debug Collision Volume"), false);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_DebugDisplay.Debug Collision Volume"), false);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.World Size"), WorldSpaceSize);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Enable"), true);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Position"), SourceOffset);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Non Uniform Scale"), SourceScale);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Velocity"), SourceVelocity);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Radius"), 27.0f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Density"), 1.35f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Temperature"), 0.18f);
		ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Color"), SmokeBaseColor);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_Buoyancy.TemperatureBuoyancy"), 1.1f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_Buoyancy.DensityBuoyancy"), 0.05f);
		ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_MaterialControls.Smoke Color"), SmokeTopColor);

		if (AppliedCount > 0)
		{
			Script->Modify();
		}

		return AppliedCount;
	}

	bool ConfigureExtractionSmokeSignalNiagaraSystem(UNiagaraSystem* System)
	{
		if (!System)
		{
			return false;
		}

		System->Modify();

		FNiagaraUserRedirectionParameterStore& UserParameters = System->GetExposedParameters();
		SetNiagaraStoreBoolByName(UserParameters, FName(TEXT("User.DrawBounds")), false);
		SetNiagaraStoreVec3ByName(UserParameters, FName(TEXT("User.WorldSpaceSize")), FVector(420.0f, 305.0f, 460.0f));
		SetNiagaraStoreFloatByName(UserParameters, FName(TEXT("User.ResolutionMaxAxis")), 96.0f);

		for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			EmitterHandle.SetDebugShowBounds(false);
			if (FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData())
			{
				EmitterData->bLocalSpace = false;
				EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
				EmitterData->FixedBounds = FBox(FVector(-230.0f, -170.0f, 0.0f), FVector(230.0f, 170.0f, 460.0f));
			}
		}

		int32 AppliedScriptParameterCount = 0;
		System->ForEachScript(
			[&AppliedScriptParameterCount](UNiagaraScript* Script)
			{
				AppliedScriptParameterCount += ConfigureExtractionSmokeSignalNiagaraScript(Script);
			});

		System->InvalidateCachedData();
		System->RequestCompile(true);
		System->PollForCompilationComplete(true);
		System->PostEditChange();
		System->MarkPackageDirty();

		UE_LOG(
			LogTunaSweeperEditor,
			Log,
			TEXT("Configured extraction smoke Niagara system. Applied %d rapid iteration parameter updates."),
			AppliedScriptParameterCount);
		return true;
	}

	bool ConfigureExplosiveBarrelSmokeNiagaraSystem(UNiagaraSystem* System, float Strength, bool bBlackSmokeOnly)
	{
		if (!System)
		{
			return false;
		}

		const float SafeStrength = FMath::Clamp(Strength, 0.35f, 2.0f);
		System->Modify();
		FNiagaraUserRedirectionParameterStore& UserParameters = System->GetExposedParameters();
		SetNiagaraStoreVec3ByName(UserParameters, FName(TEXT("User.SourceOffset")), FVector::ZeroVector);
		for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			if (FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData())
			{
				// Persistent barrel smoke must simulate around the component transform.
				// World-space fluid grids can visually drift away from a moving/placed barrel.
				EmitterData->bLocalSpace = true;
				EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
				EmitterData->FixedBounds = FBox(FVector(-180.0f, -180.0f, -20.0f), FVector(180.0f, 180.0f, 440.0f));
			}
		}

		int32 AppliedCount = 0;
		System->ForEachScript(
			[SafeStrength, bBlackSmokeOnly, &AppliedCount](UNiagaraScript* Script)
			{
				if (!Script) return;
				FNiagaraParameterStore& Store = Script->RapidIterationParameters;
				auto ApplyFloat = [&Store, &AppliedCount](const TCHAR* Name, float Value)
				{
					AppliedCount += SetNiagaraStoreFloatByName(Store, FName(Name), Value) ? 1 : 0;
				};
				auto ApplyVec3 = [&Store, &AppliedCount](const TCHAR* Name, const FVector& Value)
				{
					AppliedCount += SetNiagaraStoreVec3ByName(Store, FName(Name), Value) ? 1 : 0;
				};
				auto ApplyColor = [&Store, &AppliedCount](const TCHAR* Name, const FLinearColor& Value)
				{
					AppliedCount += SetNiagaraStoreColorByName(Store, FName(Name), Value) ? 1 : 0;
				};
				ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.World Size"), FVector(300.0f, 300.0f, 510.0f));
				ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Non Uniform Scale"), FVector(2.3f, 2.3f, 0.22f) * SafeStrength);
				ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Velocity"), FVector(18.0f, 9.0f, 150.0f + 55.0f * SafeStrength));
				ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Radius"), 24.0f * SafeStrength);
				ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Density"), 1.45f * SafeStrength);
				ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Temperature"), bBlackSmokeOnly ? 0.0f : 1.2f * SafeStrength);
				ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Color"), bBlackSmokeOnly ? FLinearColor::Black : FLinearColor(1.0f, 0.055f, 0.006f, 1.0f));
				ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_MaterialControls.Smoke Color"), bBlackSmokeOnly ? FLinearColor(0.003f, 0.003f, 0.003f, 1.0f) : FLinearColor(0.012f, 0.004f, 0.002f, 1.0f));
			});

		System->InvalidateCachedData();
		System->RequestCompile(true);
		System->PollForCompilationComplete(true);
		System->PostEditChange();
		System->MarkPackageDirty();
		UE_LOG(LogTunaSweeperEditor, Log, TEXT("Configured explosive barrel smoke (%0.2f, black smoke only: %s, %d parameter updates)."), SafeStrength, bBlackSmokeOnly ? TEXT("true") : TEXT("false"), AppliedCount);
		return SaveAsset(System);
	}

	UObject* LoadExtractionSmokeSignalSourceTemplate()
	{
		const TCHAR* SourceSystemPath =
			TEXT("/NiagaraFluids/Templates/Gas/3D/Systems/Grid3D_Gas_SimpleParticleSource.Grid3D_Gas_SimpleParticleSource");
		UObject* SourceSystem = LoadObject<UObject>(nullptr, SourceSystemPath);
		if (!SourceSystem)
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to load extraction smoke source template: %s"),
				SourceSystemPath);
		}
		return SourceSystem;
	}

	bool DeleteExistingExtractionSmokeSignalNiagaraSystem(const FString& ObjectPath)
	{
		UObject* ExistingSystem = LoadObject<UObject>(nullptr, *ObjectPath);
		if (!ExistingSystem)
		{
			return true;
		}

		TArray<UObject*> ObjectsToDelete;
		ObjectsToDelete.Add(ExistingSystem);
		const int32 DeletedCount = ObjectTools::ForceDeleteObjects(ObjectsToDelete, false);
		if (DeletedCount != ObjectsToDelete.Num())
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to recreate %s because the existing asset could not be deleted."),
				*ObjectPath);
			return false;
		}

		CollectGarbage(RF_NoFlags);
		return true;
	}

	bool EnsureExtractionSmokeSignalNiagaraSystem()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, ExtractionSmokeSignalNiagaraSystemAssetName);
		if (!DeleteExistingExtractionSmokeSignalNiagaraSystem(ObjectPath))
		{
			return false;
		}

		UObject* SourceSystem = LoadExtractionSmokeSignalSourceTemplate();
		if (!SourceSystem)
		{
			return false;
		}

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* DuplicatedSystem = AssetToolsModule.Get().DuplicateAsset(
			ExtractionSmokeSignalNiagaraSystemAssetName,
			EffectsAssetPath,
			SourceSystem);
		if (!DuplicatedSystem)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to duplicate %s."), *ObjectPath);
			return false;
		}

		if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(DuplicatedSystem))
		{
			ConfigureExtractionSmokeSignalNiagaraSystem(NiagaraSystem);
		}
		else
		{
			DuplicatedSystem->Modify();
			DuplicatedSystem->MarkPackageDirty();
		}
		return SaveAsset(DuplicatedSystem);
	}

	int32 ConfigureLookdevFluidExplosionNiagaraScript(UNiagaraScript* Script)
	{
		if (!Script)
		{
			return 0;
		}

		FNiagaraParameterStore& Store = Script->RapidIterationParameters;
		const FVector WorldSpaceSize(920.0f, 920.0f, 760.0f);
		const FVector SourceOffset(0.0f, 0.0f, 38.0f);
		const FVector SourceScale(1.0f, 1.0f, 0.78f);
		const FVector SourceVelocity(0.0f, 0.0f, 460.0f);
		const FLinearColor HotCoreColor(1.0f, 0.46f, 0.08f, 1.0f);
		const FLinearColor SmokeColor(0.22f, 0.18f, 0.14f, 1.0f);

		int32 AppliedCount = 0;
		auto ApplyBool = [&Store, &AppliedCount](const TCHAR* Name, bool bValue)
		{
			AppliedCount += SetNiagaraStoreBoolByName(Store, FName(Name), bValue) ? 1 : 0;
		};
		auto ApplyFloat = [&Store, &AppliedCount](const TCHAR* Name, float Value)
		{
			AppliedCount += SetNiagaraStoreFloatByName(Store, FName(Name), Value) ? 1 : 0;
		};
		auto ApplyVec3 = [&Store, &AppliedCount](const TCHAR* Name, const FVector& Value)
		{
			AppliedCount += SetNiagaraStoreVec3ByName(Store, FName(Name), Value) ? 1 : 0;
		};
		auto ApplyColor = [&Store, &AppliedCount](const TCHAR* Name, const FLinearColor& Value)
		{
			AppliedCount += SetNiagaraStoreColorByName(Store, FName(Name), Value) ? 1 : 0;
		};

		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Debug Draw"), false);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.Debug Collision Volume"), false);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_DebugDisplay.Debug Collision Volume"), false);
		ApplyBool(TEXT("Emitter.Debug Draw"), false);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.World Size"), WorldSpaceSize);
		ApplyVec3(TEXT("Emitter.Grid3D_Gas_InitializeEmitter.World Size"), WorldSpaceSize);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.Resolution Max Axis"), 192.0f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Resolution Max Axis"), 192.0f);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Enable"), true);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Position"), SourceOffset);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Non Uniform Scale"), SourceScale);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Velocity"), SourceVelocity);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Radius"), 92.0f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Density"), 2.25f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Temperature"), 3.2f);
		ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Color"), HotCoreColor);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_Buoyancy.TemperatureBuoyancy"), 2.8f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_Buoyancy.DensityBuoyancy"), -0.12f);
		ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_MaterialControls.Fire Color"), HotCoreColor);
		ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_MaterialControls.Smoke Color"), SmokeColor);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_MaterialControls.Density Mult"), 1.55f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_MaterialControls.Emissive Mult"), 2.8f);

		if (AppliedCount > 0)
		{
			Script->Modify();
		}

		return AppliedCount;
	}

	bool ConfigureLookdevFluidExplosionNiagaraSystem(UNiagaraSystem* System)
	{
		if (!System)
		{
			return false;
		}

		System->Modify();

		const FVector WorldSpaceSize(920.0f, 920.0f, 760.0f);
		FNiagaraUserRedirectionParameterStore& UserParameters = System->GetExposedParameters();
		SetNiagaraStoreBoolByName(UserParameters, FName(TEXT("User.DrawBounds")), false);
		SetNiagaraStoreVec3ByName(UserParameters, FName(TEXT("User.WorldSpaceSize")), WorldSpaceSize);
		SetNiagaraStoreFloatByName(UserParameters, FName(TEXT("User.ResolutionMaxAxis")), 192.0f);
		SetNiagaraStoreFloatByName(UserParameters, FName(TEXT("User.SourceRadius")), 92.0f);
		SetNiagaraStoreFloatByName(UserParameters, FName(TEXT("User.Density")), 2.25f);
		SetNiagaraStoreFloatByName(UserParameters, FName(TEXT("User.Temperature")), 3.2f);
		SetNiagaraStoreColorByName(UserParameters, FName(TEXT("User.FireColor")), FLinearColor(1.0f, 0.46f, 0.08f, 1.0f));
		SetNiagaraStoreColorByName(UserParameters, FName(TEXT("User.SmokeColor")), FLinearColor(0.22f, 0.18f, 0.14f, 1.0f));

		for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			EmitterHandle.SetDebugShowBounds(false);
			if (FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData())
			{
				EmitterData->bLocalSpace = false;
				EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
				EmitterData->FixedBounds = FBox(FVector(-520.0f, -520.0f, -30.0f), FVector(520.0f, 520.0f, 760.0f));
			}
		}

		int32 AppliedScriptParameterCount = 0;
		System->ForEachScript(
			[&AppliedScriptParameterCount](UNiagaraScript* Script)
			{
				AppliedScriptParameterCount += ConfigureLookdevFluidExplosionNiagaraScript(Script);
			});

		System->InvalidateCachedData();
		System->RequestCompile(true);
		System->PollForCompilationComplete(true);
		System->PostEditChange();
		System->MarkPackageDirty();

		UE_LOG(
			LogTunaSweeperEditor,
			Log,
			TEXT("Configured lookdev fluid explosion Niagara system. Applied %d rapid iteration parameter updates."),
			AppliedScriptParameterCount);
		return true;
	}

	UObject* LoadLookdevFluidExplosionSourceTemplate()
	{
		const TCHAR* SourceSystemPath =
			TEXT("/NiagaraFluids/Templates/Gas/3D/Systems/Grid3D_Gas_Explosion.Grid3D_Gas_Explosion");
		UObject* SourceSystem = LoadObject<UObject>(nullptr, SourceSystemPath);
		if (!SourceSystem)
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to load lookdev fluid explosion source template: %s"),
				SourceSystemPath);
		}
		return SourceSystem;
	}

	bool DeleteExistingLookdevFluidExplosionNiagaraSystem(const FString& ObjectPath)
	{
		UObject* ExistingSystem = LoadObject<UObject>(nullptr, *ObjectPath);
		if (!ExistingSystem)
		{
			return true;
		}

		TArray<UObject*> ObjectsToDelete;
		ObjectsToDelete.Add(ExistingSystem);
		const int32 DeletedCount = ObjectTools::ForceDeleteObjects(ObjectsToDelete, false);
		if (DeletedCount != ObjectsToDelete.Num())
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to recreate %s because the existing asset could not be deleted."),
				*ObjectPath);
			return false;
		}

		CollectGarbage(RF_NoFlags);
		return true;
	}

	UNiagaraSystem* EnsureLookdevFluidExplosionNiagaraSystem()
	{
		const FString ObjectPath = GetAssetObjectPath(LookdevAssetPath, LookdevFluidExplosionNiagaraSystemAssetName);
		if (!DeleteExistingLookdevFluidExplosionNiagaraSystem(ObjectPath))
		{
			return nullptr;
		}

		UObject* SourceSystem = LoadLookdevFluidExplosionSourceTemplate();
		if (!SourceSystem)
		{
			return nullptr;
		}

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* DuplicatedSystem = AssetToolsModule.Get().DuplicateAsset(
			LookdevFluidExplosionNiagaraSystemAssetName,
			LookdevAssetPath,
			SourceSystem);
		UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(DuplicatedSystem);
		if (!NiagaraSystem)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to duplicate %s."), *ObjectPath);
			return nullptr;
		}

		ConfigureLookdevFluidExplosionNiagaraSystem(NiagaraSystem);
		return SaveAsset(NiagaraSystem) ? NiagaraSystem : nullptr;
	}

	void ApplyLookdevNiagaraComponentOverrides(UNiagaraComponent* NiagaraComponent)
	{
		if (!NiagaraComponent)
		{
			return;
		}

		NiagaraComponent->SetVariableBool(FName(TEXT("User.DrawBounds")), false);
		NiagaraComponent->SetVariableVec3(FName(TEXT("User.WorldSpaceSize")), FVector(920.0f, 920.0f, 760.0f));
		NiagaraComponent->SetVariableFloat(FName(TEXT("User.ResolutionMaxAxis")), 192.0f);
		NiagaraComponent->SetVariableFloat(FName(TEXT("User.SourceRadius")), 92.0f);
		NiagaraComponent->SetVariableFloat(FName(TEXT("User.Density")), 2.25f);
		NiagaraComponent->SetVariableFloat(FName(TEXT("User.Temperature")), 3.2f);
		NiagaraComponent->SetVariableLinearColor(FName(TEXT("User.FireColor")), FLinearColor(1.0f, 0.46f, 0.08f, 1.0f));
		NiagaraComponent->SetVariableLinearColor(FName(TEXT("User.SmokeColor")), FLinearColor(0.22f, 0.18f, 0.14f, 1.0f));

		NiagaraComponent->SetVariableBool(FName(TEXT("Grid3D_Gas_Master_Emitter.Debug Draw")), false);
		NiagaraComponent->SetVariableVec3(
			FName(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.World Size")),
			FVector(920.0f, 920.0f, 760.0f));
		NiagaraComponent->SetVariableFloat(
			FName(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.Resolution Max Axis")),
			192.0f);
		NiagaraComponent->SetVariableFloat(
			FName(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Radius")),
			92.0f);
		NiagaraComponent->SetVariableFloat(
			FName(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Density")),
			2.25f);
		NiagaraComponent->SetVariableFloat(
			FName(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Temperature")),
			3.2f);
	}

	AStaticMeshActor* SpawnLookdevStaticMeshActor(
		UWorld* World,
		const TCHAR* ActorName,
		const TCHAR* ActorLabel,
		UStaticMesh* StaticMesh,
		UMaterialInterface* Material,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale)
	{
		if (!World || !StaticMesh)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, AStaticMeshActor::StaticClass(), ActorName);

		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(Location, Rotation, SpawnParameters);
		if (!MeshActor)
		{
			return nullptr;
		}

		MeshActor->SetActorLabel(ActorLabel);
		MeshActor->SetActorScale3D(Scale);
		UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent();
		if (MeshComponent)
		{
			MeshComponent->SetMobility(EComponentMobility::Static);
			MeshComponent->SetStaticMesh(StaticMesh);
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
			MeshComponent->SetGenerateOverlapEvents(false);
			if (Material)
			{
				MeshComponent->SetMaterial(0, Material);
			}
		}
		MeshActor->MarkPackageDirty();
		return MeshActor;
	}

	bool EnsureLookdevFluidExplosionCaptureLevel(UNiagaraSystem* NiagaraSystem)
	{
		if (!NiagaraSystem)
		{
			return false;
		}

		UMaterial* FloorMaterial = EnsureSolidColorMaterial(
			LookdevAssetPath,
			LookdevExplosionFloorMaterialAssetName,
			FLinearColor(0.045f, 0.043f, 0.038f, 1.0f),
			0.88f,
			0.0f,
			0.18f);
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!FloorMaterial || !CubeMesh)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load lookdev floor assets."));
			return false;
		}

		UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
		if (!World)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create lookdev fluid explosion map."));
			return false;
		}

		World->PersistentLevel->Modify();
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->Modify();
			WorldSettings->bForceNoPrecomputedLighting = true;
			WorldSettings->MarkPackageDirty();
		}

		SpawnLookdevStaticMeshActor(
			World,
			TEXT("TS_LookdevExplosion_Floor"),
			TEXT("TS_LookdevExplosion_Floor"),
			CubeMesh,
			FloorMaterial,
			FVector(0.0f, 0.0f, -8.0f),
			FRotator::ZeroRotator,
			FVector(18.0f, 18.0f, 0.16f));

		SpawnLookdevStaticMeshActor(
			World,
			TEXT("TS_LookdevExplosion_BackWall"),
			TEXT("TS_LookdevExplosion_BackWall"),
			CubeMesh,
			FloorMaterial,
			FVector(430.0f, 500.0f, 245.0f),
			FRotator(0.0f, -38.0f, 0.0f),
			FVector(13.0f, 0.16f, 5.0f));

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ADirectionalLight::StaticClass(), TEXT("TS_LookdevExplosion_KeyLight"));
		ADirectionalLight* KeyLight = World->SpawnActor<ADirectionalLight>(
			FVector(-460.0f, -620.0f, 920.0f),
			FRotator(-47.0f, 34.0f, 0.0f),
			SpawnParameters);
		if (KeyLight)
		{
			KeyLight->SetActorLabel(TEXT("TS_LookdevExplosion_KeyLight"));
			KeyLight->SetMobility(EComponentMobility::Movable);
			if (ULightComponent* LightComponent = KeyLight->GetLightComponent())
			{
				LightComponent->SetMobility(EComponentMobility::Movable);
				LightComponent->SetIntensity(3.4f);
				LightComponent->SetLightColor(FLinearColor(1.0f, 0.88f, 0.72f), false);
			}
			KeyLight->MarkPackageDirty();
		}

		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ASkyLight::StaticClass(), TEXT("TS_LookdevExplosion_SkyLight"));
		ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(
			FVector(0.0f, 0.0f, 760.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (SkyLight)
		{
			SkyLight->SetActorLabel(TEXT("TS_LookdevExplosion_SkyLight"));
			if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
			{
				SkyLightComponent->SetMobility(EComponentMobility::Movable);
				SkyLightComponent->SetIntensity(0.34f);
				SkyLightComponent->SetLightColor(FLinearColor(0.62f, 0.72f, 0.95f));
				SkyLightComponent->SetRealTimeCapture(false);
			}
			SkyLight->MarkPackageDirty();
		}

		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, APointLight::StaticClass(), TEXT("TS_LookdevExplosion_CoreLight"));
		APointLight* CoreLight = World->SpawnActor<APointLight>(
			FVector(0.0f, 0.0f, 95.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (CoreLight)
		{
			CoreLight->SetActorLabel(TEXT("TS_LookdevExplosion_CoreLight"));
			if (UPointLightComponent* PointLightComponent = CoreLight->PointLightComponent)
			{
				PointLightComponent->SetMobility(EComponentMobility::Movable);
				PointLightComponent->SetIntensity(1300.0f);
				PointLightComponent->SetAttenuationRadius(620.0f);
				PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.39f, 0.08f), false);
				PointLightComponent->SetCastShadows(false);
			}
			CoreLight->MarkPackageDirty();
		}

		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ANiagaraActor::StaticClass(), TEXT("TS_LookdevExplosion_NS"));
		ANiagaraActor* NiagaraActor = World->SpawnActor<ANiagaraActor>(
			FVector(0.0f, 0.0f, 0.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!NiagaraActor || !NiagaraActor->GetNiagaraComponent())
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to spawn lookdev Niagara actor."));
			return false;
		}

		NiagaraActor->SetActorLabel(TEXT("TS_LookdevExplosion_NS"));
		NiagaraActor->SetDestroyOnSystemFinish(false);
		UNiagaraComponent* NiagaraComponent = NiagaraActor->GetNiagaraComponent();
		NiagaraComponent->SetAsset(NiagaraSystem);
		NiagaraComponent->SetAutoActivate(true);
		NiagaraComponent->SetRelativeScale3D(FVector(1.0f));
		ApplyLookdevNiagaraComponentOverrides(NiagaraComponent);
		NiagaraActor->MarkPackageDirty();

		const FVector CameraLocation(-690.0f, -740.0f, 430.0f);
		const FVector CameraTarget(0.0f, 0.0f, 150.0f);
		const FRotator CameraRotation = (CameraTarget - CameraLocation).Rotation();
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ACameraActor::StaticClass(), TEXT("TS_LookdevExplosion_Camera"));
		ACameraActor* CameraActor = World->SpawnActor<ACameraActor>(
			CameraLocation,
			CameraRotation,
			SpawnParameters);
		if (!CameraActor || !CameraActor->GetCameraComponent())
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to spawn lookdev camera actor."));
			return false;
		}

		CameraActor->SetActorLabel(TEXT("TS_LookdevExplosion_Camera"));
		UCameraComponent* CameraComponent = CameraActor->GetCameraComponent();
		CameraComponent->SetFieldOfView(38.0f);
		CameraComponent->SetAspectRatio(16.0f / 9.0f);
		CameraComponent->SetConstraintAspectRatio(true);
		CameraActor->MarkPackageDirty();

		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, APlayerStart::StaticClass(), TEXT("TS_LookdevExplosion_PlayerStart"));
		APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(
			CameraLocation + FVector(80.0f, 80.0f, -60.0f),
			CameraRotation,
			SpawnParameters);
		if (PlayerStart)
		{
			PlayerStart->SetActorLabel(TEXT("TS_LookdevExplosion_PlayerStart"));
			PlayerStart->MarkPackageDirty();
		}

		SpawnParameters.Name = MakeUniqueObjectName(
			World->PersistentLevel,
			ATunaSweeperLookdevCameraDirectorActor::StaticClass(),
			TEXT("TS_LookdevExplosion_CameraDirector"));
		ATunaSweeperLookdevCameraDirectorActor* CameraDirector =
			World->SpawnActor<ATunaSweeperLookdevCameraDirectorActor>(
				FVector(-140.0f, -140.0f, 80.0f),
				FRotator::ZeroRotator,
				SpawnParameters);
		if (CameraDirector)
		{
			CameraDirector->SetActorLabel(TEXT("TS_LookdevExplosion_CameraDirector"));
			CameraDirector->ConfigureLookdev(CameraActor, NiagaraActor);
			CameraDirector->MarkPackageDirty();
		}

		World->MarkPackageDirty();
		const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, LookdevFluidExplosionMapPackagePath);
		if (!bSaved)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *LookdevFluidExplosionMapPackagePath);
			return false;
		}

		UE_LOG(LogTunaSweeperEditor, Display, TEXT("Lookdev fluid explosion capture map saved: %s"), *LookdevFluidExplosionMapPackagePath);
		return true;
	}

	bool EnsureLookdevFluidExplosionAssetsAndLevel()
	{
		UNiagaraSystem* NiagaraSystem = EnsureLookdevFluidExplosionNiagaraSystem();
		return NiagaraSystem && EnsureLookdevFluidExplosionCaptureLevel(NiagaraSystem);
	}

	UMaterial* EnsureMemoStorageDeviceMaterial(UTexture2D* StorageTexture)
	{
		if (!StorageTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, MemoStorageDeviceMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				MemoStorageDeviceMaterialAssetName,
				InteractionAssetPath,
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
		TextureCoordinateExpression->MaterialExpressionEditorX = -620;
		TextureCoordinateExpression->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->Material = Material;
		TextureSample->ParameterName = TEXT("StorageDeviceTexture");
		TextureSample->Texture = StorageTexture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		TextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		TextureSample->MaterialExpressionEditorX = -360;
		TextureSample->MaterialExpressionEditorY = 40;
		TextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(TextureSample);

		UMaterialExpressionScalarParameter* EmissiveStrengthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		EmissiveStrengthParameter->Material = Material;
		EmissiveStrengthParameter->ParameterName = TEXT("EmissiveStrength");
		EmissiveStrengthParameter->DefaultValue = 0.14f;
		EmissiveStrengthParameter->MaterialExpressionEditorX = -360;
		EmissiveStrengthParameter->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(EmissiveStrengthParameter);

		UMaterialExpressionMultiply* EmissiveMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMultiply->Material = Material;
		EmissiveMultiply->A.Connect(0, TextureSample);
		EmissiveMultiply->B.Connect(0, EmissiveStrengthParameter);
		EmissiveMultiply->MaterialExpressionEditorX = -80;
		EmissiveMultiply->MaterialExpressionEditorY = 170;
		Material->GetExpressionCollection().AddExpression(EmissiveMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, TextureSample);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMultiply);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.78f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.25f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool ImportMemoStorageDeviceTextureFromCommandLineIfRequested()
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureSource="), SourceFile))
		{
			return false;
		}

		UTexture2D* ImportedTexture = nullptr;
		const bool bImported = ImportWorldTexture(
			SourceFile,
			InteractionAssetPath,
			MemoStorageDeviceTextureAssetName,
			&ImportedTexture);
		UMaterial* Material = bImported ? EnsureMemoStorageDeviceMaterial(ImportedTexture) : nullptr;
		const bool bSucceeded = ImportedTexture && Material;
		if (!bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import memo storage device texture/material."));
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}

		return bSucceeded;
	}

	UMaterial* EnsureRollingBomberSpawnerMaterial(UTexture2D* SpawnerTexture)
	{
		if (!SpawnerTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, RollingBomberSpawnerMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				RollingBomberSpawnerMaterialAssetName,
				InteractionAssetPath,
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
		Material->TwoSided = false;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -760;
		TextureCoordinateExpression->MaterialExpressionEditorY = 60;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->Material = Material;
		TextureSample->ParameterName = TEXT("SpawnerMechanicTexture");
		TextureSample->Texture = SpawnerTexture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		TextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		TextureSample->MaterialExpressionEditorX = -500;
		TextureSample->MaterialExpressionEditorY = -20;
		TextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(TextureSample);

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -500;
		VertexColorExpression->MaterialExpressionEditorY = 210;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionMultiply* BaseColorMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		BaseColorMultiply->Material = Material;
		BaseColorMultiply->A.Connect(0, TextureSample);
		BaseColorMultiply->B.Connect(0, VertexColorExpression);
		BaseColorMultiply->MaterialExpressionEditorX = -180;
		BaseColorMultiply->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(BaseColorMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, BaseColorMultiply);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.72f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.12f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.35f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool ImportRollingBomberSpawnerTextureFromCommandLineIfRequested()
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureSource="), SourceFile))
		{
			return false;
		}

		UTexture2D* ImportedTexture = nullptr;
		const bool bImported = ImportWorldTexture(
			SourceFile,
			InteractionAssetPath,
			RollingBomberSpawnerTextureAssetName,
			&ImportedTexture);
		UMaterial* Material = bImported ? EnsureRollingBomberSpawnerMaterial(ImportedTexture) : nullptr;
		const bool bSucceeded = ImportedTexture && Material;
		if (!bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import rolling bomber spawner texture/material."));
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}

		return bSucceeded;
	}

	FString GetSandbagCoverTextureSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectContentDir(),
			TEXT("SourceArt"),
			TEXT("SandbagCover"),
			TEXT("T_SandbagCover_Burlap_Source.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	UMaterial* EnsureSandbagCoverMaterial(UTexture2D* SandbagTexture)
	{
		if (!SandbagTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, SandbagCoverMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				SandbagCoverMaterialAssetName,
				InteractionAssetPath,
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
		Material->TwoSided = false;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -860;
		TextureCoordinateExpression->MaterialExpressionEditorY = 40;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->Material = Material;
		TextureSample->ParameterName = TEXT("SandbagTexture");
		TextureSample->Texture = SandbagTexture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		TextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		TextureSample->MaterialExpressionEditorX = -620;
		TextureSample->MaterialExpressionEditorY = -20;
		TextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(TextureSample);

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -620;
		VertexColorExpression->MaterialExpressionEditorY = 190;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionMultiply* BaseColorMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		BaseColorMultiply->Material = Material;
		BaseColorMultiply->A.Connect(0, TextureSample);
		BaseColorMultiply->B.Connect(0, VertexColorExpression);
		BaseColorMultiply->MaterialExpressionEditorX = -360;
		BaseColorMultiply->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(BaseColorMultiply);

		UMaterialExpressionVectorParameter* DamageTintParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		DamageTintParameter->Material = Material;
		DamageTintParameter->ParameterName = TEXT("DamageTint");
		DamageTintParameter->DefaultValue = FLinearColor(0.46f, 0.37f, 0.26f, 1.0f);
		DamageTintParameter->MaterialExpressionEditorX = -360;
		DamageTintParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(DamageTintParameter);

		UMaterialExpressionMultiply* DamagedColorMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		DamagedColorMultiply->Material = Material;
		DamagedColorMultiply->A.Connect(0, BaseColorMultiply);
		DamagedColorMultiply->B.Connect(0, DamageTintParameter);
		DamagedColorMultiply->MaterialExpressionEditorX = -80;
		DamagedColorMultiply->MaterialExpressionEditorY = 210;
		Material->GetExpressionCollection().AddExpression(DamagedColorMultiply);

		UMaterialExpressionScalarParameter* DamageAlphaParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		DamageAlphaParameter->Material = Material;
		DamageAlphaParameter->ParameterName = TEXT("DamageAlpha");
		DamageAlphaParameter->DefaultValue = 0.0f;
		DamageAlphaParameter->MaterialExpressionEditorX = -80;
		DamageAlphaParameter->MaterialExpressionEditorY = 390;
		Material->GetExpressionCollection().AddExpression(DamageAlphaParameter);

		UMaterialExpressionLinearInterpolate* DamageLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
		DamageLerp->Material = Material;
		DamageLerp->A.Connect(0, BaseColorMultiply);
		DamageLerp->B.Connect(0, DamagedColorMultiply);
		DamageLerp->Alpha.Connect(0, DamageAlphaParameter);
		DamageLerp->MaterialExpressionEditorX = 170;
		DamageLerp->MaterialExpressionEditorY = 110;
		Material->GetExpressionCollection().AddExpression(DamageLerp);

		UMaterialExpressionScalarParameter* AmbientLiftParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		AmbientLiftParameter->Material = Material;
		AmbientLiftParameter->ParameterName = TEXT("AmbientLift");
		AmbientLiftParameter->DefaultValue = 0.18f;
		AmbientLiftParameter->MaterialExpressionEditorX = 170;
		AmbientLiftParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(AmbientLiftParameter);

		UMaterialExpressionMultiply* AmbientLiftMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		AmbientLiftMultiply->Material = Material;
		AmbientLiftMultiply->A.Connect(0, DamageLerp);
		AmbientLiftMultiply->B.Connect(0, AmbientLiftParameter);
		AmbientLiftMultiply->MaterialExpressionEditorX = 430;
		AmbientLiftMultiply->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(AmbientLiftMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, DamageLerp);
		MaterialEditorOnly->EmissiveColor.Connect(0, AmbientLiftMultiply);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.88f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.18f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UMaterial* EnsureSandbagCoverOverlayOutlineMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, SandbagCoverOverlayOutlineMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				SandbagCoverOverlayOutlineMaterialAssetName,
				InteractionAssetPath,
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
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->MaxWorldPositionOffsetDisplacement = 24.0f;
		Material->bAlwaysEvaluateWorldPositionOffset = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVectorParameter* ColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParameter->Material = Material;
		ColorParameter->ParameterName = TEXT("OutlineColor");
		ColorParameter->DefaultValue = FLinearColor(0.78f, 0.98f, 0.32f, 1.0f);
		ColorParameter->MaterialExpressionEditorX = -540;
		ColorParameter->MaterialExpressionEditorY = -180;
		Material->GetExpressionCollection().AddExpression(ColorParameter);

		UMaterialExpressionScalarParameter* ThicknessParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		ThicknessParameter->Material = Material;
		ThicknessParameter->ParameterName = TEXT("OutlineThickness");
		ThicknessParameter->DefaultValue = 3.0f;
		ThicknessParameter->MaterialExpressionEditorX = -540;
		ThicknessParameter->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(ThicknessParameter);

		UMaterialExpressionScalarParameter* StencilMaskValueParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		StencilMaskValueParameter->Material = Material;
		StencilMaskValueParameter->ParameterName = TEXT("StencilMaskValue");
		StencilMaskValueParameter->DefaultValue = 3.0f;
		StencilMaskValueParameter->MaterialExpressionEditorX = -540;
		StencilMaskValueParameter->MaterialExpressionEditorY = 620;
		Material->GetExpressionCollection().AddExpression(StencilMaskValueParameter);

		UMaterialExpressionVertexNormalWS* VertexNormalExpression = NewObject<UMaterialExpressionVertexNormalWS>(Material);
		VertexNormalExpression->Material = Material;
		VertexNormalExpression->MaterialExpressionEditorX = -540;
		VertexNormalExpression->MaterialExpressionEditorY = 220;
		Material->GetExpressionCollection().AddExpression(VertexNormalExpression);

		UMaterialExpressionMultiply* NormalOffsetExpression = NewObject<UMaterialExpressionMultiply>(Material);
		NormalOffsetExpression->Material = Material;
		NormalOffsetExpression->A.Connect(0, VertexNormalExpression);
		NormalOffsetExpression->B.Connect(0, ThicknessParameter);
		NormalOffsetExpression->MaterialExpressionEditorX = -230;
		NormalOffsetExpression->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(NormalOffsetExpression);

		UMaterialExpressionTwoSidedSign* TwoSidedSignExpression = NewObject<UMaterialExpressionTwoSidedSign>(Material);
		TwoSidedSignExpression->Material = Material;
		TwoSidedSignExpression->MaterialExpressionEditorX = -540;
		TwoSidedSignExpression->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(TwoSidedSignExpression);

		UMaterialExpressionConstant* HiddenFaceMaskValue = NewObject<UMaterialExpressionConstant>(Material);
		HiddenFaceMaskValue->Material = Material;
		HiddenFaceMaskValue->R = 0.0f;
		HiddenFaceMaskValue->MaterialExpressionEditorX = -260;
		HiddenFaceMaskValue->MaterialExpressionEditorY = 420;
		Material->GetExpressionCollection().AddExpression(HiddenFaceMaskValue);

		UMaterialExpressionConstant* InvertedFaceMaskValue = NewObject<UMaterialExpressionConstant>(Material);
		InvertedFaceMaskValue->Material = Material;
		InvertedFaceMaskValue->R = 1.0f;
		InvertedFaceMaskValue->MaterialExpressionEditorX = -260;
		InvertedFaceMaskValue->MaterialExpressionEditorY = 560;
		Material->GetExpressionCollection().AddExpression(InvertedFaceMaskValue);

		UMaterialExpressionIf* BackFaceOnlyMask = NewObject<UMaterialExpressionIf>(Material);
		BackFaceOnlyMask->Material = Material;
		BackFaceOnlyMask->A.Connect(0, TwoSidedSignExpression);
		BackFaceOnlyMask->ConstB = 0.0f;
		BackFaceOnlyMask->AGreaterThanB.Connect(0, InvertedFaceMaskValue);
		BackFaceOnlyMask->AEqualsB.Connect(0, HiddenFaceMaskValue);
		BackFaceOnlyMask->ALessThanB.Connect(0, HiddenFaceMaskValue);
		BackFaceOnlyMask->MaterialExpressionEditorX = 40;
		BackFaceOnlyMask->MaterialExpressionEditorY = 500;
		Material->GetExpressionCollection().AddExpression(BackFaceOnlyMask);

		UMaterialExpressionScreenPosition* ScreenPositionExpression = NewObject<UMaterialExpressionScreenPosition>(Material);
		ScreenPositionExpression->Material = Material;
		ScreenPositionExpression->MaterialExpressionEditorX = -540;
		ScreenPositionExpression->MaterialExpressionEditorY = 780;
		Material->GetExpressionCollection().AddExpression(ScreenPositionExpression);

		UMaterialExpressionSceneTexture* CustomStencilTexture = NewObject<UMaterialExpressionSceneTexture>(Material);
		CustomStencilTexture->Material = Material;
		CustomStencilTexture->SceneTextureId = PPI_CustomStencil;
		CustomStencilTexture->Coordinates.Connect(0, ScreenPositionExpression);
		CustomStencilTexture->MaterialExpressionEditorX = -250;
		CustomStencilTexture->MaterialExpressionEditorY = 760;
		Material->GetExpressionCollection().AddExpression(CustomStencilTexture);

		UMaterialExpressionComponentMask* CustomStencilValue = NewObject<UMaterialExpressionComponentMask>(Material);
		CustomStencilValue->Material = Material;
		CustomStencilValue->Input.Connect(0, CustomStencilTexture);
		CustomStencilValue->R = true;
		CustomStencilValue->G = false;
		CustomStencilValue->B = false;
		CustomStencilValue->A = false;
		CustomStencilValue->MaterialExpressionEditorX = 40;
		CustomStencilValue->MaterialExpressionEditorY = 760;
		Material->GetExpressionCollection().AddExpression(CustomStencilValue);

		UMaterialExpressionIf* StencilOutsideMask = NewObject<UMaterialExpressionIf>(Material);
		StencilOutsideMask->Material = Material;
		StencilOutsideMask->A.Connect(0, CustomStencilValue);
		StencilOutsideMask->B.Connect(0, StencilMaskValueParameter);
		StencilOutsideMask->AGreaterThanB.Connect(0, InvertedFaceMaskValue);
		StencilOutsideMask->AEqualsB.Connect(0, HiddenFaceMaskValue);
		StencilOutsideMask->ALessThanB.Connect(0, InvertedFaceMaskValue);
		StencilOutsideMask->MaterialExpressionEditorX = 310;
		StencilOutsideMask->MaterialExpressionEditorY = 720;
		Material->GetExpressionCollection().AddExpression(StencilOutsideMask);

		UMaterialExpressionMultiply* FinalOpacity = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacity->Material = Material;
		FinalOpacity->A.Connect(0, BackFaceOnlyMask);
		FinalOpacity->B.Connect(0, StencilOutsideMask);
		FinalOpacity->MaterialExpressionEditorX = 580;
		FinalOpacity->MaterialExpressionEditorY = 590;
		Material->GetExpressionCollection().AddExpression(FinalOpacity);

		MaterialEditorOnly->EmissiveColor.Connect(0, ColorParameter);
		MaterialEditorOnly->WorldPositionOffset.Connect(0, NormalOffsetExpression);
		MaterialEditorOnly->Opacity.Connect(0, FinalOpacity);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	FVector3f MakeSandbagCoverSafeTangent(const FVector3f& Normal, const FVector3f& PreferredTangent)
	{
		FVector3f Tangent = PreferredTangent - Normal * FVector3f::DotProduct(Normal, PreferredTangent);
		if (Tangent.IsNearlyZero())
		{
			const FVector3f Fallback = FMath::Abs(Normal.Z) < 0.85f
				? FVector3f(0.0f, 0.0f, 1.0f)
				: FVector3f(1.0f, 0.0f, 0.0f);
			Tangent = Fallback - Normal * FVector3f::DotProduct(Normal, Fallback);
		}

		return Tangent.IsNearlyZero() ? FVector3f(1.0f, 0.0f, 0.0f) : Tangent.GetSafeNormal();
	}

	void BuildSandbagLoafMeshDescription(
		FMeshDescription& MeshDescription,
		const FVector3f& Extent,
		int32 SegmentCount,
		const TArray<float>& RingPositions,
		const TArray<float>& RingWidthScales,
		const TArray<float>& RingHeightScales,
		const FLinearColor& VertexColor)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Sandbag"));

		const int32 RingCount = RingPositions.Num();
		if (RingCount < 2 || RingWidthScales.Num() != RingCount || RingHeightScales.Num() != RingCount || SegmentCount < 3)
		{
			return;
		}

		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();

		TArray<FVector3f> RingVertices;
		TArray<FVector3f> RingNormals;
		TArray<FVector3f> RingTangents;
		TArray<FVertexID> RingVertexIds;
		RingVertices.SetNum(RingCount * SegmentCount);
		RingNormals.SetNum(RingCount * SegmentCount);
		RingTangents.SetNum(RingCount * SegmentCount);
		RingVertexIds.SetNum(RingCount * SegmentCount);

		auto GetRingArrayIndex = [SegmentCount](int32 RingIndex, int32 SegmentIndex) -> int32
		{
			return RingIndex * SegmentCount + (SegmentIndex % SegmentCount);
		};

		auto GetRingNormal = [&RingNormals, &GetRingArrayIndex](int32 RingIndex, int32 SegmentIndex) -> const FVector3f&
		{
			return RingNormals[GetRingArrayIndex(RingIndex, SegmentIndex)];
		};

		auto GetRingTangent = [&RingTangents, &GetRingArrayIndex](int32 RingIndex, int32 SegmentIndex) -> const FVector3f&
		{
			return RingTangents[GetRingArrayIndex(RingIndex, SegmentIndex)];
		};

		auto GetRingVertexId = [&RingVertexIds, &GetRingArrayIndex](int32 RingIndex, int32 SegmentIndex) -> FVertexID
		{
			return RingVertexIds[GetRingArrayIndex(RingIndex, SegmentIndex)];
		};

		auto CreateMeshVertex = [&MeshDescription, &VertexPositions](const FVector3f& Position) -> FVertexID
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Position;
			return VertexId;
		};

		auto CreateSandbagVertexInstance =
			[&MeshDescription, &VertexInstanceNormals, &VertexInstanceTangents, &VertexInstanceBinormalSigns, &VertexInstanceUVs, &VertexInstanceColors, &VertexColor](
				FVertexID VertexId,
				const FVector3f& Normal,
				const FVector3f& Tangent,
				const FVector2f& UV) -> FVertexInstanceID
		{
			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normal;
			VertexInstanceTangents[VertexInstanceId] = Tangent;
			VertexInstanceBinormalSigns[VertexInstanceId] = 1.0f;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UV);
			VertexInstanceColors[VertexInstanceId] = FVector4f(VertexColor.R, VertexColor.G, VertexColor.B, VertexColor.A);
			return VertexInstanceId;
		};

		for (int32 RingIndex = 0; RingIndex < RingCount; ++RingIndex)
		{
			const float RingY = RingPositions[RingIndex] * Extent.Y;
			const float WidthScale = RingWidthScales[RingIndex];
			const float HeightScale = RingHeightScales[RingIndex];
			const float SafeWidth = FMath::Max(1.0f, Extent.X * WidthScale);
			const float SafeHeight = FMath::Max(1.0f, Extent.Z * HeightScale);
			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
				const float Angle = Alpha * 2.0f * UE_PI;
				const int32 RingArrayIndex = GetRingArrayIndex(RingIndex, SegmentIndex);
				RingVertices[RingArrayIndex] = FVector3f(
					FMath::Cos(Angle) * Extent.X * WidthScale,
					RingY,
					FMath::Sin(Angle) * Extent.Z * HeightScale);
				RingNormals[RingArrayIndex] = FVector3f(
					FMath::Cos(Angle) / SafeWidth,
					0.0f,
					FMath::Sin(Angle) / SafeHeight).GetSafeNormal();
				if (RingNormals[RingArrayIndex].IsNearlyZero())
				{
					RingNormals[RingArrayIndex] = FVector3f(0.0f, 0.0f, 1.0f);
				}
				RingTangents[RingArrayIndex] = MakeSandbagCoverSafeTangent(RingNormals[RingArrayIndex], FVector3f(0.0f, 1.0f, 0.0f));
				RingVertexIds[RingArrayIndex] = CreateMeshVertex(RingVertices[RingArrayIndex]);
			}
		}

		for (int32 RingIndex = 0; RingIndex < RingCount - 1; ++RingIndex)
		{
			const float V0 = static_cast<float>(RingIndex) / static_cast<float>(RingCount - 1);
			const float V1 = static_cast<float>(RingIndex + 1) / static_cast<float>(RingCount - 1);
			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const int32 NextSegmentIndex = (SegmentIndex + 1) % SegmentCount;
				const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
				const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);

				{
					TArray<FVertexInstanceID> VertexInstances;
					VertexInstances.Reserve(3);
					VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingIndex, SegmentIndex), GetRingNormal(RingIndex, SegmentIndex), GetRingTangent(RingIndex, SegmentIndex), FVector2f(U0, V0)));
					VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingIndex + 1, SegmentIndex), GetRingNormal(RingIndex + 1, SegmentIndex), GetRingTangent(RingIndex + 1, SegmentIndex), FVector2f(U0, V1)));
					VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingIndex + 1, NextSegmentIndex), GetRingNormal(RingIndex + 1, NextSegmentIndex), GetRingTangent(RingIndex + 1, NextSegmentIndex), FVector2f(U1, V1)));
					MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
				}
				{
					TArray<FVertexInstanceID> VertexInstances;
					VertexInstances.Reserve(3);
					VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingIndex, SegmentIndex), GetRingNormal(RingIndex, SegmentIndex), GetRingTangent(RingIndex, SegmentIndex), FVector2f(U0, V0)));
					VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingIndex + 1, NextSegmentIndex), GetRingNormal(RingIndex + 1, NextSegmentIndex), GetRingTangent(RingIndex + 1, NextSegmentIndex), FVector2f(U1, V1)));
					VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingIndex, NextSegmentIndex), GetRingNormal(RingIndex, NextSegmentIndex), GetRingTangent(RingIndex, NextSegmentIndex), FVector2f(U1, V0)));
					MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
				}
			}
		}

		const FVector3f MinEndCenter(0.0f, RingPositions[0] * Extent.Y, 0.0f);
		const FVector3f MaxEndCenter(0.0f, RingPositions.Last() * Extent.Y, 0.0f);
		const FVertexID MinEndCenterVertexId = CreateMeshVertex(MinEndCenter);
		const FVertexID MaxEndCenterVertexId = CreateMeshVertex(MaxEndCenter);
		const FVector3f MinEndNormal(0.0f, -1.0f, 0.0f);
		const FVector3f MaxEndNormal(0.0f, 1.0f, 0.0f);
		const FVector3f EndTangent(1.0f, 0.0f, 0.0f);
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const int32 NextSegmentIndex = (SegmentIndex + 1) % SegmentCount;
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);

			{
				TArray<FVertexInstanceID> VertexInstances;
				VertexInstances.Reserve(3);
				VertexInstances.Add(CreateSandbagVertexInstance(MinEndCenterVertexId, MinEndNormal, EndTangent, FVector2f(0.5f, 0.5f)));
				VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(0, SegmentIndex), GetRingNormal(0, SegmentIndex), GetRingTangent(0, SegmentIndex), FVector2f(U0, 0.0f)));
				VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(0, NextSegmentIndex), GetRingNormal(0, NextSegmentIndex), GetRingTangent(0, NextSegmentIndex), FVector2f(U1, 0.0f)));
				MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
			}
			{
				TArray<FVertexInstanceID> VertexInstances;
				VertexInstances.Reserve(3);
				VertexInstances.Add(CreateSandbagVertexInstance(MaxEndCenterVertexId, MaxEndNormal, EndTangent, FVector2f(0.5f, 0.5f)));
				VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingCount - 1, NextSegmentIndex), GetRingNormal(RingCount - 1, NextSegmentIndex), GetRingTangent(RingCount - 1, NextSegmentIndex), FVector2f(U1, 1.0f)));
				VertexInstances.Add(CreateSandbagVertexInstance(GetRingVertexId(RingCount - 1, SegmentIndex), GetRingNormal(RingCount - 1, SegmentIndex), GetRingTangent(RingCount - 1, SegmentIndex), FVector2f(U0, 1.0f)));
				MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
			}
		}
	}

	void BuildSandbagLowPolyMeshDescription(FMeshDescription& MeshDescription)
	{
		BuildSandbagLoafMeshDescription(
			MeshDescription,
			FVector3f(21.0f, 28.0f, 9.0f),
			8,
			{ -1.0f, -0.68f, -0.24f, 0.24f, 0.68f, 1.0f },
			{ 0.46f, 0.88f, 1.0f, 1.0f, 0.88f, 0.46f },
			{ 0.58f, 0.92f, 1.0f, 1.0f, 0.92f, 0.58f },
			FLinearColor(0.94f, 0.82f, 0.58f, 1.0f));
	}

	UStaticMesh* EnsureSandbagCoverStaticMeshAsset(
		const FString& AssetName,
		UMaterialInterface* SandbagMaterial,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription)
	{
		if (!SandbagMaterial)
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
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(SandbagMaterial, FName(TEXT("Sandbag"))));

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

		const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
		UE_LOG(
			LogTunaSweeperEditor,
			Display,
			TEXT("Built low-poly sandbag mesh %s bounds origin=%s extent=%s radius=%.2f"),
			*AssetName,
			*MeshBounds.Origin.ToString(),
			*MeshBounds.BoxExtent.ToString(),
			MeshBounds.SphereRadius);
		StaticMesh->PostEditChange();
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureSandbagCoverAssets()
	{
		UTexture2D* SandbagTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(InteractionAssetPath, SandbagCoverTextureAssetName));
		if (!SandbagTexture)
		{
			const FString SourcePath = GetSandbagCoverTextureSourcePath();
			if (!FPaths::FileExists(SourcePath) ||
				!ImportWorldTexture(SourcePath, InteractionAssetPath, SandbagCoverTextureAssetName, &SandbagTexture))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import sandbag cover texture source: %s"), *SourcePath);
				return false;
			}
		}

		UMaterial* SandbagMaterial = EnsureSandbagCoverMaterial(SandbagTexture);
		UMaterial* OverlayOutlineMaterial = EnsureSandbagCoverOverlayOutlineMaterial();
		UStaticMesh* SandbagMesh = EnsureSandbagCoverStaticMeshAsset(
			SandbagCoverBagMeshAssetName,
			SandbagMaterial,
			BuildSandbagLowPolyMeshDescription);
		UBlueprint* SandbagBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			SandbagCoverAssetName,
			ATunaSweeperSandbagCoverActor::StaticClass());
		if (!SandbagMaterial || !OverlayOutlineMaterial || !SandbagMesh || !SandbagBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(SandbagBlueprint);
		ATunaSweeperSandbagCoverActor* Defaults = SandbagBlueprint->GeneratedClass
			? Cast<ATunaSweeperSandbagCoverActor>(SandbagBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(SandbagBlueprint));
			return false;
		}

		SandbagBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureCoverDefaults(
			FName(TEXT("TS_SandbagCover_Default")),
			FVector(37.5f, 160.0f, 60.0f),
			70.0f,
			62.5f);
		Defaults->ConfigureCoverVisualDefaults(
			TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, SandbagCoverMaterialAssetName))),
			TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, SandbagCoverOverlayOutlineMaterialAssetName))));
		Defaults->ConfigureCoverMeshDefaults(
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, SandbagCoverBagMeshAssetName))));
		FBlueprintEditorUtils::MarkBlueprintAsModified(SandbagBlueprint);
		FKismetEditorUtilities::CompileBlueprint(SandbagBlueprint);
		SandbagBlueprint->MarkPackageDirty();
		return SaveAsset(SandbagBlueprint);
	}

	bool ImportSandbagCoverTextureFromCommandLineIfRequested()
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureSource="), SourceFile))
		{
			return false;
		}

		UTexture2D* ImportedTexture = nullptr;
		const bool bImported = ImportWorldTexture(
			SourceFile,
			InteractionAssetPath,
			SandbagCoverTextureAssetName,
			&ImportedTexture);
		const bool bSucceeded = bImported && EnsureSandbagCoverAssets();
		if (!bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import sandbag cover texture/material/blueprint."));
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}

		return bSucceeded;
	}

}
