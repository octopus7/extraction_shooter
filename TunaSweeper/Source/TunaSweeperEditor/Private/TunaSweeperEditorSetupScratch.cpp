#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{
	namespace
	{
		UMaterial* FindOrCreateMaterial(const FString& AssetPath, const FString& AssetName)
		{
			const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
			if (UMaterial* ExistingMaterial = LoadObject<UMaterial>(nullptr, *ObjectPath))
			{
				return ExistingMaterial;
			}

			UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
			UMaterial* Material = Cast<UMaterial>(
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(
					AssetName,
					AssetPath,
					UMaterial::StaticClass(),
					Factory));
			if (Material)
			{
				FAssetRegistryModule::AssetCreated(Material);
			}
			return Material;
		}

		UMaterialExpressionScalarParameter* AddScalarParameter(
			UMaterial* Material,
			const FName ParameterName,
			float DefaultValue,
			int32 X,
			int32 Y)
		{
			UMaterialExpressionScalarParameter* Parameter = NewObject<UMaterialExpressionScalarParameter>(Material);
			Parameter->Material = Material;
			Parameter->ParameterName = ParameterName;
			Parameter->DefaultValue = DefaultValue;
			Parameter->MaterialExpressionEditorX = X;
			Parameter->MaterialExpressionEditorY = Y;
			Material->GetExpressionCollection().AddExpression(Parameter);
			return Parameter;
		}

		UMaterialExpressionConstant* AddConstant(
			UMaterial* Material,
			float Value,
			int32 X,
			int32 Y)
		{
			UMaterialExpressionConstant* Constant = NewObject<UMaterialExpressionConstant>(Material);
			Constant->Material = Material;
			Constant->R = Value;
			Constant->MaterialExpressionEditorX = X;
			Constant->MaterialExpressionEditorY = Y;
			Material->GetExpressionCollection().AddExpression(Constant);
			return Constant;
		}

		UMaterial* EnsureScratchOverlayMaterial()
		{
			UMaterial* Material = FindOrCreateMaterial(EffectsAssetPath, ScratchOverlayMaterialAssetName);
			if (!Material)
			{
				return nullptr;
			}

			Material->Modify();
			Material->GetExpressionCollection().Empty();
			Material->MaterialDomain = MD_Surface;
			Material->BlendMode = BLEND_Additive;
			Material->TwoSided = true;
			Material->bUsedWithSkeletalMesh = true;
			Material->SetShadingModel(MSM_Unlit);

			UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
			if (!EditorOnlyData)
			{
				return nullptr;
			}

			UMaterialExpressionVectorParameter* Color = NewObject<UMaterialExpressionVectorParameter>(Material);
			Color->Material = Material;
			Color->ParameterName = TEXT("ScratchColor");
			Color->DefaultValue = FLinearColor(0.74f, 0.91f, 1.0f, 1.0f);
			Color->MaterialExpressionEditorX = -900;
			Color->MaterialExpressionEditorY = -180;
			Material->GetExpressionCollection().AddExpression(Color);

			UMaterialExpressionScalarParameter* Intensity = AddScalarParameter(
				Material, TEXT("ScratchIntensity"), 2.2f, -900, -40);
			UMaterialExpressionScalarParameter* Alpha = AddScalarParameter(
				Material, TEXT("EffectAlpha"), 0.0f, -900, 100);

			UMaterialExpressionFresnel* Fresnel = NewObject<UMaterialExpressionFresnel>(Material);
			Fresnel->Material = Material;
			Fresnel->Exponent = 3.0f;
			Fresnel->BaseReflectFraction = 0.08f;
			Fresnel->MaterialExpressionEditorX = -900;
			Fresnel->MaterialExpressionEditorY = 260;
			Material->GetExpressionCollection().AddExpression(Fresnel);

			UMaterialExpressionMultiply* FresnelScale = NewObject<UMaterialExpressionMultiply>(Material);
			FresnelScale->Material = Material;
			FresnelScale->A.Connect(0, Fresnel);
			FresnelScale->B.Connect(0, AddConstant(Material, 0.45f, -700, 330));
			FresnelScale->MaterialExpressionEditorX = -500;
			FresnelScale->MaterialExpressionEditorY = 250;
			Material->GetExpressionCollection().AddExpression(FresnelScale);

			UMaterialExpressionAdd* RimFactor = NewObject<UMaterialExpressionAdd>(Material);
			RimFactor->Material = Material;
			RimFactor->A.Connect(0, FresnelScale);
			RimFactor->B.Connect(0, AddConstant(Material, 0.0f, -500, 390));
			RimFactor->MaterialExpressionEditorX = -290;
			RimFactor->MaterialExpressionEditorY = 250;
			Material->GetExpressionCollection().AddExpression(RimFactor);

			UMaterialExpressionMultiply* ColorIntensity = NewObject<UMaterialExpressionMultiply>(Material);
			ColorIntensity->Material = Material;
			ColorIntensity->A.Connect(0, Color);
			ColorIntensity->B.Connect(0, Intensity);
			ColorIntensity->MaterialExpressionEditorX = -610;
			ColorIntensity->MaterialExpressionEditorY = -130;
			Material->GetExpressionCollection().AddExpression(ColorIntensity);

			UMaterialExpressionMultiply* ColorAlpha = NewObject<UMaterialExpressionMultiply>(Material);
			ColorAlpha->Material = Material;
			ColorAlpha->A.Connect(0, ColorIntensity);
			ColorAlpha->B.Connect(0, Alpha);
			ColorAlpha->MaterialExpressionEditorX = -380;
			ColorAlpha->MaterialExpressionEditorY = -100;
			Material->GetExpressionCollection().AddExpression(ColorAlpha);

			UMaterialExpressionMultiply* FinalEmissive = NewObject<UMaterialExpressionMultiply>(Material);
			FinalEmissive->Material = Material;
			FinalEmissive->A.Connect(0, ColorAlpha);
			FinalEmissive->B.Connect(0, RimFactor);
			FinalEmissive->MaterialExpressionEditorX = -80;
			FinalEmissive->MaterialExpressionEditorY = 0;
			Material->GetExpressionCollection().AddExpression(FinalEmissive);

			EditorOnlyData->EmissiveColor.Connect(0, FinalEmissive);
			EditorOnlyData->Opacity.Connect(0, Alpha);
			Material->PostEditChange();
			Material->MarkPackageDirty();
			return SaveAsset(Material) ? Material : nullptr;
		}

		UMaterial* EnsureScratchAfterimageMaterial()
		{
			UMaterial* Material = FindOrCreateMaterial(EffectsAssetPath, ScratchAfterimageMaterialAssetName);
			if (!Material)
			{
				return nullptr;
			}

			Material->Modify();
			Material->GetExpressionCollection().Empty();
			Material->MaterialDomain = MD_Surface;
			Material->BlendMode = BLEND_Translucent;
			Material->TwoSided = true;
			Material->bUsedWithSkeletalMesh = true;
			Material->SetShadingModel(MSM_Unlit);

			UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
			if (!EditorOnlyData)
			{
				return nullptr;
			}

			UMaterialExpressionVectorParameter* Color = NewObject<UMaterialExpressionVectorParameter>(Material);
			Color->Material = Material;
			Color->ParameterName = TEXT("ScratchColor");
			Color->DefaultValue = FLinearColor(0.30f, 0.72f, 1.0f, 1.0f);
			Color->MaterialExpressionEditorX = -760;
			Color->MaterialExpressionEditorY = -130;
			Material->GetExpressionCollection().AddExpression(Color);

			UMaterialExpressionScalarParameter* Intensity = AddScalarParameter(
				Material, TEXT("ScratchIntensity"), 0.85f, -760, 10);
			UMaterialExpressionScalarParameter* Alpha = AddScalarParameter(
				Material, TEXT("EffectAlpha"), 0.0f, -760, 150);

			UMaterialExpressionMultiply* ColorIntensity = NewObject<UMaterialExpressionMultiply>(Material);
			ColorIntensity->Material = Material;
			ColorIntensity->A.Connect(0, Color);
			ColorIntensity->B.Connect(0, Intensity);
			ColorIntensity->MaterialExpressionEditorX = -510;
			ColorIntensity->MaterialExpressionEditorY = -80;
			Material->GetExpressionCollection().AddExpression(ColorIntensity);

			UMaterialExpressionMultiply* EmissiveFade = NewObject<UMaterialExpressionMultiply>(Material);
			EmissiveFade->Material = Material;
			EmissiveFade->A.Connect(0, ColorIntensity);
			EmissiveFade->B.Connect(0, Alpha);
			EmissiveFade->MaterialExpressionEditorX = -260;
			EmissiveFade->MaterialExpressionEditorY = -60;
			Material->GetExpressionCollection().AddExpression(EmissiveFade);

			UMaterialExpressionMultiply* OpacityFade = NewObject<UMaterialExpressionMultiply>(Material);
			OpacityFade->Material = Material;
			OpacityFade->A.Connect(0, Alpha);
			OpacityFade->B.Connect(0, AddConstant(Material, 0.34f, -510, 250));
			OpacityFade->MaterialExpressionEditorX = -260;
			OpacityFade->MaterialExpressionEditorY = 170;
			Material->GetExpressionCollection().AddExpression(OpacityFade);

			EditorOnlyData->EmissiveColor.Connect(0, EmissiveFade);
			EditorOnlyData->Opacity.Connect(0, OpacityFade);
			Material->PostEditChange();
			Material->MarkPackageDirty();
			return SaveAsset(Material) ? Material : nullptr;
		}

		UMaterial* EnsureScratchGaugeMaterial()
		{
			UMaterial* Material = FindOrCreateMaterial(UIAssetPath, ScratchGaugeMaterialAssetName);
			if (!Material)
			{
				return nullptr;
			}

			Material->Modify();
			Material->GetExpressionCollection().Empty();
			Material->MaterialDomain = MD_UI;
			Material->BlendMode = BLEND_Translucent;
			Material->TwoSided = true;

			UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
			if (!EditorOnlyData)
			{
				return nullptr;
			}

			UMaterialExpressionTextureCoordinate* TextureCoordinate = NewObject<UMaterialExpressionTextureCoordinate>(Material);
			TextureCoordinate->Material = Material;
			TextureCoordinate->CoordinateIndex = 0;
			TextureCoordinate->MaterialExpressionEditorX = -650;
			TextureCoordinate->MaterialExpressionEditorY = 0;
			Material->GetExpressionCollection().AddExpression(TextureCoordinate);

			UMaterialExpressionCustom* Rainbow = NewObject<UMaterialExpressionCustom>(Material);
			Rainbow->Material = Material;
			Rainbow->Description = TEXT("Bright low-saturation vertical rainbow for the Scratch gauge");
			Rainbow->OutputType = CMOT_Float3;
			Rainbow->Code =
				TEXT("float hue = frac((1.0f - UV.y) * 0.84f + UV.x * 0.08f);\n")
				TEXT("float3 rgb = saturate(abs(frac(hue + float3(0.0f, 0.6666667f, 0.3333333f)) * 6.0f - 3.0f) - 1.0f);\n")
				TEXT("float luminance = dot(rgb, float3(0.299f, 0.587f, 0.114f));\n")
				TEXT("float3 pastel = lerp(luminance.xxx, rgb, 0.48f);\n")
				TEXT("return saturate(pastel * 1.18f + 0.12f);");
			Rainbow->MaterialExpressionEditorX = -360;
			Rainbow->MaterialExpressionEditorY = 0;
			FCustomInput UvInput;
			UvInput.InputName = TEXT("UV");
			UvInput.Input.Connect(0, TextureCoordinate);
			Rainbow->Inputs.Add(UvInput);
			Material->GetExpressionCollection().AddExpression(Rainbow);

			UMaterialExpressionConstant* Opacity = AddConstant(Material, 1.0f, -160, 170);
			EditorOnlyData->EmissiveColor.Connect(0, Rainbow);
			EditorOnlyData->Opacity.Connect(0, Opacity);
			Material->PostEditChange();
			Material->MarkPackageDirty();
			return SaveAsset(Material) ? Material : nullptr;
		}
	}

	bool EnsureScratchPresentationAssets()
	{
		if (!EnsureScratchOverlayMaterial() ||
			!EnsureScratchAfterimageMaterial() ||
			!EnsureScratchGaugeMaterial())
		{
			return false;
		}

		UWidgetBlueprint* BottomStatusWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudBottomStatusWidgetAssetName,
			UTunaSweeperHudBottomStatusWidget::StaticClass());
		if (!BottomStatusWidgetBlueprint || !BuildHudBottomStatusWidgetTree(BottomStatusWidgetBlueprint))
		{
			return false;
		}

		RegisterAllWidgetsInTree(BottomStatusWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(BottomStatusWidgetBlueprint);
		BottomStatusWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(BottomStatusWidgetBlueprint);
	}
}
