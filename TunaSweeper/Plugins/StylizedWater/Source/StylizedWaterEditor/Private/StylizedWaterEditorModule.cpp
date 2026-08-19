#include "StylizedWaterBodyActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Selection.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Framework/Commands/UIAction.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSingleLayerWaterMaterialOutput.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "UObject/MetaData.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "StylizedWaterEditor"

DEFINE_LOG_CATEGORY_STATIC(LogStylizedWaterEditor, Log, All);

namespace StylizedWaterEditor
{
	const FString InternalAssetPath = TEXT("/StylizedWater/Generated/Internal");
	const FString DepthGradientTextureName = TEXT("T_WaterDepthGradient");
	const FString MaterialName = TEXT("M_StylizedWaterSurface");
	const FString MaterialInstanceName = TEXT("MI_StylizedWater_CalmAnime");
	const FString BlueprintName = TEXT("BP_StylizedWaterBody_Internal");
	const TCHAR* AssetVersionKey = TEXT("StylizedWaterAssetVersion");
	const TCHAR* AssetVersion = TEXT("3");

	struct FGeneratedAssets
	{
		TObjectPtr<UTexture2D> DepthGradientTexture;
		TObjectPtr<UMaterial> Material;
		TObjectPtr<UMaterialInstanceConstant> MaterialInstance;
		TObjectPtr<UBlueprint> Blueprint;

		bool IsComplete() const
		{
			return DepthGradientTexture && Material && MaterialInstance && Blueprint && Blueprint->GeneratedClass;
		}
	};

	FString ObjectPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset || !Asset->GetOutermost())
		{
			return false;
		}

		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset->GetOutermost()->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, SaveArgs);
	}

	bool HasCurrentAssetVersion(const UObject* Asset)
	{
		if (!Asset || !Asset->GetOutermost())
		{
			return false;
		}

		FMetaData& MetaData = Asset->GetOutermost()->GetMetaData();
		return MetaData.GetValue(Asset, AssetVersionKey) == AssetVersion;
	}

	void StampAssetVersion(UObject* Asset)
	{
		if (Asset && Asset->GetOutermost())
		{
			Asset->GetOutermost()->GetMetaData().SetValue(Asset, AssetVersionKey, AssetVersion);
		}
	}

	template <typename TExpression>
	TExpression* AddExpression(UMaterial* Material, const int32 X, const int32 Y)
	{
		TExpression* Expression = NewObject<TExpression>(Material);
		Expression->Material = Material;
		Expression->MaterialExpressionEditorX = X;
		Expression->MaterialExpressionEditorY = Y;
		Material->GetExpressionCollection().AddExpression(Expression);
		return Expression;
	}

	UMaterialExpressionScalarParameter* AddScalarParameter(
		UMaterial* Material,
		const FName Name,
		const float DefaultValue,
		const int32 X,
		const int32 Y)
	{
		UMaterialExpressionScalarParameter* Parameter = AddExpression<UMaterialExpressionScalarParameter>(Material, X, Y);
		Parameter->ParameterName = Name;
		Parameter->DefaultValue = DefaultValue;
		return Parameter;
	}

	UMaterialExpressionVectorParameter* AddVectorParameter(
		UMaterial* Material,
		const FName Name,
		const FLinearColor& DefaultValue,
		const int32 X,
		const int32 Y)
	{
		UMaterialExpressionVectorParameter* Parameter = AddExpression<UMaterialExpressionVectorParameter>(Material, X, Y);
		Parameter->ParameterName = Name;
		Parameter->DefaultValue = DefaultValue;
		return Parameter;
	}

	void AddCustomInput(UMaterialExpressionCustom* Custom, const FName InputName, UMaterialExpression* Expression, const int32 OutputIndex = 0)
	{
		FCustomInput Input;
		Input.InputName = InputName;
		Input.Input.Connect(OutputIndex, Expression);
		Custom->Inputs.Add(Input);
	}

	UTexture2D* EnsureDepthGradientTexture(const bool bForceRebuild)
	{
		const FString TextureObjectPath = ObjectPath(InternalAssetPath, DepthGradientTextureName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TextureObjectPath);
		if (Texture && !bForceRebuild && HasCurrentAssetVersion(Texture))
		{
			return Texture;
		}

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("StylizedWater"));
		const FString SourceFilename = Plugin.IsValid()
			? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/SourceArt/WaterDepthGradient_1D.png"))
			: FString();
		FImage SourceImage;
		if (SourceFilename.IsEmpty() || !FImageUtils::LoadImage(*SourceFilename, SourceImage))
		{
			UE_LOG(LogStylizedWaterEditor, Error, TEXT("StylizedWater: failed to load depth gradient source %s."), *SourceFilename);
			return nullptr;
		}
		if (SourceImage.SizeX != 256 || SourceImage.SizeY != 1)
		{
			UE_LOG(
				LogStylizedWaterEditor,
				Error,
				TEXT("StylizedWater: depth gradient source must be 256x1, but is %lldx%lld."),
				SourceImage.SizeX,
				SourceImage.SizeY);
			return nullptr;
		}

		if (!Texture)
		{
			const FString PackageName = InternalAssetPath / DepthGradientTextureName;
			UPackage* Package = CreatePackage(*PackageName);
			Texture = Package
				? Cast<UTexture2D>(FImageUtils::CreateTexture(
					ETextureClass::TwoD,
					SourceImage,
					Package,
					DepthGradientTextureName,
					RF_Public | RF_Standalone | RF_Transactional,
					false))
				: nullptr;
			if (Texture)
			{
				FAssetRegistryModule::AssetCreated(Texture);
			}
		}
		else
		{
			Texture->Modify();
			Texture->Source.Init(SourceImage);
		}
		if (!Texture)
		{
			return nullptr;
		}

		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->Filter = TF_Bilinear;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		StampAssetVersion(Texture);
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		return SaveAsset(Texture) ? Texture : nullptr;
	}

	UMaterial* FindOrCreateMaterial()
	{
		const FString MaterialObjectPath = ObjectPath(InternalAssetPath, MaterialName);
		if (UMaterial* Existing = LoadObject<UMaterial>(nullptr, *MaterialObjectPath))
		{
			return Existing;
		}

		const FString PackageName = InternalAssetPath / MaterialName;
		UPackage* Package = CreatePackage(*PackageName);
		UMaterial* Material = Package
			? NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone | RF_Transactional)
			: nullptr;
		if (Material)
		{
			FAssetRegistryModule::AssetCreated(Material);
		}
		return Material;
	}

	UMaterial* EnsureSurfaceMaterial(UTexture2D* DepthGradientTexture, const bool bForceRebuild)
	{
		if (!DepthGradientTexture)
		{
			return nullptr;
		}
		const FString MaterialObjectPath = ObjectPath(InternalAssetPath, MaterialName);
		UMaterial* Material = FindOrCreateMaterial();
		if (!Material)
		{
			UE_LOG(LogStylizedWaterEditor, Error, TEXT("StylizedWater: failed to create the internal surface material."));
			return nullptr;
		}
		if (!bForceRebuild && HasCurrentAssetVersion(Material))
		{
			return Material;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Masked;
		Material->OpacityMaskClipValue = 0.22f;
		Material->TwoSided = true;
		Material->bTangentSpaceNormal = false;
		Material->bUsedWithSplineMeshes = true;
		Material->SetShadingModel(MSM_SingleLayerWater);

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColor = AddExpression<UMaterialExpressionVertexColor>(Material, -1800, -400);
		UMaterialExpressionComponentMask* EncodedDepth = AddExpression<UMaterialExpressionComponentMask>(Material, -1600, -400);
		EncodedDepth->Input.Connect(0, VertexColor);
		EncodedDepth->R = 1;
		EncodedDepth->G = 0;
		EncodedDepth->B = 0;
		EncodedDepth->A = 0;

		UMaterialExpressionWorldPosition* WorldPosition = AddExpression<UMaterialExpressionWorldPosition>(Material, -1800, -250);
		WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(Material, -1800, -100);
		Time->bIgnorePause = true;

		UMaterialExpressionVectorParameter* ShallowColor = AddVectorParameter(Material, TEXT("ShallowColor"), FLinearColor(0.16f, 0.72f, 0.76f, 1.0f), -1800, 80);
		UMaterialExpressionVectorParameter* MidColor = AddVectorParameter(Material, TEXT("MidColor"), FLinearColor(0.035f, 0.43f, 0.63f, 1.0f), -1800, 170);
		UMaterialExpressionVectorParameter* DeepColor = AddVectorParameter(Material, TEXT("DeepColor"), FLinearColor(0.014f, 0.16f, 0.34f, 1.0f), -1800, 260);
		UMaterialExpressionVectorParameter* FoamColor = AddVectorParameter(Material, TEXT("FoamColor"), FLinearColor(0.94f, 0.96f, 0.88f, 1.0f), -1800, 350);
		UMaterialExpressionVectorParameter* FlowDirection = AddVectorParameter(Material, TEXT("FlowDirection"), FLinearColor(1.0f, 0.0f, 0.0f, 0.0f), -1800, 440);

		UMaterialExpressionScalarParameter* DryRange = AddScalarParameter(Material, TEXT("DryRangeCm"), 300.0f, -1440, -250);
		UMaterialExpressionScalarParameter* DepthRange = AddScalarParameter(Material, TEXT("DepthRangeCm"), 1500.0f, -1440, -170);
		UMaterialExpressionScalarParameter* WaterLevelOffset = AddScalarParameter(Material, TEXT("WaterLevelOffsetCm"), 0.0f, -1440, -90);
		UMaterialExpressionScalarParameter* DepthColorRange = AddScalarParameter(Material, TEXT("DepthColorRangeCm"), 700.0f, -1440, -10);
		UMaterialExpressionScalarParameter* MidColorPosition = AddScalarParameter(Material, TEXT("MidColorPosition"), 0.42f, -1440, 70);
		UMaterialExpressionScalarParameter* DepthGradientInfluence = AddScalarParameter(Material, TEXT("DepthGradientInfluence"), 1.0f, -1440, 110);
		UMaterialExpressionScalarParameter* WaterlineSoftness = AddScalarParameter(Material, TEXT("WaterlineSoftnessCm"), 8.0f, -1440, 150);
		UMaterialExpressionScalarParameter* ShoreRunup = AddScalarParameter(Material, TEXT("ShoreRunupCm"), 28.0f, -1440, 230);
		UMaterialExpressionScalarParameter* ShoreWavelength = AddScalarParameter(Material, TEXT("ShoreWavelengthCm"), 280.0f, -1440, 310);
		UMaterialExpressionScalarParameter* ShoreWaveSpeed = AddScalarParameter(Material, TEXT("ShoreWaveSpeed"), 0.18f, -1440, 390);
		UMaterialExpressionScalarParameter* ShoreFoamDepth = AddScalarParameter(Material, TEXT("ShoreFoamDepthCm"), 120.0f, -1440, 470);
		UMaterialExpressionScalarParameter* ShoreFoamWidth = AddScalarParameter(Material, TEXT("ShoreFoamWidth"), 0.12f, -1440, 550);
		UMaterialExpressionScalarParameter* FoamIntensity = AddScalarParameter(Material, TEXT("FoamIntensity"), 0.82f, -1440, 630);
		UMaterialExpressionScalarParameter* FlowSpeed = AddScalarParameter(Material, TEXT("FlowSpeed"), 0.12f, -1080, -250);
		UMaterialExpressionScalarParameter* WaveWorldScale = AddScalarParameter(Material, TEXT("WaveWorldScale"), 0.0035f, -1080, -170);
		UMaterialExpressionScalarParameter* DistortionStrength = AddScalarParameter(Material, TEXT("DistortionStrength"), 0.16f, -1080, -90);
		UMaterialExpressionScalarParameter* GeometryWaveAmplitude = AddScalarParameter(Material, TEXT("GeometryWaveAmplitudeCm"), 3.0f, -1080, -10);
		UMaterialExpressionScalarParameter* EmissiveStrength = AddScalarParameter(Material, TEXT("EmissiveStrength"), 0.035f, -1080, 70);
		UMaterialExpressionScalarParameter* Opacity = AddScalarParameter(Material, TEXT("Opacity"), 0.82f, -1080, 150);
		UMaterialExpressionScalarParameter* Roughness = AddScalarParameter(Material, TEXT("Roughness"), 0.12f, -1080, 230);
		UMaterialExpressionScalarParameter* Specular = AddScalarParameter(Material, TEXT("Specular"), 0.68f, -1080, 310);

		UMaterialExpressionCustom* DepthCoordinates = AddExpression<UMaterialExpressionCustom>(Material, -1040, -440);
		DepthCoordinates->Description = TEXT("Baked signed depth to 1D palette coordinate");
		DepthCoordinates->OutputType = CMOT_Float2;
		DepthCoordinates->Code =
			TEXT("float signedDepth = lerp(-max(DryRangeCm, 1.0f), max(DepthRangeCm, 1.0f), saturate(EncodedDepth));\n")
			TEXT("float depth01 = saturate((signedDepth + WaterLevelOffsetCm) / max(DepthColorRangeCm, 10.0f));\n")
			TEXT("return float2(depth01, 0.5f);");
		AddCustomInput(DepthCoordinates, TEXT("EncodedDepth"), EncodedDepth);
		AddCustomInput(DepthCoordinates, TEXT("DryRangeCm"), DryRange);
		AddCustomInput(DepthCoordinates, TEXT("DepthRangeCm"), DepthRange);
		AddCustomInput(DepthCoordinates, TEXT("WaterLevelOffsetCm"), WaterLevelOffset);
		AddCustomInput(DepthCoordinates, TEXT("DepthColorRangeCm"), DepthColorRange);

		UMaterialExpressionTextureSampleParameter2D* DepthGradientSample = AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, -820, -440);
		DepthGradientSample->ParameterName = TEXT("DepthGradientTexture");
		DepthGradientSample->Texture = DepthGradientTexture;
		DepthGradientSample->SamplerType = SAMPLERTYPE_Color;
		DepthGradientSample->Coordinates.Connect(0, DepthCoordinates);

		UMaterialExpressionCustom* Style = AddExpression<UMaterialExpressionCustom>(Material, -520, -100);
		Style->Description = TEXT("Anime depth color, visual waterline, shore runup, and broken foam");
		Style->OutputType = CMOT_Float4;
		Style->Code =
			TEXT("float signedDepth = lerp(-max(DryRangeCm, 1.0f), max(DepthRangeCm, 1.0f), saturate(EncodedDepth));\n")
			TEXT("float2 flow = FlowDirection.xy;\n")
			TEXT("flow = dot(flow, flow) > 0.0001f ? normalize(flow) : float2(1.0f, 0.0f);\n")
			TEXT("float2 across = float2(-flow.y, flow.x);\n")
			TEXT("float alongCoast = dot(WorldPos.xy, across);\n")
			TEXT("float coastWarp = sin(alongCoast * WaveWorldScale * 0.73f) + 0.5f * sin(alongCoast * WaveWorldScale * 1.91f + 1.7f);\n")
			TEXT("float shorePhase = signedDepth / max(ShoreWavelengthCm, 20.0f) * 6.2831853f - TimeValue * ShoreWaveSpeed * 6.2831853f + coastWarp * 0.35f;\n")
			TEXT("float wave01 = 0.5f + 0.5f * sin(shorePhase);\n")
			TEXT("float runup = ShoreRunupCm * (wave01 - 0.35f);\n")
			TEXT("float apparentDepth = signedDepth + WaterLevelOffsetCm + runup;\n")
			TEXT("float softness = max(WaterlineSoftnessCm, 0.1f);\n")
			TEXT("float waterMask = smoothstep(-softness, softness, apparentDepth);\n")
			TEXT("float depth01 = saturate(apparentDepth / max(DepthColorRangeCm, 10.0f));\n")
			TEXT("float middle = clamp(MidColorPosition, 0.05f, 0.95f);\n")
			TEXT("float lower = smoothstep(0.0f, 1.0f, saturate(depth01 / middle));\n")
			TEXT("float upper = smoothstep(0.0f, 1.0f, saturate((depth01 - middle) / max(1.0f - middle, 0.05f)));\n")
			TEXT("float3 legacyColor = lerp(ShallowColorValue.rgb, MidColorValue.rgb, lower);\n")
			TEXT("legacyColor = lerp(legacyColor, DeepColorValue.rgb, upper);\n")
			TEXT("float3 baseColor = lerp(legacyColor, DepthGradientColorValue.rgb, saturate(DepthGradientInfluence));\n")
			TEXT("float broadFlow = 0.5f + 0.5f * sin(dot(WorldPos.xy, flow) * WaveWorldScale - TimeValue * FlowSpeed + sin(dot(WorldPos.xy, across) * WaveWorldScale * 0.71f) * 0.6f);\n")
			TEXT("baseColor *= lerp(0.978f, 1.024f, broadFlow);\n")
			TEXT("float foamWidth = clamp(ShoreFoamWidth, 0.01f, 0.49f);\n")
			TEXT("float crest = saturate(smoothstep(1.0f - foamWidth, 1.0f - foamWidth * 0.45f, wave01) - smoothstep(1.0f - foamWidth * 0.28f, 1.0f, wave01));\n")
			TEXT("float shallowMask = 1.0f - smoothstep(0.0f, max(ShoreFoamDepthCm, 10.0f), max(apparentDepth, 0.0f));\n")
			TEXT("float breakupWave = 0.5f + 0.5f * sin(alongCoast * WaveWorldScale * 4.1f + sin(alongCoast * WaveWorldScale * 1.37f + TimeValue * 0.11f) * 2.0f);\n")
			TEXT("float breakup = smoothstep(0.28f, 0.72f, breakupWave);\n")
			TEXT("float contact = 1.0f - smoothstep(0.0f, softness * 3.0f, abs(apparentDepth));\n")
			TEXT("float foam = saturate(max(crest * shallowMask, contact * 0.46f) * breakup * FoamIntensity) * waterMask;\n")
			TEXT("return float4(lerp(baseColor, FoamColorValue.rgb, foam), waterMask);");

		AddCustomInput(Style, TEXT("EncodedDepth"), EncodedDepth);
		AddCustomInput(Style, TEXT("WorldPos"), WorldPosition);
		AddCustomInput(Style, TEXT("TimeValue"), Time);
		AddCustomInput(Style, TEXT("ShallowColorValue"), ShallowColor);
		AddCustomInput(Style, TEXT("MidColorValue"), MidColor);
		AddCustomInput(Style, TEXT("DeepColorValue"), DeepColor);
		AddCustomInput(Style, TEXT("DepthGradientColorValue"), DepthGradientSample);
		AddCustomInput(Style, TEXT("FoamColorValue"), FoamColor);
		AddCustomInput(Style, TEXT("DryRangeCm"), DryRange);
		AddCustomInput(Style, TEXT("DepthRangeCm"), DepthRange);
		AddCustomInput(Style, TEXT("WaterLevelOffsetCm"), WaterLevelOffset);
		AddCustomInput(Style, TEXT("DepthColorRangeCm"), DepthColorRange);
		AddCustomInput(Style, TEXT("MidColorPosition"), MidColorPosition);
		AddCustomInput(Style, TEXT("DepthGradientInfluence"), DepthGradientInfluence);
		AddCustomInput(Style, TEXT("WaterlineSoftnessCm"), WaterlineSoftness);
		AddCustomInput(Style, TEXT("ShoreRunupCm"), ShoreRunup);
		AddCustomInput(Style, TEXT("ShoreWavelengthCm"), ShoreWavelength);
		AddCustomInput(Style, TEXT("ShoreWaveSpeed"), ShoreWaveSpeed);
		AddCustomInput(Style, TEXT("ShoreFoamDepthCm"), ShoreFoamDepth);
		AddCustomInput(Style, TEXT("ShoreFoamWidth"), ShoreFoamWidth);
		AddCustomInput(Style, TEXT("FoamIntensity"), FoamIntensity);
		AddCustomInput(Style, TEXT("FlowDirection"), FlowDirection);
		AddCustomInput(Style, TEXT("FlowSpeed"), FlowSpeed);
		AddCustomInput(Style, TEXT("WaveWorldScale"), WaveWorldScale);

		UMaterialExpressionComponentMask* StyleColor = AddExpression<UMaterialExpressionComponentMask>(Material, -330, -160);
		StyleColor->Input.Connect(0, Style);
		StyleColor->R = 1;
		StyleColor->G = 1;
		StyleColor->B = 1;
		StyleColor->A = 0;

		UMaterialExpressionComponentMask* StyleMask = AddExpression<UMaterialExpressionComponentMask>(Material, -330, 0);
		StyleMask->Input.Connect(0, Style);
		StyleMask->R = 0;
		StyleMask->G = 0;
		StyleMask->B = 0;
		StyleMask->A = 1;

		UMaterialExpressionMultiply* Emissive = AddExpression<UMaterialExpressionMultiply>(Material, -80, -80);
		Emissive->A.Connect(0, StyleColor);
		Emissive->B.Connect(0, EmissiveStrength);

		UMaterialExpressionCustom* SurfaceNormal = AddExpression<UMaterialExpressionCustom>(Material, -330, 180);
		SurfaceNormal->Description = TEXT("Low-frequency world-space flow normal");
		SurfaceNormal->OutputType = CMOT_Float3;
		SurfaceNormal->Code =
			TEXT("float2 flow = FlowDirection.xy;\n")
			TEXT("flow = dot(flow, flow) > 0.0001f ? normalize(flow) : float2(1.0f, 0.0f);\n")
			TEXT("float2 across = float2(-flow.y, flow.x);\n")
			TEXT("float2 p = WorldPos.xy * max(WaveWorldScale, 0.00001f);\n")
			TEXT("float phaseA = dot(p, flow) * 6.2831853f - TimeValue * FlowSpeed * 2.4f;\n")
			TEXT("float phaseB = dot(p, across) * 9.1f + TimeValue * FlowSpeed * 1.35f + sin(phaseA * 0.37f);\n")
			TEXT("float2 gradient = flow * cos(phaseA) * 0.68f + across * cos(phaseB) * 0.32f;\n")
			TEXT("gradient *= max(DistortionStrength, 0.0f);\n")
			TEXT("return normalize(float3(-gradient.x, -gradient.y, 1.0f));");
		AddCustomInput(SurfaceNormal, TEXT("WorldPos"), WorldPosition);
		AddCustomInput(SurfaceNormal, TEXT("TimeValue"), Time);
		AddCustomInput(SurfaceNormal, TEXT("FlowDirection"), FlowDirection);
		AddCustomInput(SurfaceNormal, TEXT("FlowSpeed"), FlowSpeed);
		AddCustomInput(SurfaceNormal, TEXT("WaveWorldScale"), WaveWorldScale);
		AddCustomInput(SurfaceNormal, TEXT("DistortionStrength"), DistortionStrength);

		UMaterialExpressionCustom* WorldPositionOffset = AddExpression<UMaterialExpressionCustom>(Material, -330, 380);
		WorldPositionOffset->Description = TEXT("Small calm-surface and shore crest displacement");
		WorldPositionOffset->OutputType = CMOT_Float3;
		WorldPositionOffset->Code =
			TEXT("float signedDepth = lerp(-max(DryRangeCm, 1.0f), max(DepthRangeCm, 1.0f), saturate(EncodedDepth));\n")
			TEXT("float2 flow = FlowDirection.xy;\n")
			TEXT("flow = dot(flow, flow) > 0.0001f ? normalize(flow) : float2(1.0f, 0.0f);\n")
			TEXT("float2 across = float2(-flow.y, flow.x);\n")
			TEXT("float broad = sin(dot(WorldPos.xy, flow) * WaveWorldScale * 2.2f - TimeValue * FlowSpeed * 1.7f);\n")
			TEXT("float shorePhase = signedDepth / max(ShoreWavelengthCm, 20.0f) * 6.2831853f - TimeValue * ShoreWaveSpeed * 6.2831853f;\n")
			TEXT("float shoreMask = 1.0f - smoothstep(0.0f, max(ShoreFoamDepthCm, 10.0f), abs(signedDepth + WaterLevelOffsetCm));\n")
			TEXT("float crest = sin(shorePhase) * shoreMask;\n")
			TEXT("return float3(0.0f, 0.0f, (broad * 0.24f + crest * 0.76f) * max(GeometryWaveAmplitudeCm, 0.0f));");
		AddCustomInput(WorldPositionOffset, TEXT("EncodedDepth"), EncodedDepth);
		AddCustomInput(WorldPositionOffset, TEXT("WorldPos"), WorldPosition);
		AddCustomInput(WorldPositionOffset, TEXT("TimeValue"), Time);
		AddCustomInput(WorldPositionOffset, TEXT("FlowDirection"), FlowDirection);
		AddCustomInput(WorldPositionOffset, TEXT("FlowSpeed"), FlowSpeed);
		AddCustomInput(WorldPositionOffset, TEXT("WaveWorldScale"), WaveWorldScale);
		AddCustomInput(WorldPositionOffset, TEXT("DryRangeCm"), DryRange);
		AddCustomInput(WorldPositionOffset, TEXT("DepthRangeCm"), DepthRange);
		AddCustomInput(WorldPositionOffset, TEXT("WaterLevelOffsetCm"), WaterLevelOffset);
		AddCustomInput(WorldPositionOffset, TEXT("ShoreWavelengthCm"), ShoreWavelength);
		AddCustomInput(WorldPositionOffset, TEXT("ShoreWaveSpeed"), ShoreWaveSpeed);
		AddCustomInput(WorldPositionOffset, TEXT("ShoreFoamDepthCm"), ShoreFoamDepth);
		AddCustomInput(WorldPositionOffset, TEXT("GeometryWaveAmplitudeCm"), GeometryWaveAmplitude);

		UMaterialExpressionVectorParameter* Scattering = AddVectorParameter(Material, TEXT("ScatteringCoefficients"), FLinearColor(0.004f, 0.012f, 0.018f, 1.0f), 40, 180);
		UMaterialExpressionVectorParameter* Absorption = AddVectorParameter(Material, TEXT("AbsorptionCoefficients"), FLinearColor(0.009f, 0.0035f, 0.0015f, 1.0f), 40, 270);
		UMaterialExpressionScalarParameter* PhaseG = AddScalarParameter(Material, TEXT("PhaseG"), 0.18f, 40, 360);
		UMaterialExpressionSingleLayerWaterMaterialOutput* WaterOutput = AddExpression<UMaterialExpressionSingleLayerWaterMaterialOutput>(Material, 340, 280);
		WaterOutput->ScatteringCoefficients.Connect(0, Scattering);
		WaterOutput->AbsorptionCoefficients.Connect(0, Absorption);
		WaterOutput->PhaseG.Connect(0, PhaseG);
		// Single Layer Water composites the scene behind the surface through this input.
		// Feeding the baked depth color here keeps the depth palette visible instead of
		// washing it out with one constant tint during the water composite pass.
		WaterOutput->ColorScaleBehindWater.Connect(0, StyleColor);

		EditorData->BaseColor.Connect(0, StyleColor);
		EditorData->EmissiveColor.Connect(0, Emissive);
		EditorData->Opacity.Connect(0, Opacity);
		EditorData->OpacityMask.Connect(0, StyleMask);
		EditorData->Roughness.Connect(0, Roughness);
		EditorData->Specular.Connect(0, Specular);
		EditorData->Metallic.UseConstant = true;
		EditorData->Metallic.Constant = 0.0f;
		EditorData->Normal.Connect(0, SurfaceNormal);
		EditorData->WorldPositionOffset.Connect(0, WorldPositionOffset);

		StampAssetVersion(Material);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		if (!SaveAsset(Material))
		{
			UE_LOG(LogStylizedWaterEditor, Error, TEXT("StylizedWater: failed to save %s."), *MaterialObjectPath);
			return nullptr;
		}

		return Material;
	}

	UMaterialInstanceConstant* EnsureMaterialInstance(UMaterial* ParentMaterial, const bool bForceRebuild)
	{
		if (!ParentMaterial)
		{
			return nullptr;
		}

		const FString InstanceObjectPath = ObjectPath(InternalAssetPath, MaterialInstanceName);
		UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *InstanceObjectPath);
		if (!Instance)
		{
			const FString PackageName = InternalAssetPath / MaterialInstanceName;
			UPackage* Package = CreatePackage(*PackageName);
			Instance = Package
				? NewObject<UMaterialInstanceConstant>(Package, *MaterialInstanceName, RF_Public | RF_Standalone | RF_Transactional)
				: nullptr;
			if (Instance)
			{
				FAssetRegistryModule::AssetCreated(Instance);
			}
		}
		if (!Instance)
		{
			return nullptr;
		}
		if (!bForceRebuild && HasCurrentAssetVersion(Instance) && Instance->Parent == ParentMaterial)
		{
			return Instance;
		}

		Instance->Modify();
		Instance->SetParentEditorOnly(ParentMaterial);
		Instance->ClearParameterValuesEditorOnly();
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("ShallowColor")), FLinearColor(0.16f, 0.72f, 0.76f, 1.0f));
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("MidColor")), FLinearColor(0.035f, 0.43f, 0.63f, 1.0f));
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("DeepColor")), FLinearColor(0.014f, 0.16f, 0.34f, 1.0f));
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("FoamColor")), FLinearColor(0.94f, 0.96f, 0.88f, 1.0f));
		StampAssetVersion(Instance);
		Instance->PostEditChange();
		Instance->MarkPackageDirty();
		return SaveAsset(Instance) ? Instance : nullptr;
	}

	UBlueprint* EnsureBlueprintTemplate(UMaterialInstanceConstant* MaterialInstance, const bool bForceRebuild)
	{
		if (!MaterialInstance)
		{
			return nullptr;
		}

		const FString BlueprintObjectPath = ObjectPath(InternalAssetPath, BlueprintName);
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintObjectPath);
		if (!Blueprint)
		{
			const FString PackageName = InternalAssetPath / BlueprintName;
			UPackage* Package = CreatePackage(*PackageName);
			Blueprint = Package
				? FKismetEditorUtilities::CreateBlueprint(
					AStylizedWaterBodyActor::StaticClass(),
					Package,
					*BlueprintName,
					BPTYPE_Normal,
					UBlueprint::StaticClass(),
					UBlueprintGeneratedClass::StaticClass())
				: nullptr;
			if (Blueprint)
			{
				FAssetRegistryModule::AssetCreated(Blueprint);
				FKismetEditorUtilities::CompileBlueprint(Blueprint);
			}
		}
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			return nullptr;
		}

		if (bForceRebuild || !HasCurrentAssetVersion(Blueprint))
		{
			Blueprint->Modify();
			if (AStylizedWaterBodyActor* Defaults = Cast<AStylizedWaterBodyActor>(Blueprint->GeneratedClass->GetDefaultObject()))
			{
				Defaults->Modify();
				Defaults->SetTemplateMaterialInstance(MaterialInstance);
				Defaults->ApplyPreset(EStylizedWaterPreset::GentleBeach, false);
			}
			StampAssetVersion(Blueprint);
			Blueprint->MarkPackageDirty();
			if (!SaveAsset(Blueprint))
			{
				return nullptr;
			}
		}

		return Blueprint;
	}

	FGeneratedAssets EnsurePluginAssets(const bool bForceRebuild)
	{
		FGeneratedAssets Assets;
		Assets.DepthGradientTexture = EnsureDepthGradientTexture(bForceRebuild);
		Assets.Material = EnsureSurfaceMaterial(Assets.DepthGradientTexture, bForceRebuild);
		Assets.MaterialInstance = EnsureMaterialInstance(Assets.Material, bForceRebuild);
		Assets.Blueprint = EnsureBlueprintTemplate(Assets.MaterialInstance, bForceRebuild);
		if (Assets.IsComplete())
		{
			UE_LOG(LogStylizedWaterEditor, Display, TEXT("StylizedWater: internal T_, M_, MI_, and BP_ assets are ready."));
		}
		else
		{
			UE_LOG(LogStylizedWaterEditor, Error, TEXT("StylizedWater: internal asset generation failed."));
		}
		return Assets;
	}

	FVector ChooseSpawnLocation()
	{
		if (!GEditor)
		{
			return FVector::ZeroVector;
		}

		if (USelection* Selection = GEditor->GetSelectedActors())
		{
			if (AActor* SelectedActor = Selection->GetTop<AActor>())
			{
				return SelectedActor->GetActorLocation() + FVector(0.0, 0.0, 30.0);
			}
		}

		return FVector::ZeroVector;
	}

	FString PresetActorLabel(const EStylizedWaterPreset Preset)
	{
		switch (Preset)
		{
		case EStylizedWaterPreset::CalmLake:
			return TEXT("SW_CalmLake");
		case EStylizedWaterPreset::GentleBeach:
			return TEXT("SW_GentleBeach");
		case EStylizedWaterPreset::FlowingRiver:
			return TEXT("SW_FlowingRiver");
		default:
			return TEXT("SW_StylizedWater");
		}
	}
}

class FStylizedWaterEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		EditorInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(this, &FStylizedWaterEditorModule::OnEditorInitialized);
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FStylizedWaterEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (EditorInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(EditorInitializedHandle);
			EditorInitializedHandle.Reset();
		}

		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}
	}

private:
	void OnEditorInitialized(double)
	{
		const bool bForceRebuild = FParse::Param(FCommandLine::Get(), TEXT("StylizedWaterRebuildAssets"));
		StylizedWaterEditor::EnsurePluginAssets(bForceRebuild);
		if (FParse::Param(FCommandLine::Get(), TEXT("StylizedWaterAssetGenerationQuit")))
		{
			FGenericPlatformMisc::RequestExit(false);
		}
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
		FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
		if (!MainSection.FindEntry(TEXT("TunaSweeper")))
		{
			FToolMenuEntry& TunaSweeperEntry = MainSection.AddSubMenu(
				TEXT("TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
				FNewToolMenuChoice());
			TunaSweeperEntry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
		}

		UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
			TEXT("LevelEditor.MainMenu.TunaSweeper"),
			NAME_None,
			EMultiBoxType::Menu,
			false);
		FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(TEXT("Rendering"), LOCTEXT("RenderingSection", "Rendering"));
		Section.AddMenuEntry(
			TEXT("AddStylizedWaterCalmLake"),
			LOCTEXT("AddCalmLake", "Stylized Water: Add Calm Lake"),
			LOCTEXT("AddCalmLakeTooltip", "Create a calm stylized lake without exposing the internal Blueprint or material instance."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FStylizedWaterEditorModule::AddWaterBody, EStylizedWaterPreset::CalmLake)));
		Section.AddMenuEntry(
			TEXT("AddStylizedWaterGentleBeach"),
			LOCTEXT("AddGentleBeach", "Stylized Water: Add Gentle Beach"),
			LOCTEXT("AddGentleBeachTooltip", "Create a gentle beach with visual runup and broken foam."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FStylizedWaterEditorModule::AddWaterBody, EStylizedWaterPreset::GentleBeach)));
		Section.AddMenuEntry(
			TEXT("AddStylizedWaterFlowingRiver"),
			LOCTEXT("AddFlowingRiver", "Stylized Water: Add Flowing River"),
			LOCTEXT("AddFlowingRiverTooltip", "Create a long stylized water surface with stronger directional flow."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FStylizedWaterEditorModule::AddWaterBody, EStylizedWaterPreset::FlowingRiver)));
	}

	void AddWaterBody(const EStylizedWaterPreset Preset)
	{
		if (!GEditor)
		{
			return;
		}

		StylizedWaterEditor::FGeneratedAssets Assets = StylizedWaterEditor::EnsurePluginAssets(false);
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!Assets.IsComplete() || !World)
		{
			UE_LOG(LogStylizedWaterEditor, Error, TEXT("StylizedWater: cannot add a water body because the editor world or internal assets are unavailable."));
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("AddStylizedWaterTransaction", "Add Stylized Water Body"));
		World->Modify();
		const FTransform SpawnTransform(FRotator::ZeroRotator, StylizedWaterEditor::ChooseSpawnLocation());
		AStylizedWaterBodyActor* WaterBody = World->SpawnActorDeferred<AStylizedWaterBodyActor>(
			Assets.Blueprint->GeneratedClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!WaterBody)
		{
			return;
		}

		WaterBody->SetFlags(RF_Transactional);
		WaterBody->Modify();
		WaterBody->SetActorLabel(StylizedWaterEditor::PresetActorLabel(Preset));
		WaterBody->SetTemplateMaterialInstance(Assets.MaterialInstance);
		WaterBody->ApplyPreset(Preset, false);
		WaterBody->FinishSpawning(SpawnTransform);
		WaterBody->MarkPackageDirty();

		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(WaterBody, true, true, true);
	}

	FDelegateHandle EditorInitializedHandle;
};

IMPLEMENT_MODULE(FStylizedWaterEditorModule, StylizedWaterEditor)

#undef LOCTEXT_NAMESPACE
