#include "TunaSweeperPuddleSkyReflectionMaterial.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/Texture2D.h"
#include "Factories/MaterialFactoryNew.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionPixelNormalWS.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperPuddleSkyReflectionMaterial, Log, All);

namespace TunaSweeperPuddleSkyReflectionMaterial
{
	const FString AssetPath = TEXT("/Game/Prototype/Water");
	const FString MaterialAssetName = TEXT("M_PuddleSkyReflection");
	const FString MaterialInstanceAssetName = TEXT("MI_PuddleSkyReflection");
	const FString DefaultSkyTextureAssetName = TEXT("T_PuddleSkyReflection_DefaultSkyClouds");

	const FName SkyCloudTextureParameterName = TEXT("SkyCloudTexture");
	const FName CloudHeightParameterName = TEXT("CloudHeight");
	const FName CloudScaleParameterName = TEXT("CloudScale");
	const FName MaxTraceDistanceParameterName = TEXT("MaxTraceDistance");
	const FName WindSpeedParameterName = TEXT("WindSpeed");
	const FName WindAngleDegreesParameterName = TEXT("WindAngleDegrees");
	const FName ReflectionIntensityParameterName = TEXT("ReflectionIntensity");
	const FName WaterTintParameterName = TEXT("WaterTint");
	const FName WaterTintStrengthParameterName = TEXT("WaterTintStrength");
	const FName OpacityParameterName = TEXT("Opacity");

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

		const FString PackageFileName =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}

	UTexture2D* EnsureDefaultSkyCloudTexture()
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, DefaultSkyTextureAssetName);
		if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath))
		{
			return ExistingTexture;
		}

		constexpr int32 TextureSize = 512;
		const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *DefaultSkyTextureAssetName);
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogTunaSweeperPuddleSkyReflectionMaterial, Error, TEXT("Failed to create %s."), *PackageName);
			return nullptr;
		}

		UTexture2D* Texture = NewObject<UTexture2D>(
			Package,
			*DefaultSkyTextureAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Texture)
		{
			return nullptr;
		}

		TArray<uint8> Pixels;
		Pixels.SetNum(TextureSize * TextureSize * 4);

		for (int32 Y = 0; Y < TextureSize; ++Y)
		{
			for (int32 X = 0; X < TextureSize; ++X)
			{
				const float U = static_cast<float>(X) / static_cast<float>(TextureSize - 1);
				const float V = static_cast<float>(Y) / static_cast<float>(TextureSize - 1);
				const float CloudA = FMath::PerlinNoise2D(FVector2D(U * 5.0f + 13.5f, V * 4.5f - 8.2f)) * 0.5f + 0.5f;
				const float CloudB = FMath::PerlinNoise2D(FVector2D(U * 10.0f - 1.8f, V * 9.0f + 4.4f)) * 0.5f + 0.5f;
				const float CloudC = FMath::PerlinNoise2D(FVector2D(U * 22.0f + 2.1f, V * 18.0f - 5.7f)) * 0.5f + 0.5f;
				const float CloudMask = FMath::SmoothStep(0.50f, 0.86f, CloudA * 0.62f + CloudB * 0.28f + CloudC * 0.10f);
				const FLinearColor SkyColor = FLinearColor(
					FMath::Lerp(0.28f, 0.52f, V),
					FMath::Lerp(0.55f, 0.74f, V),
					FMath::Lerp(0.95f, 1.00f, V),
					1.0f);
				const FLinearColor CloudColor = FLinearColor(0.92f, 0.96f, 1.0f, 1.0f);
				const FLinearColor FinalColor = FLinearColor::LerpUsingHSV(SkyColor, CloudColor, CloudMask * 0.82f);
				const int32 PixelIndex = (Y * TextureSize + X) * 4;
				Pixels[PixelIndex + 0] = FMath::Clamp(FMath::RoundToInt(FinalColor.B * 255.0f), 0, 255);
				Pixels[PixelIndex + 1] = FMath::Clamp(FMath::RoundToInt(FinalColor.G * 255.0f), 0, 255);
				Pixels[PixelIndex + 2] = FMath::Clamp(FMath::RoundToInt(FinalColor.R * 255.0f), 0, 255);
				Pixels[PixelIndex + 3] = 255;
			}
		}

		Texture->Source.Init(TextureSize, TextureSize, 1, 1, TSF_BGRA8, Pixels.GetData());
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->LODGroup = TEXTUREGROUP_World;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Texture);

		if (!SaveAsset(Texture))
		{
			UE_LOG(LogTunaSweeperPuddleSkyReflectionMaterial, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Texture;
	}

	UMaterial* FindOrCreateMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, MaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (Material)
		{
			return Material;
		}

		UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			MaterialAssetName,
			AssetPath,
			UMaterial::StaticClass(),
			MaterialFactory);

		Material = Cast<UMaterial>(CreatedAsset);
		if (!Material)
		{
			UE_LOG(LogTunaSweeperPuddleSkyReflectionMaterial, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Material);
		return Material;
	}

	UMaterialExpressionScalarParameter* AddScalarParameter(
		UMaterial* Material,
		const FName ParameterName,
		const float DefaultValue,
		const int32 EditorX,
		const int32 EditorY)
	{
		UMaterialExpressionScalarParameter* Expression = NewObject<UMaterialExpressionScalarParameter>(Material);
		Expression->Material = Material;
		Expression->ParameterName = ParameterName;
		Expression->DefaultValue = DefaultValue;
		Expression->MaterialExpressionEditorX = EditorX;
		Expression->MaterialExpressionEditorY = EditorY;
		Material->GetExpressionCollection().AddExpression(Expression);
		return Expression;
	}

	UMaterial* EnsureMaterial(UTexture2D* DefaultSkyTexture)
	{
		if (!DefaultSkyTexture)
		{
			return nullptr;
		}

		UMaterial* Material = FindOrCreateMaterial();
		if (!Material)
		{
			return nullptr;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Translucent;
		Material->TwoSided = true;
		Material->SetShadingModel(MSM_Unlit);

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(
				LogTunaSweeperPuddleSkyReflectionMaterial,
				Error,
				TEXT("Failed to edit %s."),
				*GetAssetObjectPath(AssetPath, MaterialAssetName));
			return nullptr;
		}

		UMaterialExpressionWorldPosition* WorldPosition = NewObject<UMaterialExpressionWorldPosition>(Material);
		WorldPosition->Material = Material;
		WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
		WorldPosition->MaterialExpressionEditorX = -1180;
		WorldPosition->MaterialExpressionEditorY = -360;
		Material->GetExpressionCollection().AddExpression(WorldPosition);

		UMaterialExpressionCameraPositionWS* CameraPosition = NewObject<UMaterialExpressionCameraPositionWS>(Material);
		CameraPosition->Material = Material;
		CameraPosition->MaterialExpressionEditorX = -1180;
		CameraPosition->MaterialExpressionEditorY = -240;
		Material->GetExpressionCollection().AddExpression(CameraPosition);

		UMaterialExpressionPixelNormalWS* PixelNormal = NewObject<UMaterialExpressionPixelNormalWS>(Material);
		PixelNormal->Material = Material;
		PixelNormal->MaterialExpressionEditorX = -1180;
		PixelNormal->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(PixelNormal);

		UMaterialExpressionTime* Time = NewObject<UMaterialExpressionTime>(Material);
		Time->Material = Material;
		Time->bIgnorePause = true;
		Time->MaterialExpressionEditorX = -1180;
		Time->MaterialExpressionEditorY = 20;
		Material->GetExpressionCollection().AddExpression(Time);

		UMaterialExpressionScalarParameter* CloudHeight =
			AddScalarParameter(Material, CloudHeightParameterName, 9000.0f, -1180, 160);
		UMaterialExpressionScalarParameter* CloudScale =
			AddScalarParameter(Material, CloudScaleParameterName, 0.00016f, -1180, 250);
		UMaterialExpressionScalarParameter* MaxTraceDistance =
			AddScalarParameter(Material, MaxTraceDistanceParameterName, 30000.0f, -1180, 340);
		UMaterialExpressionScalarParameter* WindSpeed =
			AddScalarParameter(Material, WindSpeedParameterName, 0.0f, -1180, 430);
		UMaterialExpressionScalarParameter* WindAngleDegrees =
			AddScalarParameter(Material, WindAngleDegreesParameterName, 18.0f, -1180, 520);

		UMaterialExpressionCustom* ReflectionUv = NewObject<UMaterialExpressionCustom>(Material);
		ReflectionUv->Material = Material;
		ReflectionUv->Description = TEXT("Planar sky reflection UV");
		ReflectionUv->OutputType = CMOT_Float2;
		ReflectionUv->Code =
			TEXT("float3 baseNormal = normalize(SurfaceNormal);\n")
			TEXT("float3 viewRay = normalize(WorldPos - CameraPos);\n")
			TEXT("float3 reflectionRay = reflect(viewRay, baseNormal);\n")
			TEXT("float targetHeight = max(CloudHeight, WorldPos.z + 100.0f);\n")
			TEXT("float rayZ = max(reflectionRay.z, 0.001f);\n")
			TEXT("float traceT = saturate(((targetHeight - WorldPos.z) / rayZ) / max(MaxTraceDistance, 1.0f));\n")
			TEXT("traceT *= MaxTraceDistance;\n")
			TEXT("float angleRadians = radians(WindAngleDegrees);\n")
			TEXT("float2 windDir = float2(cos(angleRadians), sin(angleRadians));\n")
			TEXT("float2 hitUv = (WorldPos.xy + reflectionRay.xy * traceT) * CloudScale;\n")
			TEXT("return hitUv + windDir * Time * WindSpeed;");

		FCustomInput WorldPosInput;
		WorldPosInput.InputName = TEXT("WorldPos");
		WorldPosInput.Input.Connect(0, WorldPosition);
		ReflectionUv->Inputs.Add(WorldPosInput);

		FCustomInput CameraPosInput;
		CameraPosInput.InputName = TEXT("CameraPos");
		CameraPosInput.Input.Connect(0, CameraPosition);
		ReflectionUv->Inputs.Add(CameraPosInput);

		FCustomInput SurfaceNormalInput;
		SurfaceNormalInput.InputName = TEXT("SurfaceNormal");
		SurfaceNormalInput.Input.Connect(0, PixelNormal);
		ReflectionUv->Inputs.Add(SurfaceNormalInput);

		FCustomInput TimeInput;
		TimeInput.InputName = TEXT("Time");
		TimeInput.Input.Connect(0, Time);
		ReflectionUv->Inputs.Add(TimeInput);

		FCustomInput CloudHeightInput;
		CloudHeightInput.InputName = TEXT("CloudHeight");
		CloudHeightInput.Input.Connect(0, CloudHeight);
		ReflectionUv->Inputs.Add(CloudHeightInput);

		FCustomInput CloudScaleInput;
		CloudScaleInput.InputName = TEXT("CloudScale");
		CloudScaleInput.Input.Connect(0, CloudScale);
		ReflectionUv->Inputs.Add(CloudScaleInput);

		FCustomInput MaxTraceDistanceInput;
		MaxTraceDistanceInput.InputName = TEXT("MaxTraceDistance");
		MaxTraceDistanceInput.Input.Connect(0, MaxTraceDistance);
		ReflectionUv->Inputs.Add(MaxTraceDistanceInput);

		FCustomInput WindSpeedInput;
		WindSpeedInput.InputName = TEXT("WindSpeed");
		WindSpeedInput.Input.Connect(0, WindSpeed);
		ReflectionUv->Inputs.Add(WindSpeedInput);

		FCustomInput WindAngleInput;
		WindAngleInput.InputName = TEXT("WindAngleDegrees");
		WindAngleInput.Input.Connect(0, WindAngleDegrees);
		ReflectionUv->Inputs.Add(WindAngleInput);

		ReflectionUv->MaterialExpressionEditorX = -780;
		ReflectionUv->MaterialExpressionEditorY = 130;
		Material->GetExpressionCollection().AddExpression(ReflectionUv);

		UMaterialExpressionTextureSampleParameter2D* SkyCloudSample =
			NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		SkyCloudSample->Material = Material;
		SkyCloudSample->ParameterName = SkyCloudTextureParameterName;
		SkyCloudSample->Texture = DefaultSkyTexture;
		SkyCloudSample->SamplerType = SAMPLERTYPE_Color;
		SkyCloudSample->Coordinates.Connect(0, ReflectionUv);
		SkyCloudSample->MaterialExpressionEditorX = -480;
		SkyCloudSample->MaterialExpressionEditorY = 40;
		Material->GetExpressionCollection().AddExpression(SkyCloudSample);

		UMaterialExpressionScalarParameter* ReflectionIntensity =
			AddScalarParameter(Material, ReflectionIntensityParameterName, 1.08f, -480, -190);
		UMaterialExpressionMultiply* ReflectedSkyColor = NewObject<UMaterialExpressionMultiply>(Material);
		ReflectedSkyColor->Material = Material;
		ReflectedSkyColor->A.Connect(0, SkyCloudSample);
		ReflectedSkyColor->B.Connect(0, ReflectionIntensity);
		ReflectedSkyColor->MaterialExpressionEditorX = -180;
		ReflectedSkyColor->MaterialExpressionEditorY = -20;
		Material->GetExpressionCollection().AddExpression(ReflectedSkyColor);

		UMaterialExpressionVectorParameter* WaterTint = NewObject<UMaterialExpressionVectorParameter>(Material);
		WaterTint->Material = Material;
		WaterTint->ParameterName = WaterTintParameterName;
		WaterTint->DefaultValue = FLinearColor(0.035f, 0.22f, 0.28f, 1.0f);
		WaterTint->MaterialExpressionEditorX = -480;
		WaterTint->MaterialExpressionEditorY = 250;
		Material->GetExpressionCollection().AddExpression(WaterTint);

		UMaterialExpressionScalarParameter* WaterTintStrength =
			AddScalarParameter(Material, WaterTintStrengthParameterName, 0.18f, -480, 360);
		UMaterialExpressionMultiply* WaterTintColor = NewObject<UMaterialExpressionMultiply>(Material);
		WaterTintColor->Material = Material;
		WaterTintColor->A.Connect(0, WaterTint);
		WaterTintColor->B.Connect(0, WaterTintStrength);
		WaterTintColor->MaterialExpressionEditorX = -180;
		WaterTintColor->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(WaterTintColor);

		UMaterialExpressionAdd* FinalColor = NewObject<UMaterialExpressionAdd>(Material);
		FinalColor->Material = Material;
		FinalColor->A.Connect(0, ReflectedSkyColor);
		FinalColor->B.Connect(0, WaterTintColor);
		FinalColor->MaterialExpressionEditorX = 120;
		FinalColor->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(FinalColor);

		UMaterialExpressionScalarParameter* Opacity =
			AddScalarParameter(Material, OpacityParameterName, 0.68f, 120, 360);

		MaterialEditorOnly->BaseColor.Connect(0, FinalColor);
		MaterialEditorOnly->EmissiveColor.Connect(0, FinalColor);
		MaterialEditorOnly->Opacity.Connect(0, Opacity);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.045f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.8f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(
				LogTunaSweeperPuddleSkyReflectionMaterial,
				Error,
				TEXT("Failed to save %s."),
				*GetAssetObjectPath(AssetPath, MaterialAssetName));
			return nullptr;
		}

		return Material;
	}

	UMaterialInstanceConstant* EnsureMaterialInstance(UMaterial* ParentMaterial, UTexture2D* DefaultSkyTexture)
	{
		if (!ParentMaterial || !DefaultSkyTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(AssetPath, MaterialInstanceAssetName);
		UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *ObjectPath);
		if (!MaterialInstance)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *MaterialInstanceAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			MaterialInstance = NewObject<UMaterialInstanceConstant>(
				Package,
				*MaterialInstanceAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!MaterialInstance)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(MaterialInstance);
		}

		MaterialInstance->Modify();
		MaterialInstance->SetParentEditorOnly(ParentMaterial);
		MaterialInstance->ClearParameterValuesEditorOnly();
		MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(SkyCloudTextureParameterName), DefaultSkyTexture);
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(CloudHeightParameterName), 9000.0f);
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(CloudScaleParameterName), 0.00016f);
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(MaxTraceDistanceParameterName), 30000.0f);
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(WindSpeedParameterName), 0.0f);
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(WindAngleDegreesParameterName), 18.0f);
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(ReflectionIntensityParameterName), 1.08f);
		MaterialInstance->SetVectorParameterValueEditorOnly(
			FMaterialParameterInfo(WaterTintParameterName),
			FLinearColor(0.035f, 0.22f, 0.28f, 1.0f));
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(WaterTintStrengthParameterName), 0.18f);
		MaterialInstance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(OpacityParameterName), 0.68f);
		MaterialInstance->PostEditChange();
		MaterialInstance->MarkPackageDirty();

		if (!SaveAsset(MaterialInstance))
		{
			UE_LOG(LogTunaSweeperPuddleSkyReflectionMaterial, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return MaterialInstance;
	}

	bool EnsureAssets()
	{
		UTexture2D* DefaultSkyTexture = EnsureDefaultSkyCloudTexture();
		UMaterial* Material = EnsureMaterial(DefaultSkyTexture);
		UMaterialInstanceConstant* MaterialInstance = EnsureMaterialInstance(Material, DefaultSkyTexture);

		const bool bSucceeded = DefaultSkyTexture && Material && MaterialInstance;
		if (bSucceeded)
		{
			UE_LOG(
				LogTunaSweeperPuddleSkyReflectionMaterial,
				Display,
				TEXT("Puddle sky reflection assets ready. Material=%s Instance=%s"),
				*GetAssetObjectPath(AssetPath, MaterialAssetName),
				*GetAssetObjectPath(AssetPath, MaterialInstanceAssetName));
		}
		else
		{
			UE_LOG(
				LogTunaSweeperPuddleSkyReflectionMaterial,
				Error,
				TEXT("Puddle sky reflection assets failed. Material=%s Instance=%s"),
				*GetAssetObjectPath(AssetPath, MaterialAssetName),
				*GetAssetObjectPath(AssetPath, MaterialInstanceAssetName));
		}
		return bSucceeded;
	}
}
