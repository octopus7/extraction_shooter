#include "SogSplatEditorUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetTypeActions_Base.h"
#include "AssetToolsModule.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "SogActorFactory.h"
#include "SogAsset.h"
#include "Subsystems/PlacementSubsystem.h"
#include "Editor.h"
#include "UObject/SavePackage.h"
#include "Factories/MaterialFactoryNew.h"

#define LOCTEXT_NAMESPACE "SogSplatEditor"

namespace
{
	const FString DefaultMaterialPath = TEXT("/SogSplat/Materials");
	const FString DefaultMaterialName = TEXT("M_SogSoftEllipse");
	const FString DefaultMaterialObjectPath = DefaultMaterialPath / DefaultMaterialName + TEXT(".") + DefaultMaterialName;

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

	class FAssetTypeActions_SogAsset final : public FAssetTypeActions_Base
	{
	public:
		explicit FAssetTypeActions_SogAsset(EAssetTypeCategories::Type InCategory)
			: Category(InCategory)
		{
		}

		virtual FText GetName() const override
		{
			return LOCTEXT("SogAssetName", "SOG Splat");
		}

		virtual FColor GetTypeColor() const override
		{
			return FColor(98, 180, 255);
		}

		virtual UClass* GetSupportedClass() const override
		{
			return USogAsset::StaticClass();
		}

		virtual uint32 GetCategories() override
		{
			return Category;
		}

	private:
		EAssetTypeCategories::Type Category;
	};
}

const TCHAR* SogSplatEditorUtils::GetDefaultMaterialObjectPath()
{
	return *DefaultMaterialObjectPath;
}

UMaterialInterface* SogSplatEditorUtils::EnsureDefaultSogMaterial()
{
	UMaterial* Material = LoadObject<UMaterial>(nullptr, *DefaultMaterialObjectPath);
	if (!Material)
	{
		UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			DefaultMaterialName,
			DefaultMaterialPath,
			UMaterial::StaticClass(),
			MaterialFactory);

		Material = Cast<UMaterial>(CreatedAsset);
		if (!Material)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create default SOG material at %s."), *DefaultMaterialObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Material);
	}

	Material->Modify();
	Material->GetExpressionCollection().Empty();
	Material->BlendMode = BLEND_Translucent;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	Material->bUsedWithInstancedStaticMeshes = true;

	UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
	if (!MaterialEditorOnly)
	{
		return Material;
	}

	UMaterialExpressionPerInstanceCustomData3Vector* InstanceColor = NewObject<UMaterialExpressionPerInstanceCustomData3Vector>(Material);
	InstanceColor->Material = Material;
	InstanceColor->DataIndex = 0;
	InstanceColor->ConstDefaultValue = FLinearColor::White;
	InstanceColor->MaterialExpressionEditorX = -680;
	InstanceColor->MaterialExpressionEditorY = -120;
	Material->GetExpressionCollection().AddExpression(InstanceColor);

	UMaterialExpressionScalarParameter* Intensity = NewObject<UMaterialExpressionScalarParameter>(Material);
	Intensity->Material = Material;
	Intensity->ParameterName = TEXT("Intensity");
	Intensity->DefaultValue = 1.0f;
	Intensity->MaterialExpressionEditorX = -680;
	Intensity->MaterialExpressionEditorY = 80;
	Material->GetExpressionCollection().AddExpression(Intensity);

	UMaterialExpressionMultiply* Emissive = NewObject<UMaterialExpressionMultiply>(Material);
	Emissive->Material = Material;
	Emissive->A.Connect(0, InstanceColor);
	Emissive->B.Connect(0, Intensity);
	Emissive->MaterialExpressionEditorX = -360;
	Emissive->MaterialExpressionEditorY = -20;
	Material->GetExpressionCollection().AddExpression(Emissive);

	UMaterialExpressionPerInstanceCustomData* InstanceAlpha = NewObject<UMaterialExpressionPerInstanceCustomData>(Material);
	InstanceAlpha->Material = Material;
	InstanceAlpha->DataIndex = 3;
	InstanceAlpha->ConstDefaultValue = 1.0f;
	InstanceAlpha->MaterialExpressionEditorX = -680;
	InstanceAlpha->MaterialExpressionEditorY = 260;
	Material->GetExpressionCollection().AddExpression(InstanceAlpha);

	UMaterialExpressionTextureCoordinate* TextureCoordinate = NewObject<UMaterialExpressionTextureCoordinate>(Material);
	TextureCoordinate->Material = Material;
	TextureCoordinate->CoordinateIndex = 0;
	TextureCoordinate->MaterialExpressionEditorX = -680;
	TextureCoordinate->MaterialExpressionEditorY = 420;
	Material->GetExpressionCollection().AddExpression(TextureCoordinate);

	UMaterialExpressionScalarParameter* GaussianSigmaRadius = NewObject<UMaterialExpressionScalarParameter>(Material);
	GaussianSigmaRadius->Material = Material;
	GaussianSigmaRadius->ParameterName = TEXT("GaussianSigmaRadius");
	GaussianSigmaRadius->DefaultValue = 3.0f;
	GaussianSigmaRadius->MaterialExpressionEditorX = -680;
	GaussianSigmaRadius->MaterialExpressionEditorY = 580;
	Material->GetExpressionCollection().AddExpression(GaussianSigmaRadius);

	UMaterialExpressionScalarParameter* EdgeFade = NewObject<UMaterialExpressionScalarParameter>(Material);
	EdgeFade->Material = Material;
	EdgeFade->ParameterName = TEXT("EdgeFade");
	EdgeFade->DefaultValue = 0.18f;
	EdgeFade->MaterialExpressionEditorX = -680;
	EdgeFade->MaterialExpressionEditorY = 720;
	Material->GetExpressionCollection().AddExpression(EdgeFade);

	UMaterialExpressionCustom* SoftEllipseOpacity = NewObject<UMaterialExpressionCustom>(Material);
	SoftEllipseOpacity->Material = Material;
	SoftEllipseOpacity->Description = TEXT("Soft ellipse opacity");
	SoftEllipseOpacity->OutputType = CMOT_Float1;
	SoftEllipseOpacity->Code =
		TEXT("float2 p = UV * 2.0f - 1.0f;\n")
		TEXT("float radiusSq = dot(p, p);\n")
		TEXT("float sigmaRadius = max(GaussianSigmaRadius, 0.0001f);\n")
		TEXT("float gaussian = exp(-0.5f * radiusSq * sigmaRadius * sigmaRadius);\n")
		TEXT("float edge = saturate((1.0f - radiusSq) / max(EdgeFade, 0.0001f));\n")
		TEXT("edge = edge * edge * (3.0f - 2.0f * edge);\n")
		TEXT("return Alpha * gaussian * edge;");

	FCustomInput UvInput;
	UvInput.InputName = TEXT("UV");
	UvInput.Input.Connect(0, TextureCoordinate);
	SoftEllipseOpacity->Inputs.Add(UvInput);

	FCustomInput AlphaInput;
	AlphaInput.InputName = TEXT("Alpha");
	AlphaInput.Input.Connect(0, InstanceAlpha);
	SoftEllipseOpacity->Inputs.Add(AlphaInput);

	FCustomInput SigmaRadiusInput;
	SigmaRadiusInput.InputName = TEXT("GaussianSigmaRadius");
	SigmaRadiusInput.Input.Connect(0, GaussianSigmaRadius);
	SoftEllipseOpacity->Inputs.Add(SigmaRadiusInput);

	FCustomInput EdgeInput;
	EdgeInput.InputName = TEXT("EdgeFade");
	EdgeInput.Input.Connect(0, EdgeFade);
	SoftEllipseOpacity->Inputs.Add(EdgeInput);

	SoftEllipseOpacity->MaterialExpressionEditorX = -320;
	SoftEllipseOpacity->MaterialExpressionEditorY = 380;
	Material->GetExpressionCollection().AddExpression(SoftEllipseOpacity);

	MaterialEditorOnly->BaseColor.Connect(0, InstanceColor);
	MaterialEditorOnly->EmissiveColor.Connect(0, Emissive);
	MaterialEditorOnly->Opacity.Connect(0, SoftEllipseOpacity);

	Material->PostEditChange();
	Material->MarkPackageDirty();
	SaveAsset(Material);
	return Material;
}

class FSogSplatEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("SogSplat")), LOCTEXT("SogSplatCategory", "SOG Splat"));
		AssetTypeActions = MakeShared<FAssetTypeActions_SogAsset>(AssetCategory);
		AssetTools.RegisterAssetTypeActions(AssetTypeActions.ToSharedRef());

		RegisterActorFactory();
		SogSplatEditorUtils::EnsureDefaultSogMaterial();
	}

	virtual void ShutdownModule() override
	{
		UnregisterActorFactory();

		if (AssetTypeActions.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
			AssetTools.UnregisterAssetTypeActions(AssetTypeActions.ToSharedRef());
		}
		AssetTypeActions.Reset();
	}

private:
	void RegisterActorFactory()
	{
		if (IsRunningCommandlet() || !GEditor || SogActorFactory)
		{
			return;
		}

		for (UActorFactory* ExistingFactory : GEditor->ActorFactories)
		{
			if (USogActorFactory* ExistingSogFactory = Cast<USogActorFactory>(ExistingFactory))
			{
				SogActorFactory = ExistingSogFactory;
				break;
			}
		}

		if (!SogActorFactory)
		{
			SogActorFactory = NewObject<USogActorFactory>(GetTransientPackage(), USogActorFactory::StaticClass());
		}

		if (!SogActorFactory)
		{
			return;
		}

		GEditor->ActorFactories.AddUnique(SogActorFactory);

		if (UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
		{
			if (!PlacementSubsystem->GetAssetFactoryFromFactoryClass(USogActorFactory::StaticClass()))
			{
				TScriptInterface<IAssetFactoryInterface> FactoryInterface(SogActorFactory);
				PlacementSubsystem->RegisterAssetFactory(FactoryInterface);
			}
		}
	}

	void UnregisterActorFactory()
	{
		if (!SogActorFactory)
		{
			return;
		}

		if (IsEngineExitRequested())
		{
			SogActorFactory = nullptr;
			return;
		}

		if (GEditor)
		{
			if (UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
			{
				TScriptInterface<IAssetFactoryInterface> FactoryInterface(SogActorFactory);
				PlacementSubsystem->UnregisterAssetFactory(FactoryInterface);
			}

			GEditor->ActorFactories.Remove(SogActorFactory);
		}

		SogActorFactory = nullptr;
	}

	EAssetTypeCategories::Type AssetCategory = EAssetTypeCategories::Misc;
	TSharedPtr<IAssetTypeActions> AssetTypeActions;
	USogActorFactory* SogActorFactory = nullptr;
};

IMPLEMENT_MODULE(FSogSplatEditorModule, SogSplatEditor)

#undef LOCTEXT_NAMESPACE
