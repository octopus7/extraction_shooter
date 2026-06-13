#include "TunaSweeperProceduralTerrainTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "CoreMinimal.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Factories/MaterialFactoryNew.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Materials/MaterialExpressionLandscapeLayerCoords.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperProceduralTerrainTest, Log, All);

namespace TunaSweeperProceduralTerrainTest
{
	const FString AssetPath = TEXT("/Game/Prototype/TerrainTest");
	const FString MapPackagePath = TEXT("/Game/PrototypeTerrainPathCreekMap");
	const FString LandscapeMaterialAssetName = TEXT("M_TerrainTest_Landscape");
	const FString CreekWaterMaterialAssetName = TEXT("M_TerrainTest_CreekWater");
	const FString CreekWaterRippleMaterialAssetName = TEXT("M_TerrainTest_CreekWater_Ripples");
	const FString CreekWaterMeshAssetName = TEXT("SM_TerrainTest_CreekWater");
	const FString CreekWaterTextureAssetName = TEXT("T_TerrainTest_CreekWater");
	const FString CreekWaterSourceArtFileName = TEXT("T_TerrainTest_CreekWater_Imagegen.png");

	constexpr int32 TextureSize = 1024;
	constexpr int32 ComponentCount = 4;
	constexpr int32 NumSubsections = 1;
	constexpr int32 SubsectionSizeQuads = 63;
	constexpr int32 LandscapeQuads = ComponentCount * NumSubsections * SubsectionSizeQuads;
	constexpr int32 LandscapeVerts = LandscapeQuads + 1;
	constexpr float LandscapeScaleXY = 40.0f;
	constexpr float LandscapeScaleZ = 28.0f;
	constexpr float LandscapeWorldSize = LandscapeQuads * LandscapeScaleXY;
	constexpr float LandscapeHalfWorldSize = LandscapeWorldSize * 0.5f;
	constexpr float CreekSplineMargin = 420.0f;
	constexpr float CreekWaterHalfWidth = 180.0f;
	constexpr float CreekWaterZOffset = 42.0f;
	constexpr float CreekWaterRippleOverlayZOffset = 3.0f;
	constexpr float CreekWaterTextureWorldScale = 0.0018f;
	constexpr int32 CreekSplinePointCount = 15;
	constexpr int32 CreekWaterRibbonSampleCount = 96;
	constexpr float PlayerStartX = 520.0f;
	constexpr float PlayerStartY = -220.0f;
	constexpr float PlayerStartGroundClearance = 120.0f;

	enum class ETerrainLayer : uint8
	{
		Dirt,
		Grass,
		Rock,
		DarkDirt,
		Count
	};

	struct FTerrainLayerDefinition
	{
		ETerrainLayer Layer = ETerrainLayer::Grass;
		FName LayerName;
		FString TextureAssetName;
		FString LayerInfoAssetName;
		FString SourceArtFileName;
		FLinearColor DebugColor;
		float MappingScale = 160.0f;
		float PreviewWeight = 0.25f;
	};

	const TArray<FTerrainLayerDefinition>& GetLayerDefinitions()
	{
		static const TArray<FTerrainLayerDefinition> Definitions =
		{
			{
				ETerrainLayer::Dirt,
				TEXT("Dirt"),
				TEXT("T_TerrainTest_Dirt"),
				TEXT("LI_TerrainTest_Dirt"),
				TEXT("T_TerrainTest_Dirt_Imagegen.png"),
				FLinearColor(0.54f, 0.34f, 0.18f, 1.0f),
				55.0f,
				0.30f
			},
			{
				ETerrainLayer::Grass,
				TEXT("Grass"),
				TEXT("T_TerrainTest_Grass"),
				TEXT("LI_TerrainTest_Grass"),
				TEXT("T_TerrainTest_Grass_Imagegen.png"),
				FLinearColor(0.16f, 0.45f, 0.11f, 1.0f),
				34.0f,
				0.45f
			},
			{
				ETerrainLayer::Rock,
				TEXT("Rock"),
				TEXT("T_TerrainTest_Rock"),
				TEXT("LI_TerrainTest_Rock"),
				TEXT("T_TerrainTest_Rock_Imagegen.png"),
				FLinearColor(0.42f, 0.42f, 0.38f, 1.0f),
				72.0f,
				0.15f
			},
			{
				ETerrainLayer::DarkDirt,
				TEXT("DarkDirt"),
				TEXT("T_TerrainTest_DarkDirt"),
				TEXT("LI_TerrainTest_DarkDirt"),
				TEXT("T_TerrainTest_DarkDirt_Imagegen.png"),
				FLinearColor(0.16f, 0.11f, 0.08f, 1.0f),
				58.0f,
				0.10f
			}
		};
		return Definitions;
	}

	FString GetAssetObjectPath(const FString& InAssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *InAssetPath, *AssetName, *AssetName);
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

	float SmoothStep(float Edge0, float Edge1, float X)
	{
		if (FMath::IsNearlyEqual(Edge0, Edge1))
		{
			return X >= Edge1 ? 1.0f : 0.0f;
		}

		const float T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.0f, 1.0f);
		return T * T * (3.0f - 2.0f * T);
	}

	float BandMask(float Value, float Center, float HalfWidth, float Feather)
	{
		const float Distance = FMath::Abs(Value - Center);
		return 1.0f - SmoothStep(HalfWidth, HalfWidth + Feather, Distance);
	}

	float Noise01(float X, float Y, float Scale, float Seed)
	{
		return FMath::Clamp(FMath::PerlinNoise2D(FVector2D(X * Scale + Seed, Y * Scale - Seed)) * 0.5f + 0.5f, 0.0f, 1.0f);
	}

	FString GetTerrainSourceArtPath(const FString& FileName)
	{
		return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("SourceArt/TerrainTest"), FileName);
	}

	bool LoadSourceArtTexturePixels(const FString& SourceArtFileName, const FString& DebugName, TArray<FColor>& OutPixels)
	{
		if (SourceArtFileName.IsEmpty())
		{
			return false;
		}

		const FString SourceFile = GetTerrainSourceArtPath(SourceArtFileName);
		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Missing required source art for %s: %s."), *DebugName, *SourceFile);
			return false;
		}

		FImage SourceImage;
		if (!FImageUtils::LoadImage(*SourceFile, SourceImage))
		{
			UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to load source art for %s: %s."), *DebugName, *SourceFile);
			return false;
		}

		SourceImage.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		TArray<FColor> SourcePixels;
		const TArrayView64<FColor> SourceView = SourceImage.AsBGRA8();
		SourcePixels.Append(SourceView.GetData(), static_cast<int32>(SourceView.Num()));

		if (SourceImage.SizeX == TextureSize && SourceImage.SizeY == TextureSize)
		{
			OutPixels = MoveTemp(SourcePixels);
		}
		else
		{
			OutPixels.SetNumUninitialized(TextureSize * TextureSize);
			FImageUtils::ImageResize(
				SourceImage.SizeX,
				SourceImage.SizeY,
				SourcePixels,
				TextureSize,
				TextureSize,
				OutPixels,
				true,
				false);
		}

		for (FColor& Pixel : OutPixels)
		{
			Pixel.A = 255;
		}
		return OutPixels.Num() == TextureSize * TextureSize;
	}

	UTexture2D* EnsureTextureAssetFromSourceArt(const FString& TextureAssetName, const FString& SourceArtFileName)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, TextureAssetName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!Texture)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *TextureAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			Texture = NewObject<UTexture2D>(
				Package,
				*TextureAssetName,
				RF_Public | RF_Standalone | RF_Transactional);

			if (!Texture)
			{
				UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Texture);
		}

		TArray<FColor> Pixels;
		if (!LoadSourceArtTexturePixels(SourceArtFileName, TextureAssetName, Pixels))
		{
			return nullptr;
		}

		Texture->Modify();
		Texture->Source.Init(TextureSize, TextureSize, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->AddressX = TA_Wrap;
		Texture->AddressY = TA_Wrap;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();

		return SaveAsset(Texture) ? Texture : nullptr;
	}

	UTexture2D* EnsureTerrainTextureAsset(const FTerrainLayerDefinition& Definition)
	{
		return EnsureTextureAssetFromSourceArt(Definition.TextureAssetName, Definition.SourceArtFileName);
	}

	ULandscapeLayerInfoObject* EnsureLayerInfoAsset(const FTerrainLayerDefinition& Definition)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, Definition.LayerInfoAssetName);
		ULandscapeLayerInfoObject* LayerInfo = LoadObject<ULandscapeLayerInfoObject>(nullptr, *ObjectPath);
		if (!LayerInfo)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *Definition.LayerInfoAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			LayerInfo = NewObject<ULandscapeLayerInfoObject>(
				Package,
				*Definition.LayerInfoAssetName,
				RF_Public | RF_Standalone | RF_Transactional);

			if (!LayerInfo)
			{
				UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(LayerInfo);
		}

		LayerInfo->Modify();
		LayerInfo->SetLayerName(Definition.LayerName, false);
		LayerInfo->SetLayerUsageDebugColor(Definition.DebugColor, false, EPropertyChangeType::ValueSet);
		LayerInfo->SetHardness(Definition.Layer == ETerrainLayer::Rock ? 0.72f : 0.35f, false, EPropertyChangeType::ValueSet);
		LayerInfo->SetMinimumCollisionRelevanceWeight(0.10f, false, EPropertyChangeType::ValueSet);
		LayerInfo->SetBlendMethod(ELandscapeTargetLayerBlendMethod::FinalWeightBlending, false);
		LayerInfo->PostEditChange();
		LayerInfo->MarkPackageDirty();

		return SaveAsset(LayerInfo) ? LayerInfo : nullptr;
	}

	UMaterial* FindOrCreateMaterial(const FString& AssetName)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (Material)
		{
			return Material;
		}

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
			UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Material);
		return Material;
	}

	UMaterial* EnsureLandscapeMaterial(const TMap<ETerrainLayer, UTexture2D*>& Textures)
	{
		UMaterial* Material = FindOrCreateMaterial(LandscapeMaterialAssetName);
		if (!Material)
		{
			return nullptr;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Opaque;
		Material->TwoSided = false;
		Material->SetShadingModel(MSM_DefaultLit);

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			return nullptr;
		}

		UMaterialExpressionLandscapeLayerBlend* LayerBlend = NewObject<UMaterialExpressionLandscapeLayerBlend>(Material);
		LayerBlend->Material = Material;
		LayerBlend->MaterialExpressionEditorX = -260;
		LayerBlend->MaterialExpressionEditorY = 20;
		Material->GetExpressionCollection().AddExpression(LayerBlend);

		int32 TextureIndex = 0;
		for (const FTerrainLayerDefinition& Definition : GetLayerDefinitions())
		{
			UTexture2D* const* TexturePtr = Textures.Find(Definition.Layer);
			if (!TexturePtr || !*TexturePtr)
			{
				return nullptr;
			}

			UMaterialExpressionLandscapeLayerCoords* LayerCoordinates = NewObject<UMaterialExpressionLandscapeLayerCoords>(Material);
			LayerCoordinates->Material = Material;
			LayerCoordinates->MappingType = TCMT_XY;
			LayerCoordinates->MappingScale = Definition.MappingScale;
			LayerCoordinates->MaterialExpressionEditorX = -900;
			LayerCoordinates->MaterialExpressionEditorY = -220 + TextureIndex * 180;
			Material->GetExpressionCollection().AddExpression(LayerCoordinates);

			UMaterialExpressionTextureSample* TextureSample = NewObject<UMaterialExpressionTextureSample>(Material);
			TextureSample->Material = Material;
			TextureSample->Texture = *TexturePtr;
			TextureSample->Coordinates.Connect(0, LayerCoordinates);
			TextureSample->MaterialExpressionEditorX = -620;
			TextureSample->MaterialExpressionEditorY = -220 + TextureIndex * 180;
			TextureSample->AutoSetSampleType();
			Material->GetExpressionCollection().AddExpression(TextureSample);

			FLayerBlendInput BlendInput;
			BlendInput.LayerName = Definition.LayerName;
			BlendInput.BlendType = LB_WeightBlend;
			BlendInput.PreviewWeight = Definition.PreviewWeight;
			BlendInput.LayerInput.Connect(0, TextureSample);
			LayerBlend->Layers.Add(BlendInput);

			++TextureIndex;
		}

		MaterialEditorOnly->BaseColor.Connect(0, LayerBlend);
		MaterialEditorOnly->EmissiveColor.Expression = nullptr;
		MaterialEditorOnly->EmissiveColor.UseConstant = true;
		MaterialEditorOnly->EmissiveColor.Constant = FLinearColor::Black;
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.86f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.18f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		return SaveAsset(Material) ? Material : nullptr;
	}

	UMaterial* EnsureCreekWaterMaterial(UTexture2D* WaterTexture)
	{
		if (!WaterTexture)
		{
			return nullptr;
		}

		UMaterial* Material = FindOrCreateMaterial(CreekWaterMaterialAssetName);
		if (!Material)
		{
			return nullptr;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Translucent;
		Material->TwoSided = true;
		Material->bUsedWithSplineMeshes = true;
		Material->SetShadingModel(MSM_Unlit);

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			return nullptr;
		}

		UMaterialExpressionWorldPosition* WorldPosition = NewObject<UMaterialExpressionWorldPosition>(Material);
		WorldPosition->Material = Material;
		WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
		WorldPosition->MaterialExpressionEditorX = -920;
		WorldPosition->MaterialExpressionEditorY = -170;
		Material->GetExpressionCollection().AddExpression(WorldPosition);

		UMaterialExpressionComponentMask* WorldPositionXY = NewObject<UMaterialExpressionComponentMask>(Material);
		WorldPositionXY->Material = Material;
		WorldPositionXY->Input.Connect(0, WorldPosition);
		WorldPositionXY->R = 1;
		WorldPositionXY->G = 1;
		WorldPositionXY->B = 0;
		WorldPositionXY->A = 0;
		WorldPositionXY->MaterialExpressionEditorX = -700;
		WorldPositionXY->MaterialExpressionEditorY = -170;
		Material->GetExpressionCollection().AddExpression(WorldPositionXY);

		UMaterialExpressionMultiply* WaterTextureCoordinates = NewObject<UMaterialExpressionMultiply>(Material);
		WaterTextureCoordinates->Material = Material;
		WaterTextureCoordinates->A.Connect(0, WorldPositionXY);
		WaterTextureCoordinates->ConstB = CreekWaterTextureWorldScale;
		WaterTextureCoordinates->MaterialExpressionEditorX = -520;
		WaterTextureCoordinates->MaterialExpressionEditorY = -170;
		Material->GetExpressionCollection().AddExpression(WaterTextureCoordinates);

		UMaterialExpressionTextureSample* WaterTextureSample = NewObject<UMaterialExpressionTextureSample>(Material);
		WaterTextureSample->Material = Material;
		WaterTextureSample->Texture = WaterTexture;
		WaterTextureSample->Coordinates.Connect(0, WaterTextureCoordinates);
		WaterTextureSample->MaterialExpressionEditorX = -300;
		WaterTextureSample->MaterialExpressionEditorY = -80;
		WaterTextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(WaterTextureSample);

		UMaterialExpressionTextureCoordinate* RibbonUV = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		RibbonUV->Material = Material;
		RibbonUV->CoordinateIndex = 0;
		RibbonUV->MaterialExpressionEditorX = -920;
		RibbonUV->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(RibbonUV);

		UMaterialExpressionComponentMask* RibbonU = NewObject<UMaterialExpressionComponentMask>(Material);
		RibbonU->Material = Material;
		RibbonU->Input.Connect(0, RibbonUV);
		RibbonU->R = 1;
		RibbonU->G = 0;
		RibbonU->B = 0;
		RibbonU->A = 0;
		RibbonU->MaterialExpressionEditorX = -720;
		RibbonU->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(RibbonU);

		UMaterialExpressionSubtract* EdgeCentered = NewObject<UMaterialExpressionSubtract>(Material);
		EdgeCentered->Material = Material;
		EdgeCentered->A.Connect(0, RibbonU);
		EdgeCentered->ConstB = 0.5f;
		EdgeCentered->MaterialExpressionEditorX = -540;
		EdgeCentered->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(EdgeCentered);

		UMaterialExpressionAbs* EdgeAbs = NewObject<UMaterialExpressionAbs>(Material);
		EdgeAbs->Material = Material;
		EdgeAbs->Input.Connect(0, EdgeCentered);
		EdgeAbs->MaterialExpressionEditorX = -380;
		EdgeAbs->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(EdgeAbs);

		UMaterialExpressionMultiply* EdgeDistance = NewObject<UMaterialExpressionMultiply>(Material);
		EdgeDistance->Material = Material;
		EdgeDistance->A.Connect(0, EdgeAbs);
		EdgeDistance->ConstB = 2.0f;
		EdgeDistance->MaterialExpressionEditorX = -220;
		EdgeDistance->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(EdgeDistance);

		UMaterialExpressionSmoothStep* EdgeMask = NewObject<UMaterialExpressionSmoothStep>(Material);
		EdgeMask->Material = Material;
		EdgeMask->Value.Connect(0, EdgeDistance);
		EdgeMask->ConstMin = 0.58f;
		EdgeMask->ConstMax = 0.95f;
		EdgeMask->MaterialExpressionEditorX = -40;
		EdgeMask->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(EdgeMask);

		UMaterialExpressionTime* Time = NewObject<UMaterialExpressionTime>(Material);
		Time->Material = Material;
		Time->bIgnorePause = true;
		Time->MaterialExpressionEditorX = -540;
		Time->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(Time);

		UMaterialExpressionMultiply* TimeScale = NewObject<UMaterialExpressionMultiply>(Material);
		TimeScale->Material = Material;
		TimeScale->A.Connect(0, Time);
		TimeScale->ConstB = 0.16f;
		TimeScale->MaterialExpressionEditorX = -360;
		TimeScale->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(TimeScale);

		UMaterialExpressionSine* EdgeSine = NewObject<UMaterialExpressionSine>(Material);
		EdgeSine->Material = Material;
		EdgeSine->Input.Connect(0, TimeScale);
		EdgeSine->Period = 1.0f;
		EdgeSine->MaterialExpressionEditorX = -180;
		EdgeSine->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(EdgeSine);

		UMaterialExpressionMultiply* EdgeSineHalf = NewObject<UMaterialExpressionMultiply>(Material);
		EdgeSineHalf->Material = Material;
		EdgeSineHalf->A.Connect(0, EdgeSine);
		EdgeSineHalf->ConstB = 0.5f;
		EdgeSineHalf->MaterialExpressionEditorX = 0;
		EdgeSineHalf->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(EdgeSineHalf);

		UMaterialExpressionAdd* EdgeSineNormalized = NewObject<UMaterialExpressionAdd>(Material);
		EdgeSineNormalized->Material = Material;
		EdgeSineNormalized->A.Connect(0, EdgeSineHalf);
		EdgeSineNormalized->ConstB = 0.5f;
		EdgeSineNormalized->MaterialExpressionEditorX = 180;
		EdgeSineNormalized->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(EdgeSineNormalized);

		UMaterialExpressionMultiply* EdgePulseMask = NewObject<UMaterialExpressionMultiply>(Material);
		EdgePulseMask->Material = Material;
		EdgePulseMask->A.Connect(0, EdgeMask);
		EdgePulseMask->B.Connect(0, EdgeSineNormalized);
		EdgePulseMask->MaterialExpressionEditorX = 350;
		EdgePulseMask->MaterialExpressionEditorY = 330;
		Material->GetExpressionCollection().AddExpression(EdgePulseMask);

		UMaterialExpressionMultiply* EdgeEmissivePulse = NewObject<UMaterialExpressionMultiply>(Material);
		EdgeEmissivePulse->Material = Material;
		EdgeEmissivePulse->A.Connect(0, EdgePulseMask);
		EdgeEmissivePulse->ConstB = 0.10f;
		EdgeEmissivePulse->MaterialExpressionEditorX = 540;
		EdgeEmissivePulse->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(EdgeEmissivePulse);

		UMaterialExpressionMultiply* EdgeOpacityPulse = NewObject<UMaterialExpressionMultiply>(Material);
		EdgeOpacityPulse->Material = Material;
		EdgeOpacityPulse->A.Connect(0, EdgePulseMask);
		EdgeOpacityPulse->ConstB = 0.055f;
		EdgeOpacityPulse->MaterialExpressionEditorX = 540;
		EdgeOpacityPulse->MaterialExpressionEditorY = 410;
		Material->GetExpressionCollection().AddExpression(EdgeOpacityPulse);

		UMaterialExpressionMultiply* EmissiveLift = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveLift->Material = Material;
		EmissiveLift->A.Connect(0, WaterTextureSample);
		EmissiveLift->ConstB = 0.24f;
		EmissiveLift->MaterialExpressionEditorX = -160;
		EmissiveLift->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(EmissiveLift);

		UMaterialExpressionAdd* EmissiveWithEdgePulse = NewObject<UMaterialExpressionAdd>(Material);
		EmissiveWithEdgePulse->Material = Material;
		EmissiveWithEdgePulse->A.Connect(0, EmissiveLift);
		EmissiveWithEdgePulse->B.Connect(0, EdgeEmissivePulse);
		EmissiveWithEdgePulse->MaterialExpressionEditorX = 720;
		EmissiveWithEdgePulse->MaterialExpressionEditorY = 160;
		Material->GetExpressionCollection().AddExpression(EmissiveWithEdgePulse);

		UMaterialExpressionConstant* Opacity = NewObject<UMaterialExpressionConstant>(Material);
		Opacity->Material = Material;
		Opacity->R = 0.38f;
		Opacity->MaterialExpressionEditorX = -160;
		Opacity->MaterialExpressionEditorY = 280;
		Material->GetExpressionCollection().AddExpression(Opacity);

		UMaterialExpressionAdd* OpacityWithEdgePulse = NewObject<UMaterialExpressionAdd>(Material);
		OpacityWithEdgePulse->Material = Material;
		OpacityWithEdgePulse->A.Connect(0, Opacity);
		OpacityWithEdgePulse->B.Connect(0, EdgeOpacityPulse);
		OpacityWithEdgePulse->MaterialExpressionEditorX = 720;
		OpacityWithEdgePulse->MaterialExpressionEditorY = 330;
		Material->GetExpressionCollection().AddExpression(OpacityWithEdgePulse);

		MaterialEditorOnly->BaseColor.Connect(0, WaterTextureSample);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveWithEdgePulse);
		MaterialEditorOnly->Opacity.Connect(0, OpacityWithEdgePulse);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.24f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.72f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		return SaveAsset(Material) ? Material : nullptr;
	}

	UMaterial* EnsureCreekWaterRippleMaterial(UTexture2D* WaterTexture)
	{
		if (!WaterTexture)
		{
			return nullptr;
		}

		UMaterial* Material = FindOrCreateMaterial(CreekWaterRippleMaterialAssetName);
		if (!Material)
		{
			return nullptr;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Masked;
		Material->OpacityMaskClipValue = 0.42f;
		Material->TwoSided = true;
		Material->bUsedWithSplineMeshes = true;
		Material->SetShadingModel(MSM_Unlit);

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			return nullptr;
		}

		UMaterialExpressionWorldPosition* WorldPosition = NewObject<UMaterialExpressionWorldPosition>(Material);
		WorldPosition->Material = Material;
		WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
		WorldPosition->MaterialExpressionEditorX = -920;
		WorldPosition->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(WorldPosition);

		UMaterialExpressionComponentMask* WorldPositionXY = NewObject<UMaterialExpressionComponentMask>(Material);
		WorldPositionXY->Material = Material;
		WorldPositionXY->Input.Connect(0, WorldPosition);
		WorldPositionXY->R = 1;
		WorldPositionXY->G = 1;
		WorldPositionXY->B = 0;
		WorldPositionXY->A = 0;
		WorldPositionXY->MaterialExpressionEditorX = -700;
		WorldPositionXY->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(WorldPositionXY);

		UMaterialExpressionMultiply* WaterTextureCoordinates = NewObject<UMaterialExpressionMultiply>(Material);
		WaterTextureCoordinates->Material = Material;
		WaterTextureCoordinates->A.Connect(0, WorldPositionXY);
		WaterTextureCoordinates->ConstB = CreekWaterTextureWorldScale;
		WaterTextureCoordinates->MaterialExpressionEditorX = -520;
		WaterTextureCoordinates->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(WaterTextureCoordinates);

		UMaterialExpressionTextureSample* WaterTextureSample = NewObject<UMaterialExpressionTextureSample>(Material);
		WaterTextureSample->Material = Material;
		WaterTextureSample->Texture = WaterTexture;
		WaterTextureSample->Coordinates.Connect(0, WaterTextureCoordinates);
		WaterTextureSample->MaterialExpressionEditorX = -300;
		WaterTextureSample->MaterialExpressionEditorY = -120;
		WaterTextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(WaterTextureSample);

		UMaterialExpressionComponentMask* BrightRippleSource = NewObject<UMaterialExpressionComponentMask>(Material);
		BrightRippleSource->Material = Material;
		BrightRippleSource->Input.Connect(0, WaterTextureSample);
		BrightRippleSource->R = 1;
		BrightRippleSource->G = 0;
		BrightRippleSource->B = 0;
		BrightRippleSource->A = 0;
		BrightRippleSource->MaterialExpressionEditorX = -80;
		BrightRippleSource->MaterialExpressionEditorY = 30;
		Material->GetExpressionCollection().AddExpression(BrightRippleSource);

		UMaterialExpressionSmoothStep* RippleMask = NewObject<UMaterialExpressionSmoothStep>(Material);
		RippleMask->Material = Material;
		RippleMask->Value.Connect(0, BrightRippleSource);
		RippleMask->ConstMin = 0.62f;
		RippleMask->ConstMax = 0.86f;
		RippleMask->MaterialExpressionEditorX = 120;
		RippleMask->MaterialExpressionEditorY = 30;
		Material->GetExpressionCollection().AddExpression(RippleMask);

		UMaterialExpressionConstant3Vector* RippleColor = NewObject<UMaterialExpressionConstant3Vector>(Material);
		RippleColor->Material = Material;
		RippleColor->Constant = FLinearColor(0.94f, 0.98f, 1.0f, 1.0f);
		RippleColor->MaterialExpressionEditorX = 120;
		RippleColor->MaterialExpressionEditorY = -150;
		Material->GetExpressionCollection().AddExpression(RippleColor);

		MaterialEditorOnly->BaseColor.Connect(0, RippleColor);
		MaterialEditorOnly->EmissiveColor.Connect(0, RippleColor);
		MaterialEditorOnly->Opacity.Expression = nullptr;
		MaterialEditorOnly->Opacity.UseConstant = true;
		MaterialEditorOnly->Opacity.Constant = 1.0f;
		MaterialEditorOnly->OpacityMask.Connect(0, RippleMask);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.18f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.35f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		return SaveAsset(Material) ? Material : nullptr;
	}

	float GetPathCenterY(float WorldX)
	{
		const float T = FMath::Clamp((WorldX + LandscapeHalfWorldSize) / LandscapeWorldSize, 0.0f, 1.0f);
		return FMath::Lerp(-1700.0f, 1450.0f, T) + 360.0f * FMath::Sin(T * UE_TWO_PI * 1.45f + 0.45f);
	}

	float GetCreekCenterX(float WorldY)
	{
		const float T = FMath::Clamp((WorldY + LandscapeHalfWorldSize) / LandscapeWorldSize, 0.0f, 1.0f);
		return FMath::Lerp(-3350.0f, 3150.0f, T) + 520.0f * FMath::Sin(T * UE_TWO_PI * 1.25f - 0.35f);
	}

	float EvaluatePathDistance(float WorldX, float WorldY)
	{
		return FMath::Abs(WorldY - GetPathCenterY(WorldX));
	}

	float EvaluateCreekDistance(float WorldX, float WorldY)
	{
		return FMath::Abs(WorldX - GetCreekCenterX(WorldY));
	}

	struct FTerrainMasks
	{
		float PathCore = 0.0f;
		float PathShoulder = 0.0f;
		float CreekWater = 0.0f;
		float CreekBed = 0.0f;
		float CreekBank = 0.0f;
		float BorderRise = 0.0f;
		float Ridge = 0.0f;
		float FineNoise = 0.0f;
	};

	FTerrainMasks EvaluateTerrainMasks(float WorldX, float WorldY)
	{
		FTerrainMasks Masks;

		const float PathDistance = EvaluatePathDistance(WorldX, WorldY);
		Masks.PathCore = 1.0f - SmoothStep(120.0f, 260.0f, PathDistance);
		Masks.PathShoulder = 1.0f - SmoothStep(260.0f, 520.0f, PathDistance);

		const float CreekDistance = EvaluateCreekDistance(WorldX, WorldY);
		Masks.CreekWater = 1.0f - SmoothStep(95.0f, 155.0f, CreekDistance);
		Masks.CreekBed = 1.0f - SmoothStep(170.0f, 290.0f, CreekDistance);
		Masks.CreekBank = SmoothStep(165.0f, 290.0f, CreekDistance) * (1.0f - SmoothStep(360.0f, 620.0f, CreekDistance));

		const float EdgeDistance = FMath::Min(
			FMath::Min(WorldX + LandscapeHalfWorldSize, LandscapeHalfWorldSize - WorldX),
			FMath::Min(WorldY + LandscapeHalfWorldSize, LandscapeHalfWorldSize - WorldY));
		Masks.BorderRise = 1.0f - SmoothStep(500.0f, 1400.0f, EdgeDistance);

		const float RidgeNoise = Noise01(WorldX, WorldY, 0.00135f, 71.0f);
		const float BroadNoise = Noise01(WorldX, WorldY, 0.00058f, 13.0f);
		Masks.Ridge = SmoothStep(0.56f, 0.82f, RidgeNoise) * (1.0f - Masks.PathCore * 0.65f) * (1.0f - Masks.CreekBed * 0.55f);
		Masks.FineNoise = BroadNoise;

		return Masks;
	}

	float EvaluateTerrainHeightCm(float WorldX, float WorldY)
	{
		const FTerrainMasks Masks = EvaluateTerrainMasks(WorldX, WorldY);
		float Height = 18.0f;
		Height += (Noise01(WorldX, WorldY, 0.00095f, 5.0f) - 0.5f) * 56.0f;
		Height += (Noise01(WorldX, WorldY, 0.00215f, 31.0f) - 0.5f) * 24.0f;

		Height += Masks.BorderRise * 62.0f;
		Height += Masks.Ridge * 80.0f;
		Height += Masks.CreekBank * 36.0f;

		const float PathTarget = -8.0f + (Noise01(WorldX, WorldY, 0.0040f, 91.0f) - 0.5f) * 8.0f;
		Height = FMath::Lerp(Height, PathTarget, Masks.PathCore * 0.78f);

		const float CreekTarget = -84.0f + (Noise01(WorldX, WorldY, 0.0030f, 121.0f) - 0.5f) * 10.0f;
		Height = FMath::Lerp(Height, CreekTarget, Masks.CreekBed * 0.86f);

		return FMath::Clamp(Height, -115.0f, 165.0f);
	}

	uint16 EncodeTerrainHeight(float HeightCm)
	{
		const int32 Encoded = 32768 + FMath::RoundToInt(HeightCm * 128.0f / LandscapeScaleZ);
		return static_cast<uint16>(FMath::Clamp(Encoded, 0, 65535));
	}

	void EvaluateLayerWeights(float WorldX, float WorldY, uint8 OutWeights[static_cast<uint8>(ETerrainLayer::Count)])
	{
		const FTerrainMasks Masks = EvaluateTerrainMasks(WorldX, WorldY);
		const float PatchNoise = Noise01(WorldX, WorldY, 0.0027f, 207.0f);
		const float PebbleNoise = Noise01(WorldX, WorldY, 0.0061f, 303.0f);

		float Dirt = 0.16f + Masks.PathShoulder * 0.95f + (PatchNoise > 0.72f ? 0.18f : 0.0f);
		float Grass = 0.70f + (1.0f - Masks.PathShoulder) * 0.14f - Masks.CreekBed * 0.58f - Masks.Ridge * 0.28f;
		float Rock = 0.07f + Masks.Ridge * 0.55f + Masks.CreekBank * 0.35f + (PebbleNoise > 0.76f ? 0.22f : 0.0f);
		float DarkDirt = 0.06f + Masks.CreekBed * 1.20f + Masks.CreekBank * 0.22f + Masks.PathCore * 0.08f;

		Dirt = FMath::Max(Dirt, 0.0f);
		Grass = FMath::Max(Grass, 0.0f);
		Rock = FMath::Max(Rock, 0.0f);
		DarkDirt = FMath::Max(DarkDirt, 0.0f);

		const float Sum = FMath::Max(Dirt + Grass + Rock + DarkDirt, UE_SMALL_NUMBER);
		const int32 DirtWeight = FMath::Clamp(FMath::RoundToInt(255.0f * Dirt / Sum), 0, 255);
		const int32 GrassWeight = FMath::Clamp(FMath::RoundToInt(255.0f * Grass / Sum), 0, 255);
		const int32 RockWeight = FMath::Clamp(FMath::RoundToInt(255.0f * Rock / Sum), 0, 255);
		const int32 Remaining = FMath::Clamp(255 - DirtWeight - GrassWeight - RockWeight, 0, 255);

		OutWeights[static_cast<uint8>(ETerrainLayer::Dirt)] = static_cast<uint8>(DirtWeight);
		OutWeights[static_cast<uint8>(ETerrainLayer::Grass)] = static_cast<uint8>(GrassWeight);
		OutWeights[static_cast<uint8>(ETerrainLayer::Rock)] = static_cast<uint8>(RockWeight);
		OutWeights[static_cast<uint8>(ETerrainLayer::DarkDirt)] = static_cast<uint8>(Remaining);
	}

	void BuildLandscapeData(TArray<uint16>& OutHeightData, TArray<TArray<uint8>>& OutWeightData)
	{
		OutHeightData.SetNumUninitialized(LandscapeVerts * LandscapeVerts);
		OutWeightData.SetNum(static_cast<uint8>(ETerrainLayer::Count));
		for (TArray<uint8>& LayerData : OutWeightData)
		{
			LayerData.SetNumUninitialized(LandscapeVerts * LandscapeVerts);
		}

		for (int32 Y = 0; Y < LandscapeVerts; ++Y)
		{
			for (int32 X = 0; X < LandscapeVerts; ++X)
			{
				const int32 Index = Y * LandscapeVerts + X;
				const float WorldX = static_cast<float>(X) * LandscapeScaleXY - LandscapeHalfWorldSize;
				const float WorldY = static_cast<float>(Y) * LandscapeScaleXY - LandscapeHalfWorldSize;

				OutHeightData[Index] = EncodeTerrainHeight(EvaluateTerrainHeightCm(WorldX, WorldY));

				uint8 Weights[static_cast<uint8>(ETerrainLayer::Count)] = {};
				EvaluateLayerWeights(WorldX, WorldY, Weights);
				for (uint8 LayerIndex = 0; LayerIndex < static_cast<uint8>(ETerrainLayer::Count); ++LayerIndex)
				{
					OutWeightData[LayerIndex][Index] = Weights[LayerIndex];
				}
			}
		}
	}

	FVertexInstanceID AddWaterVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector2f& UV,
		const FLinearColor& Color)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();

		const FVertexID VertexId = MeshDescription.CreateVertex();
		VertexPositions[VertexId] = Position;

		const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
		VertexInstanceNormals[VertexInstanceId] = FVector3f::UpVector;
		VertexInstanceTangents[VertexInstanceId] = FVector3f::XAxisVector;
		VertexInstanceBinormalSigns[VertexInstanceId] = 1.0f;
		VertexInstanceUVs.Set(VertexInstanceId, 0, UV);
		VertexInstanceColors[VertexInstanceId] = FVector4f(Color);
		return VertexInstanceId;
	}

	FVector GetCreekSplinePointAt(float T)
	{
		const float ClampedT = FMath::Clamp(T, 0.0f, 1.0f);
		const float WorldY = FMath::Lerp(-LandscapeHalfWorldSize + CreekSplineMargin, LandscapeHalfWorldSize - CreekSplineMargin, ClampedT);
		const float WorldX = GetCreekCenterX(WorldY);
		const float WorldZ = EvaluateTerrainHeightCm(WorldX, WorldY) + CreekWaterZOffset;
		return FVector(WorldX, WorldY, WorldZ);
	}

	float GetCreekWaterHalfWidthAt(float T, const FVector& Center)
	{
		const float SineVariation = 22.0f * FMath::Sin(T * UE_TWO_PI * 2.7f + 0.4f);
		const float NoiseVariation = (Noise01(Center.X, Center.Y, 0.0011f, 511.0f) - 0.5f) * 22.0f;
		return FMath::Clamp(CreekWaterHalfWidth + SineVariation + NoiseVariation, 148.0f, 212.0f);
	}

	void BuildCreekWaterRibbonMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		const FLinearColor VertexColor(0.18f, 0.56f, 0.62f, 0.46f);

		TArray<FVector> LeftEdgePoints;
		TArray<FVector> RightEdgePoints;
		LeftEdgePoints.Reserve(CreekWaterRibbonSampleCount + 1);
		RightEdgePoints.Reserve(CreekWaterRibbonSampleCount + 1);

		for (int32 PointIndex = 0; PointIndex <= CreekWaterRibbonSampleCount; ++PointIndex)
		{
			const float T = static_cast<float>(PointIndex) / static_cast<float>(CreekWaterRibbonSampleCount);
			const FVector Center = GetCreekSplinePointAt(T);
			const FVector Previous = GetCreekSplinePointAt(T - (1.0f / CreekWaterRibbonSampleCount));
			const FVector Next = GetCreekSplinePointAt(T + (1.0f / CreekWaterRibbonSampleCount));

			FVector Tangent = Next - Previous;
			Tangent.Z = 0.0f;
			if (!Tangent.Normalize())
			{
				Tangent = FVector::ForwardVector;
			}

			const FVector RightVector(Tangent.Y, -Tangent.X, 0.0f);
			const float HalfWidth = GetCreekWaterHalfWidthAt(T, Center);
			LeftEdgePoints.Add(Center - RightVector * HalfWidth);
			RightEdgePoints.Add(Center + RightVector * HalfWidth);
		}

		for (int32 SegmentIndex = 0; SegmentIndex < CreekWaterRibbonSampleCount; ++SegmentIndex)
		{
			const float T0 = static_cast<float>(SegmentIndex) / static_cast<float>(CreekWaterRibbonSampleCount);
			const float T1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(CreekWaterRibbonSampleCount);

			TArray<FVertexInstanceID> VertexInstances;
			VertexInstances.Add(AddWaterVertex(MeshDescription, Attributes, FVector3f(LeftEdgePoints[SegmentIndex]), FVector2f(0.0f, T0), VertexColor));
			VertexInstances.Add(AddWaterVertex(MeshDescription, Attributes, FVector3f(RightEdgePoints[SegmentIndex]), FVector2f(1.0f, T0), VertexColor));
			VertexInstances.Add(AddWaterVertex(MeshDescription, Attributes, FVector3f(RightEdgePoints[SegmentIndex + 1]), FVector2f(1.0f, T1), VertexColor));
			VertexInstances.Add(AddWaterVertex(MeshDescription, Attributes, FVector3f(LeftEdgePoints[SegmentIndex + 1]), FVector2f(0.0f, T1), VertexColor));
			MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
		}
	}

	UStaticMesh* EnsureCreekWaterRibbonMesh(UMaterialInterface* WaterMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, CreekWaterMeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *CreekWaterMeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*CreekWaterMeshAssetName,
				RF_Public | RF_Standalone | RF_Transactional);

			if (!StaticMesh)
			{
				UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();
		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(WaterMaterial));

		FMeshDescription MeshDescription;
		BuildCreekWaterRibbonMeshDescription(MeshDescription);

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bFastBuild = true;
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams);

		if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
		{
			BodySetup->Modify();
			BodySetup->AggGeom.EmptyElements();
			BodySetup->bHasCookedCollisionData = false;
			BodySetup->bNeverNeedsCookedCollisionData = true;
			BodySetup->InvalidatePhysicsData();
		}

		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	ALandscape* SpawnLandscape(
		UWorld* World,
		UMaterialInterface* LandscapeMaterial,
		const TMap<ETerrainLayer, ULandscapeLayerInfoObject*>& LayerInfos)
	{
		if (!World || !LandscapeMaterial)
		{
			return nullptr;
		}

		TArray<uint16> HeightData;
		TArray<TArray<uint8>> WeightData;
		BuildLandscapeData(HeightData, WeightData);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ALandscape::StaticClass(), TEXT("TS_ProceduralTerrain_PathCreek"));
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ALandscape* Landscape = World->SpawnActor<ALandscape>(
			FVector(-LandscapeHalfWorldSize, -LandscapeHalfWorldSize, 0.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!Landscape)
		{
			UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to spawn procedural terrain test Landscape."));
			return nullptr;
		}

		Landscape->SetActorLabel(TEXT("TS_ProceduralTerrain_PathCreek"));
		Landscape->LandscapeMaterial = LandscapeMaterial;
		Landscape->SetActorRelativeScale3D(FVector(LandscapeScaleXY, LandscapeScaleXY, LandscapeScaleZ));
		Landscape->StaticLightingLOD = 0;

		TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
		HeightDataPerLayers.Add(FGuid(), MoveTemp(HeightData));

		TArray<FLandscapeImportLayerInfo> ImportLayerInfos;
		for (const FTerrainLayerDefinition& Definition : GetLayerDefinitions())
		{
			ULandscapeLayerInfoObject* const* LayerInfoPtr = LayerInfos.Find(Definition.Layer);
			if (!LayerInfoPtr || !*LayerInfoPtr)
			{
				return nullptr;
			}

			FLandscapeImportLayerInfo ImportLayerInfo(Definition.LayerName);
			ImportLayerInfo.LayerInfo = *LayerInfoPtr;
			ImportLayerInfo.LayerData = MoveTemp(WeightData[static_cast<uint8>(Definition.Layer)]);
			ImportLayerInfos.Add(MoveTemp(ImportLayerInfo));
		}

		TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
		MaterialLayerDataPerLayers.Add(FGuid(), MoveTemp(ImportLayerInfos));

		Landscape->Import(
			FGuid::NewGuid(),
			0,
			0,
			LandscapeQuads,
			LandscapeQuads,
			NumSubsections,
			SubsectionSizeQuads,
			HeightDataPerLayers,
			TEXT(""),
			MaterialLayerDataPerLayers,
			ELandscapeImportAlphamapType::Additive,
			TArrayView<const FLandscapeLayer>());

		if (ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo())
		{
			LandscapeInfo->UpdateLayerInfoMap(Landscape);
		}

		Landscape->UpdateAllComponentMaterialInstances(true);
		Landscape->MarkPackageDirty();
		return Landscape;
	}

	FVector GetCreekSplinePoint(int32 PointIndex)
	{
		const float T = static_cast<float>(PointIndex) / static_cast<float>(CreekSplinePointCount - 1);
		const float WorldY = FMath::Lerp(-LandscapeHalfWorldSize + CreekSplineMargin, LandscapeHalfWorldSize - CreekSplineMargin, T);
		const float WorldX = GetCreekCenterX(WorldY);
		const float WorldZ = EvaluateTerrainHeightCm(WorldX, WorldY) + CreekWaterZOffset;
		return FVector(WorldX, WorldY, WorldZ);
	}

	bool SpawnCreekWaterSpline(UWorld* World, UStaticMesh* WaterRibbonMesh, UMaterialInterface* WaterMaterial, UMaterialInterface* RippleMaterial)
	{
		if (!World || !WaterRibbonMesh || !WaterMaterial || !RippleMaterial)
		{
			return false;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, AActor::StaticClass(), TEXT("TS_ProceduralCreekWaterSpline"));
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* WaterActor = World->SpawnActor<AActor>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!WaterActor)
		{
			return false;
		}

		WaterActor->SetActorLabel(TEXT("TS_ProceduralCreekWaterSpline"));

		USceneComponent* RootComponent = NewObject<USceneComponent>(WaterActor, TEXT("Root"));
		if (!RootComponent)
		{
			return false;
		}

		RootComponent->SetMobility(EComponentMobility::Static);
		WaterActor->SetRootComponent(RootComponent);
		WaterActor->AddInstanceComponent(RootComponent);
		RootComponent->RegisterComponent();

		USplineComponent* CreekSpline = NewObject<USplineComponent>(WaterActor, TEXT("CreekSpline"));
		if (!CreekSpline)
		{
			return false;
		}

		CreekSpline->SetMobility(EComponentMobility::Static);
		CreekSpline->SetupAttachment(RootComponent);
		WaterActor->AddInstanceComponent(CreekSpline);
		CreekSpline->RegisterComponent();
		CreekSpline->ClearSplinePoints(false);

		for (int32 PointIndex = 0; PointIndex < CreekSplinePointCount; ++PointIndex)
		{
			CreekSpline->AddSplinePoint(GetCreekSplinePoint(PointIndex), ESplineCoordinateSpace::Local, false);
			CreekSpline->SetSplinePointType(PointIndex, ESplinePointType::Curve, false);
		}

		CreekSpline->UpdateSpline();

		UStaticMeshComponent* WaterMeshComponent = NewObject<UStaticMeshComponent>(WaterActor, TEXT("CreekWaterRibbon"));
		if (!WaterMeshComponent)
		{
			return false;
		}

		WaterMeshComponent->SetMobility(EComponentMobility::Static);
		WaterMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WaterMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		WaterMeshComponent->SetGenerateOverlapEvents(false);
		WaterMeshComponent->SetStaticMesh(WaterRibbonMesh);
		WaterMeshComponent->SetMaterial(0, WaterMaterial);
		WaterMeshComponent->SetCastShadow(false);
		WaterMeshComponent->SetVisibility(true, false);
		WaterMeshComponent->SetHiddenInGame(false);
		WaterMeshComponent->SetupAttachment(RootComponent);
		WaterActor->AddInstanceComponent(WaterMeshComponent);
		WaterMeshComponent->RegisterComponent();

		UStaticMeshComponent* RippleMeshComponent = NewObject<UStaticMeshComponent>(WaterActor, TEXT("CreekWaterWhiteRipples"));
		if (!RippleMeshComponent)
		{
			return false;
		}

		RippleMeshComponent->SetMobility(EComponentMobility::Static);
		RippleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RippleMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		RippleMeshComponent->SetGenerateOverlapEvents(false);
		RippleMeshComponent->SetStaticMesh(WaterRibbonMesh);
		RippleMeshComponent->SetMaterial(0, RippleMaterial);
		RippleMeshComponent->SetCastShadow(false);
		RippleMeshComponent->SetVisibility(true, false);
		RippleMeshComponent->SetHiddenInGame(false);
		RippleMeshComponent->SetupAttachment(RootComponent);
		RippleMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, CreekWaterRippleOverlayZOffset));
		WaterActor->AddInstanceComponent(RippleMeshComponent);
		RippleMeshComponent->RegisterComponent();

		WaterActor->MarkPackageDirty();
		return true;
	}

	bool SpawnProceduralLighting(UWorld* World)
	{
		if (!World)
		{
			return false;
		}

		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->Modify();
			WorldSettings->bForceNoPrecomputedLighting = true;
			WorldSettings->MarkPackageDirty();
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ADirectionalLight::StaticClass(), TEXT("TS_ProceduralTerrain_Sun"));
		ADirectionalLight* SunLight = World->SpawnActor<ADirectionalLight>(
			FVector(0.0f, -1200.0f, 2600.0f),
			FRotator(-48.0f, -34.0f, 0.0f),
			SpawnParameters);
		if (!SunLight)
		{
			return false;
		}

		SunLight->SetActorLabel(TEXT("TS_ProceduralTerrain_Sun"));
		SunLight->SetMobility(EComponentMobility::Movable);
		if (ULightComponent* LightComponent = SunLight->GetLightComponent())
		{
			LightComponent->SetMobility(EComponentMobility::Movable);
			LightComponent->SetIntensity(3.6f);
			LightComponent->SetLightColor(FLinearColor(1.0f, 0.93f, 0.82f), false);
		}
		SunLight->MarkPackageDirty();

		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ASkyLight::StaticClass(), TEXT("TS_ProceduralTerrain_SkyLight"));
		ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(
			FVector(0.0f, 0.0f, 1600.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!SkyLight)
		{
			return false;
		}

		SkyLight->SetActorLabel(TEXT("TS_ProceduralTerrain_SkyLight"));
		if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
		{
			SkyLightComponent->SetMobility(EComponentMobility::Movable);
			SkyLightComponent->SetIntensity(0.45f);
			SkyLightComponent->SetLightColor(FLinearColor(0.76f, 0.84f, 1.0f));
			SkyLightComponent->SetRealTimeCapture(false);
		}
		SkyLight->MarkPackageDirty();

		return true;
	}

	bool SpawnPlayerStart(UWorld* World)
	{
		if (!World)
		{
			return false;
		}

		const float GroundZ = EvaluateTerrainHeightCm(PlayerStartX, PlayerStartY);
		const FVector StartLocation(PlayerStartX, PlayerStartY, GroundZ + PlayerStartGroundClearance);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, APlayerStart::StaticClass(), TEXT("TS_ProceduralPlayerStart"));

		APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(
			StartLocation,
			FRotator(0.0f, 25.0f, 0.0f),
			SpawnParameters);
		if (!PlayerStart)
		{
			return false;
		}

		PlayerStart->SetActorLabel(TEXT("TS_ProceduralPlayerStart"));
		PlayerStart->MarkPackageDirty();
		return true;
	}

	bool EnsureProceduralTerrainTestLevel()
	{
		TMap<ETerrainLayer, UTexture2D*> Textures;
		TMap<ETerrainLayer, ULandscapeLayerInfoObject*> LayerInfos;

		for (const FTerrainLayerDefinition& Definition : GetLayerDefinitions())
		{
			UTexture2D* Texture = EnsureTerrainTextureAsset(Definition);
			ULandscapeLayerInfoObject* LayerInfo = EnsureLayerInfoAsset(Definition);
			if (!Texture || !LayerInfo)
			{
				return false;
			}

			Textures.Add(Definition.Layer, Texture);
			LayerInfos.Add(Definition.Layer, LayerInfo);
		}

		UMaterial* LandscapeMaterial = EnsureLandscapeMaterial(Textures);
		UTexture2D* WaterTexture = EnsureTextureAssetFromSourceArt(CreekWaterTextureAssetName, CreekWaterSourceArtFileName);
		UMaterial* WaterMaterial = EnsureCreekWaterMaterial(WaterTexture);
		UMaterial* WaterRippleMaterial = EnsureCreekWaterRippleMaterial(WaterTexture);
		UStaticMesh* WaterRibbonMesh = EnsureCreekWaterRibbonMesh(WaterMaterial);
		if (!LandscapeMaterial || !WaterMaterial || !WaterRippleMaterial || !WaterRibbonMesh)
		{
			return false;
		}

		UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
		if (!World)
		{
			UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to create procedural terrain test map."));
			return false;
		}

		World->PersistentLevel->Modify();
		ALandscape* Landscape = SpawnLandscape(World, LandscapeMaterial, LayerInfos);
		if (!Landscape || !SpawnCreekWaterSpline(World, WaterRibbonMesh, WaterMaterial, WaterRippleMaterial) || !SpawnProceduralLighting(World) || !SpawnPlayerStart(World))
		{
			return false;
		}

		World->MarkPackageDirty();
		const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, MapPackagePath);
		if (!bSaved)
		{
			UE_LOG(LogTunaSweeperProceduralTerrainTest, Error, TEXT("Failed to save %s."), *MapPackagePath);
			return false;
		}

		UE_LOG(LogTunaSweeperProceduralTerrainTest, Display, TEXT("Procedural terrain test map saved: %s"), *MapPackagePath);
		return true;
	}
}
