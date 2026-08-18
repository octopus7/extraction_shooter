#include "RegionalGroundFogActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "ComponentVisualizer.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "IAssetTools.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PrimitiveDrawInterface.h"
#include "SceneManagement.h"
#include "UObject/SavePackage.h"
#include "Engine/Texture2D.h"
#include "Factories/MaterialFactoryNew.h"

#define LOCTEXT_NAMESPACE "RegionalGroundFogEditor"

namespace RegionalGroundFogEditor
{
	const FString PluginContentPath = TEXT("/RegionalGroundFog");
	const FString TexturePath = PluginContentPath / TEXT("Textures");
	const FString TextureName = TEXT("T_RegionalGroundFogDensity_01");
	const FString MaterialPath = PluginContentPath / TEXT("Materials");
	const FString MaterialName = TEXT("M_RegionalGroundFogCard");

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

		const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, SaveArgs);
	}

	UTexture2D* EnsureDensityTexture()
	{
		const FString TextureObjectPath = ObjectPath(TexturePath, TextureName);
		if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *TextureObjectPath))
		{
			return ExistingTexture;
		}

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("RegionalGroundFog"));
		if (!Plugin.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("RegionalGroundFog: could not locate its plugin directory."));
			return nullptr;
		}

		const FString SourcePath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/SourceArt/T_RegionalGroundFogDensity_01.png"));
		if (!FPaths::FileExists(SourcePath))
		{
			UE_LOG(LogTemp, Error, TEXT("RegionalGroundFog: missing density source image: %s"), *SourcePath);
			return nullptr;
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = TexturePath;
		ImportData->Filenames.Add(SourcePath);
		ImportData->bReplaceExisting = false;
		ImportData->bSkipReadOnly = true;

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		AssetTools.ImportAssetsAutomated(ImportData);

		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TextureObjectPath);
		if (!Texture)
		{
			UE_LOG(LogTemp, Error, TEXT("RegionalGroundFog: failed to import density texture into %s."), *TexturePath);
			return nullptr;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->SRGB = false;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
		return Texture;
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

	UMaterial* EnsureFogCardMaterial(UTexture2D* DensityTexture)
	{
		if (!DensityTexture)
		{
			return nullptr;
		}

		const FString MaterialObjectPath = ObjectPath(MaterialPath, MaterialName);
		if (UMaterial* ExistingMaterial = LoadObject<UMaterial>(nullptr, *MaterialObjectPath))
		{
			return ExistingMaterial;
		}

		UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		UMaterial* Material = Cast<UMaterial>(AssetTools.CreateAsset(MaterialName, MaterialPath, UMaterial::StaticClass(), MaterialFactory));
		if (!Material)
		{
			UE_LOG(LogTemp, Error, TEXT("RegionalGroundFog: failed to create %s."), *MaterialObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Material);
		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
		if (!EditorOnlyData)
		{
			return nullptr;
		}

		UMaterialExpressionTextureSampleParameter2D* Density = AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, -620, 0);
		Density->ParameterName = TEXT("FogDensity");
		Density->Texture = DensityTexture;
		Density->SamplerType = SAMPLERTYPE_Color;

		UMaterialExpressionVectorParameter* FogColor = AddExpression<UMaterialExpressionVectorParameter>(Material, -620, -180);
		FogColor->ParameterName = TEXT("FogColor");
		FogColor->DefaultValue = FLinearColor(0.75f, 0.84f, 0.93f, 1.0f);

		UMaterialExpressionScalarParameter* OpacityScale = AddExpression<UMaterialExpressionScalarParameter>(Material, -620, 220);
		OpacityScale->ParameterName = TEXT("OpacityScale");
		OpacityScale->DefaultValue = 0.16f;

		UMaterialExpressionMultiply* Emissive = AddExpression<UMaterialExpressionMultiply>(Material, -260, -80);
		Emissive->A.Connect(0, Density);
		Emissive->B.Connect(0, FogColor);

		UMaterialExpressionMultiply* Opacity = AddExpression<UMaterialExpressionMultiply>(Material, -260, 190);
		Opacity->A.Connect(4, Density);
		Opacity->B.Connect(0, OpacityScale);

		EditorOnlyData->EmissiveColor.Connect(0, Emissive);
		EditorOnlyData->Opacity.Connect(0, Opacity);

		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	void EnsurePluginAssets()
	{
		UTexture2D* DensityTexture = EnsureDensityTexture();
		EnsureFogCardMaterial(DensityTexture);
	}

	class FRegionalGroundFogVisualizer final : public FComponentVisualizer
	{
	public:
		virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override
		{
			const URegionalGroundFogVisualizationComponent* VisualizationComponent = Cast<URegionalGroundFogVisualizationComponent>(Component);
			const ARegionalGroundFogActor* FogActor = VisualizationComponent ? Cast<ARegionalGroundFogActor>(VisualizationComponent->GetOwner()) : nullptr;
			if (!FogActor)
			{
				return;
			}

			const FTransform ActorTransform = FogActor->GetActorTransform();
			for (const FRegionalGroundFogNode& Node : FogActor->GetFogNodes())
			{
				const FVector Center = ActorTransform.TransformPosition(Node.LocalCenter);
				DrawWireSphere(PDI, Center, FColor(65, 216, 235), Node.CoreRadius, 20, SDPG_World, 1.5f);
				DrawWireSphere(PDI, Center, FColor(56, 116, 255), Node.OuterRadius, 28, SDPG_World, 2.0f);
				PDI->DrawPoint(Center, FColor::White, 10.0f, SDPG_World);
			}
		}
	};
}

class FRegionalGroundFogEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		EditorInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(this, &FRegionalGroundFogEditorModule::OnEditorInitialized);

		if (GUnrealEd)
		{
			Visualizer = MakeShared<RegionalGroundFogEditor::FRegionalGroundFogVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(URegionalGroundFogVisualizationComponent::StaticClass()->GetFName(), Visualizer);
		}
	}

	virtual void ShutdownModule() override
	{
		if (EditorInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(EditorInitializedHandle);
			EditorInitializedHandle.Reset();
		}

		if (GUnrealEd && Visualizer.IsValid())
		{
			GUnrealEd->UnregisterComponentVisualizer(URegionalGroundFogVisualizationComponent::StaticClass()->GetFName());
		}
		Visualizer.Reset();
	}

private:
	void OnEditorInitialized(double)
	{
		RegionalGroundFogEditor::EnsurePluginAssets();
	}

	TSharedPtr<FComponentVisualizer> Visualizer;
	FDelegateHandle EditorInitializedHandle;
};

IMPLEMENT_MODULE(FRegionalGroundFogEditorModule, RegionalGroundFogEditor)

#undef LOCTEXT_NAMESPACE
