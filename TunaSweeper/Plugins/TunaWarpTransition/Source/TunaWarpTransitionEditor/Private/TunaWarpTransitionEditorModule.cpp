#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "TunaWarpTransitionProfile.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionViewProperty.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/MetaData.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaWarpTransitionEditor, Log, All);

namespace TunaWarpTransitionEditor
{
	const FString InternalAssetPath = TEXT("/TunaWarpTransition/Generated/Internal");
	const FString PublicMaterialPath = TEXT("/TunaWarpTransition/Materials");
	const FString PublicProfilePath = TEXT("/TunaWarpTransition/Profiles");
	const FString WarpMaterialName = TEXT("M_PP_TunaWarpRadial_Internal");
	const FString RimMaterialName = TEXT("M_PP_TunaWarpArrivalRim_Internal");
	const FString WarpInstanceName = TEXT("MI_PP_TunaWarpRadial_Default");
	const FString RimInstanceName = TEXT("MI_PP_TunaWarpArrivalRim_Default");
	const FString DefaultProfileName = TEXT("DA_WarpTransition_Default");
	const TCHAR* AssetVersionKey = TEXT("TunaWarpTransitionAssetVersion");
	const TCHAR* AssetVersion = TEXT("3");

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
		return Asset->GetOutermost()->GetMetaData().GetValue(Asset, AssetVersionKey) == AssetVersion;
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

	void AddCustomInput(
		UMaterialExpressionCustom* Custom,
		const FName InputName,
		UMaterialExpression* Expression,
		const int32 OutputIndex = 0)
	{
		FCustomInput Input;
		Input.InputName = InputName;
		Input.Input.Connect(OutputIndex, Expression);
		Custom->Inputs.Add(Input);
	}

	UMaterial* FindOrCreateMaterial(const FString& AssetPath, const FString& AssetName)
	{
		if (UMaterial* Existing = LoadObject<UMaterial>(nullptr, *ObjectPath(AssetPath, AssetName)))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*(AssetPath / AssetName));
		UMaterial* Material = Package
			? NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional)
			: nullptr;
		if (Material)
		{
			FAssetRegistryModule::AssetCreated(Material);
		}
		return Material;
	}

	UMaterialExpressionCustom* AddWarpUv(
		UMaterial* Material,
		UMaterialExpression* ScreenUv,
		UMaterialExpression* ViewSize,
		UMaterialExpression* Center,
		UMaterialExpression* Radius,
		UMaterialExpression* WarpAmount,
		UMaterialExpression* MinimumRadialScale,
		UMaterialExpression* StreakLength,
		const float SampleFraction,
		const int32 X,
		const int32 Y)
	{
		UMaterialExpressionCustom* WarpUv = AddExpression<UMaterialExpressionCustom>(Material, X, Y);
		WarpUv->Description = FString::Printf(TEXT("Aspect-correct radial warp UV, streak sample %.2f"), SampleFraction);
		WarpUv->OutputType = CMOT_Float2;
		WarpUv->Code = FString::Printf(
			TEXT("float2 size = max(ViewSizeValue.xy, float2(1.0f, 1.0f));\n")
			TEXT("float2 axis = float2(size.x / size.y, 1.0f);\n")
			TEXT("float2 p = (ScreenUvValue.xy - CenterValue.xy) * axis;\n")
			TEXT("float r = length(p);\n")
			TEXT("float radius = max(RadiusValue, 0.0001f);\n")
			TEXT("float amount = saturate(WarpAmountValue);\n")
			TEXT("float k = lerp(1.0f, clamp(MinimumRadialScaleValue, 0.05f, 1.0f), amount);\n")
			TEXT("float sampleRadius;\n")
			TEXT("if (r <= radius)\n")
			TEXT("{\n")
			TEXT("    float x = saturate(r / radius);\n")
			TEXT("    sampleRadius = radius * x / max(k + (1.0f - k) * x, 0.0001f);\n")
			TEXT("}\n")
			TEXT("else\n")
			TEXT("{\n")
			TEXT("    sampleRadius = radius + k * (r - radius);\n")
			TEXT("    sampleRadius = max(radius, sampleRadius - StreakLengthValue * amount * %.8ff);\n")
			TEXT("}\n")
			TEXT("float2 direction = r > 0.00001f ? p / r : float2(0.0f, 0.0f);\n")
			TEXT("float2 warpedUv = CenterValue.xy + direction * sampleRadius / axis;\n")
			TEXT("return clamp(warpedUv, float2(0.0001f, 0.0001f), float2(0.9999f, 0.9999f));"),
			SampleFraction);
		AddCustomInput(WarpUv, TEXT("ScreenUvValue"), ScreenUv);
		AddCustomInput(WarpUv, TEXT("ViewSizeValue"), ViewSize);
		AddCustomInput(WarpUv, TEXT("CenterValue"), Center);
		AddCustomInput(WarpUv, TEXT("RadiusValue"), Radius);
		AddCustomInput(WarpUv, TEXT("WarpAmountValue"), WarpAmount);
		AddCustomInput(WarpUv, TEXT("MinimumRadialScaleValue"), MinimumRadialScale);
		AddCustomInput(WarpUv, TEXT("StreakLengthValue"), StreakLength);
		return WarpUv;
	}

	UMaterial* EnsureWarpMaterial()
	{
		UMaterial* Material = FindOrCreateMaterial(InternalAssetPath, WarpMaterialName);
		if (!Material)
		{
			return nullptr;
		}
		if (HasCurrentAssetVersion(Material))
		{
			return Material;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendableLocation = BL_SceneColorAfterTonemapping;
		Material->BlendablePriority = 100;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_Unlit);

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return nullptr;
		}

		UMaterialExpressionScreenPosition* ScreenUv = AddExpression<UMaterialExpressionScreenPosition>(Material, -1500, -360);
		UMaterialExpressionViewProperty* ViewSize = AddExpression<UMaterialExpressionViewProperty>(Material, -1500, -260);
		ViewSize->Property = MEVP_ViewSize;
		UMaterialExpressionVectorParameter* Center = AddVectorParameter(Material, TEXT("CenterUV"), FLinearColor(0.5f, 0.5f, 0.0f, 0.0f), -1500, -150);
		UMaterialExpressionScalarParameter* Radius = AddScalarParameter(Material, TEXT("Radius"), 0.72f, -1500, -40);
		UMaterialExpressionScalarParameter* WarpAmount = AddScalarParameter(Material, TEXT("WarpAmount"), 0.0f, -1500, 70);
		UMaterialExpressionScalarParameter* MinimumRadialScale = AddScalarParameter(Material, TEXT("MinimumRadialScale"), 0.24f, -1500, 180);
		UMaterialExpressionScalarParameter* StreakLength = AddScalarParameter(Material, TEXT("StreakLength"), 0.0f, -1500, 290);
		UMaterialExpressionVectorParameter* CoverColor = AddVectorParameter(Material, TEXT("CoverColor"), FLinearColor(0.008f, 0.025f, 0.045f, 1.0f), -260, 320);
		UMaterialExpressionComponentMask* CoverColorRgb = AddExpression<UMaterialExpressionComponentMask>(Material, -80, 320);
		CoverColorRgb->Input.Connect(0, CoverColor);
		CoverColorRgb->R = 1;
		CoverColorRgb->G = 1;
		CoverColorRgb->B = 1;
		CoverColorRgb->A = 0;
		UMaterialExpressionScalarParameter* CoverAmount = AddScalarParameter(Material, TEXT("CoverAmount"), 0.0f, -260, 440);

		UMaterialExpressionCustom* WarpUv0 = AddWarpUv(Material, ScreenUv, ViewSize, Center, Radius, WarpAmount, MinimumRadialScale, StreakLength, 0.0f, -1030, -330);
		UMaterialExpressionCustom* WarpUv1 = AddWarpUv(Material, ScreenUv, ViewSize, Center, Radius, WarpAmount, MinimumRadialScale, StreakLength, 0.5f, -1030, 0);
		UMaterialExpressionCustom* WarpUv2 = AddWarpUv(Material, ScreenUv, ViewSize, Center, Radius, WarpAmount, MinimumRadialScale, StreakLength, 1.0f, -1030, 330);

		UMaterialExpressionSceneTexture* Scene0 = AddExpression<UMaterialExpressionSceneTexture>(Material, -660, -330);
		Scene0->SceneTextureId = PPI_PostProcessInput0;
		Scene0->bFiltered = true;
		Scene0->Coordinates.Connect(0, WarpUv0);
		UMaterialExpressionSceneTexture* Scene1 = AddExpression<UMaterialExpressionSceneTexture>(Material, -660, 0);
		Scene1->SceneTextureId = PPI_PostProcessInput0;
		Scene1->bFiltered = true;
		Scene1->Coordinates.Connect(0, WarpUv1);
		UMaterialExpressionSceneTexture* Scene2 = AddExpression<UMaterialExpressionSceneTexture>(Material, -660, 330);
		Scene2->SceneTextureId = PPI_PostProcessInput0;
		Scene2->bFiltered = true;
		Scene2->Coordinates.Connect(0, WarpUv2);
		UMaterialExpressionComponentMask* SceneRgb0 = AddExpression<UMaterialExpressionComponentMask>(Material, -500, -330);
		SceneRgb0->Input.Connect(0, Scene0);
		SceneRgb0->R = 1;
		SceneRgb0->G = 1;
		SceneRgb0->B = 1;
		SceneRgb0->A = 0;
		UMaterialExpressionComponentMask* SceneRgb1 = AddExpression<UMaterialExpressionComponentMask>(Material, -500, 0);
		SceneRgb1->Input.Connect(0, Scene1);
		SceneRgb1->R = 1;
		SceneRgb1->G = 1;
		SceneRgb1->B = 1;
		SceneRgb1->A = 0;
		UMaterialExpressionComponentMask* SceneRgb2 = AddExpression<UMaterialExpressionComponentMask>(Material, -500, 330);
		SceneRgb2->Input.Connect(0, Scene2);
		SceneRgb2->R = 1;
		SceneRgb2->G = 1;
		SceneRgb2->B = 1;
		SceneRgb2->A = 0;

		UMaterialExpressionMultiply* Weighted0 = AddExpression<UMaterialExpressionMultiply>(Material, -280, -280);
		Weighted0->A.Connect(0, SceneRgb0);
		Weighted0->ConstB = 0.56f;
		UMaterialExpressionMultiply* Weighted1 = AddExpression<UMaterialExpressionMultiply>(Material, -280, -80);
		Weighted1->A.Connect(0, SceneRgb1);
		Weighted1->ConstB = 0.29f;
		UMaterialExpressionMultiply* Weighted2 = AddExpression<UMaterialExpressionMultiply>(Material, -280, 120);
		Weighted2->A.Connect(0, SceneRgb2);
		Weighted2->ConstB = 0.15f;
		UMaterialExpressionAdd* Add01 = AddExpression<UMaterialExpressionAdd>(Material, -120, -190);
		Add01->A.Connect(0, Weighted0);
		Add01->B.Connect(0, Weighted1);
		UMaterialExpressionAdd* DistortedColor = AddExpression<UMaterialExpressionAdd>(Material, 100, -100);
		DistortedColor->A.Connect(0, Add01);
		DistortedColor->B.Connect(0, Weighted2);

		UMaterialExpressionLinearInterpolate* CoverBlend = AddExpression<UMaterialExpressionLinearInterpolate>(Material, 370, 0);
		CoverBlend->A.Connect(0, DistortedColor);
		CoverBlend->B.Connect(0, CoverColorRgb);
		CoverBlend->Alpha.Connect(0, CoverAmount);
		EditorData->EmissiveColor.Connect(0, CoverBlend);

		StampAssetVersion(Material);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	UMaterial* EnsureRimMaterial()
	{
		UMaterial* Material = FindOrCreateMaterial(InternalAssetPath, RimMaterialName);
		if (!Material)
		{
			return nullptr;
		}
		if (HasCurrentAssetVersion(Material))
		{
			return Material;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendableLocation = BL_SceneColorBeforeDOF;
		Material->BlendablePriority = 0;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_Unlit);

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return nullptr;
		}

		UMaterialExpressionSceneTexture* SceneColor = AddExpression<UMaterialExpressionSceneTexture>(Material, -1450, -520);
		SceneColor->SceneTextureId = PPI_PostProcessInput0;
		UMaterialExpressionComponentMask* SceneColorRgb = AddExpression<UMaterialExpressionComponentMask>(Material, -1230, -520);
		SceneColorRgb->Input.Connect(0, SceneColor);
		SceneColorRgb->R = 1;
		SceneColorRgb->G = 1;
		SceneColorRgb->B = 1;
		SceneColorRgb->A = 0;
		UMaterialExpressionSceneTexture* WorldNormal = AddExpression<UMaterialExpressionSceneTexture>(Material, -1450, -380);
		WorldNormal->SceneTextureId = PPI_WorldNormal;
		UMaterialExpressionSceneTexture* SceneDepth = AddExpression<UMaterialExpressionSceneTexture>(Material, -1450, -240);
		SceneDepth->SceneTextureId = PPI_SceneDepth;
		UMaterialExpressionWorldPosition* WorldPosition = AddExpression<UMaterialExpressionWorldPosition>(Material, -1450, -100);
		WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
		UMaterialExpressionCameraPositionWS* CameraPosition = AddExpression<UMaterialExpressionCameraPositionWS>(Material, -1450, 40);
		UMaterialExpressionViewProperty* PreExposure = AddExpression<UMaterialExpressionViewProperty>(Material, -1450, 180);
		PreExposure->Property = MEVP_PreExposure;

		UMaterialExpressionVectorParameter* PlayerPosition = AddVectorParameter(Material, TEXT("PlayerWorldPosition"), FLinearColor::Black, -1120, -80);
		UMaterialExpressionScalarParameter* RimAmount = AddScalarParameter(Material, TEXT("RimAmount"), 0.0f, -1120, 40);
		UMaterialExpressionScalarParameter* RimPower = AddScalarParameter(Material, TEXT("RimPower"), 2.3f, -1120, 140);
		UMaterialExpressionScalarParameter* EdgeStrength = AddScalarParameter(Material, TEXT("EdgeStrength"), 1.25f, -1120, 240);
		UMaterialExpressionScalarParameter* NormalEdgeScale = AddScalarParameter(Material, TEXT("NormalEdgeScale"), 1.8f, -1120, 340);
		UMaterialExpressionScalarParameter* DepthEdgeScale = AddScalarParameter(Material, TEXT("DepthEdgeScale"), 320.0f, -1120, 440);
		UMaterialExpressionScalarParameter* GlobalRimFraction = AddScalarParameter(Material, TEXT("GlobalRimFraction"), 0.34f, -820, 40);
		UMaterialExpressionScalarParameter* WaveRadius = AddScalarParameter(Material, TEXT("WaveRadiusCm"), 0.0f, -820, 140);
		UMaterialExpressionScalarParameter* WaveHalfWidth = AddScalarParameter(Material, TEXT("WaveHalfWidthCm"), 430.0f, -820, 240);
		UMaterialExpressionScalarParameter* WaveSoftness = AddScalarParameter(Material, TEXT("WaveSoftnessCm"), 260.0f, -820, 340);
		UMaterialExpressionScalarParameter* MaxSceneDepth = AddScalarParameter(Material, TEXT("MaxSceneDepthCm"), 500000.0f, -820, 440);
		UMaterialExpressionVectorParameter* RimColor = AddVectorParameter(Material, TEXT("RimColor"), FLinearColor(0.24f, 0.88f, 1.0f, 1.0f), -180, 80);
		UMaterialExpressionComponentMask* RimColorRgb = AddExpression<UMaterialExpressionComponentMask>(Material, 0, 80);
		RimColorRgb->Input.Connect(0, RimColor);
		RimColorRgb->R = 1;
		RimColorRgb->G = 1;
		RimColorRgb->B = 1;
		RimColorRgb->A = 0;
		UMaterialExpressionScalarParameter* RimIntensity = AddScalarParameter(Material, TEXT("RimIntensity"), 10.0f, -180, 200);

		UMaterialExpressionCustom* RimMask = AddExpression<UMaterialExpressionCustom>(Material, -430, -210);
		RimMask->Description = TEXT("World-normal Fresnel, depth/normal derivative edges, and player-centered XY wave");
		RimMask->OutputType = CMOT_Float1;
		RimMask->Code =
			TEXT("float3 n = normalize(WorldNormalValue.rgb);\n")
			TEXT("float3 p = WorldPositionValue.xyz;\n")
			TEXT("float3 v = normalize(CameraPositionValue.xyz - p);\n")
			TEXT("float3 l = normalize(PlayerPositionValue.xyz - p);\n")
			TEXT("float fresnel = pow(saturate(1.0f - dot(n, v)), max(RimPowerValue, 0.05f));\n")
			TEXT("float lightSide = 0.25f + 0.75f * saturate(dot(n, l));\n")
			TEXT("float normalEdge = saturate((length(ddx(n)) + length(ddy(n))) * max(NormalEdgeScaleValue, 0.0f));\n")
			TEXT("float depth = SceneDepthValue.r;\n")
			TEXT("float relativeDepthEdge = (abs(ddx(depth)) + abs(ddy(depth))) / max(depth, 1.0f);\n")
			TEXT("float depthEdge = saturate(relativeDepthEdge * max(DepthEdgeScaleValue, 0.0f));\n")
			TEXT("float edge = max(normalEdge, depthEdge) * max(EdgeStrengthValue, 0.0f);\n")
			TEXT("float distanceXY = length(p.xy - PlayerPositionValue.xy);\n")
			TEXT("float waveDelta = abs(distanceXY - max(WaveRadiusValue, 0.0f));\n")
			TEXT("float wave = 1.0f - smoothstep(max(WaveHalfWidthValue, 1.0f), max(WaveHalfWidthValue + WaveSoftnessValue, 2.0f), waveDelta);\n")
			TEXT("float globalFraction = saturate(GlobalRimFractionValue);\n")
			TEXT("float spatialWeight = max(globalFraction, wave);\n")
			TEXT("float rim = max(fresnel * lightSide, edge) * spatialWeight;\n")
			TEXT("float validScene = depth > 0.0f && depth < max(MaxSceneDepthValue, 1.0f) ? 1.0f : 0.0f;\n")
			TEXT("return saturate(rim) * saturate(RimAmountValue) * validScene;");
		AddCustomInput(RimMask, TEXT("WorldNormalValue"), WorldNormal);
		AddCustomInput(RimMask, TEXT("SceneDepthValue"), SceneDepth);
		AddCustomInput(RimMask, TEXT("WorldPositionValue"), WorldPosition);
		AddCustomInput(RimMask, TEXT("CameraPositionValue"), CameraPosition);
		AddCustomInput(RimMask, TEXT("PlayerPositionValue"), PlayerPosition);
		AddCustomInput(RimMask, TEXT("RimAmountValue"), RimAmount);
		AddCustomInput(RimMask, TEXT("RimPowerValue"), RimPower);
		AddCustomInput(RimMask, TEXT("EdgeStrengthValue"), EdgeStrength);
		AddCustomInput(RimMask, TEXT("NormalEdgeScaleValue"), NormalEdgeScale);
		AddCustomInput(RimMask, TEXT("DepthEdgeScaleValue"), DepthEdgeScale);
		AddCustomInput(RimMask, TEXT("GlobalRimFractionValue"), GlobalRimFraction);
		AddCustomInput(RimMask, TEXT("WaveRadiusValue"), WaveRadius);
		AddCustomInput(RimMask, TEXT("WaveHalfWidthValue"), WaveHalfWidth);
		AddCustomInput(RimMask, TEXT("WaveSoftnessValue"), WaveSoftness);
		AddCustomInput(RimMask, TEXT("MaxSceneDepthValue"), MaxSceneDepth);

		UMaterialExpressionMultiply* ColoredRim = AddExpression<UMaterialExpressionMultiply>(Material, 80, -80);
		ColoredRim->A.Connect(0, RimMask);
		ColoredRim->B.Connect(0, RimColorRgb);
		UMaterialExpressionMultiply* IntenseRim = AddExpression<UMaterialExpressionMultiply>(Material, 300, -80);
		IntenseRim->A.Connect(0, ColoredRim);
		IntenseRim->B.Connect(0, RimIntensity);
		UMaterialExpressionMultiply* PreExposedRim = AddExpression<UMaterialExpressionMultiply>(Material, 520, -80);
		PreExposedRim->A.Connect(0, IntenseRim);
		PreExposedRim->B.Connect(0, PreExposure);
		UMaterialExpressionAdd* FinalColor = AddExpression<UMaterialExpressionAdd>(Material, 760, -180);
		FinalColor->A.Connect(0, SceneColorRgb);
		FinalColor->B.Connect(0, PreExposedRim);
		EditorData->EmissiveColor.Connect(0, FinalColor);

		StampAssetVersion(Material);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	UMaterialInstanceConstant* EnsureMaterialInstance(UMaterial* Parent, const FString& AssetName)
	{
		if (!Parent)
		{
			return nullptr;
		}

		UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(
			nullptr,
			*ObjectPath(PublicMaterialPath, AssetName));
		if (!Instance)
		{
			UPackage* Package = CreatePackage(*(PublicMaterialPath / AssetName));
			Instance = Package
				? NewObject<UMaterialInstanceConstant>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional)
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

		if (Instance->Parent != Parent)
		{
			Instance->Modify();
			Instance->SetParentEditorOnly(Parent);
			Instance->PostEditChange();
			Instance->MarkPackageDirty();
			if (!SaveAsset(Instance))
			{
				return nullptr;
			}
		}
		return Instance;
	}

	UTunaWarpTransitionProfile* EnsureDefaultProfile()
	{
		if (UTunaWarpTransitionProfile* Existing = LoadObject<UTunaWarpTransitionProfile>(
			nullptr,
			*ObjectPath(PublicProfilePath, DefaultProfileName)))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*(PublicProfilePath / DefaultProfileName));
		UTunaWarpTransitionProfile* Profile = Package
			? NewObject<UTunaWarpTransitionProfile>(Package, *DefaultProfileName, RF_Public | RF_Standalone | RF_Transactional)
			: nullptr;
		if (!Profile)
		{
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Profile);
		Profile->MarkPackageDirty();
		return SaveAsset(Profile) ? Profile : nullptr;
	}

	void EnsurePluginAssets()
	{
		UMaterial* WarpMaterial = EnsureWarpMaterial();
		UMaterial* RimMaterial = EnsureRimMaterial();
		UMaterialInstanceConstant* WarpInstance = EnsureMaterialInstance(WarpMaterial, WarpInstanceName);
		UMaterialInstanceConstant* RimInstance = EnsureMaterialInstance(RimMaterial, RimInstanceName);
		UTunaWarpTransitionProfile* DefaultProfile = EnsureDefaultProfile();
		if (!WarpMaterial || !RimMaterial || !WarpInstance || !RimInstance || !DefaultProfile)
		{
			UE_LOG(LogTunaWarpTransitionEditor, Error, TEXT("Tuna Warp Transition failed to generate one or more default assets."));
			return;
		}
		UE_LOG(LogTunaWarpTransitionEditor, Log, TEXT("Tuna Warp Transition material and profile assets are ready."));
	}
}

class FTunaWarpTransitionEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		EditorInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
			this,
			&FTunaWarpTransitionEditorModule::OnEditorInitialized);
	}

	virtual void ShutdownModule() override
	{
		if (EditorInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(EditorInitializedHandle);
			EditorInitializedHandle.Reset();
		}
	}

private:
	void OnEditorInitialized(double)
	{
		TunaWarpTransitionEditor::EnsurePluginAssets();
	}

	FDelegateHandle EditorInitializedHandle;
};

IMPLEMENT_MODULE(FTunaWarpTransitionEditorModule, TunaWarpTransitionEditor)
