#include "TunaSweeperEditorSetupShared.h"

#include "Algo/AllOf.h"
#include "Component/TunaSweeperOcclusionRevealComponent.h"
#include "Effect/TunaSweeperOcclusionRevealSettingsDataAsset.h"
#include "Environment/TunaSweeperOcclusionRevealBox.h"
#include "Factories/DataAssetFactory.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialParameterCollectionFactoryNew.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialParameterCollection.h"

namespace TunaSweeperEditorSetup
{
	namespace
	{
		const FString OcclusionAssetPath = TEXT("/Game/Effects");
		const FString OcclusionEnvironmentAssetPath = TEXT("/Game/Environment/Occlusion");
		const FString RevealSettingsAssetName = TEXT("DA_OcclusionRevealSettings");
		const FString RevealCollectionAssetName = TEXT("MPC_OcclusionReveal");
		const FString RevealMaterialAssetName = TEXT("M_OcclusionRevealMasked");
		const FString RevealBoxBlueprintAssetName = TEXT("BP_OcclusionRevealBox");

		FString ObjectPath(const FString& AssetPath, const FString& AssetName)
		{
			return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
		}

		UMaterialExpressionCollectionParameter* AddCollectionParameter(
			UMaterial* Material,
			UMaterialParameterCollection* Collection,
			const FName ParameterName,
			int32 EditorX,
			int32 EditorY)
		{
			UMaterialExpressionCollectionParameter* Expression = NewObject<UMaterialExpressionCollectionParameter>(Material);
			Expression->Material = Material;
			Expression->Collection = Collection;
			Expression->ParameterName = ParameterName;
			Expression->ParameterId = Collection->GetParameterId(ParameterName);
			Expression->ExpressionGUID = FGuid::NewGuid();
			Expression->MaterialExpressionEditorX = EditorX;
			Expression->MaterialExpressionEditorY = EditorY;
			Material->GetExpressionCollection().AddExpression(Expression);
			return Expression;
		}

		void AddScalarParameter(UMaterialParameterCollection* Collection, const FName ParameterName, float DefaultValue)
		{
			FCollectionScalarParameter Parameter;
			Parameter.ParameterName = ParameterName;
			Parameter.DefaultValue = DefaultValue;
			Collection->ScalarParameters.Add(Parameter);
		}

		void AddVectorParameter(UMaterialParameterCollection* Collection, const FName ParameterName)
		{
			FCollectionVectorParameter Parameter;
			Parameter.ParameterName = ParameterName;
			Parameter.DefaultValue = FLinearColor::Black;
			Collection->VectorParameters.Add(Parameter);
		}

		UTunaSweeperOcclusionRevealSettingsDataAsset* EnsureRevealSettingsAsset()
		{
			const FString AssetObjectPath = ObjectPath(OcclusionAssetPath, RevealSettingsAssetName);
			UTunaSweeperOcclusionRevealSettingsDataAsset* Settings = LoadObject<UTunaSweeperOcclusionRevealSettingsDataAsset>(nullptr, *AssetObjectPath);
			if (!Settings)
			{
				UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
				Factory->DataAssetClass = UTunaSweeperOcclusionRevealSettingsDataAsset::StaticClass();
				Settings = Cast<UTunaSweeperOcclusionRevealSettingsDataAsset>(
					FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(
						RevealSettingsAssetName, OcclusionAssetPath, UTunaSweeperOcclusionRevealSettingsDataAsset::StaticClass(), Factory));
			}

			if (!Settings)
			{
				return nullptr;
			}

			Settings->Modify();
			Settings->InnerDiameterCm = 400.0f;
			Settings->OuterDiameterCm = 600.0f;
			Settings->MarkPackageDirty();
			return SaveAsset(Settings) ? Settings : nullptr;
		}

		UMaterialParameterCollection* EnsureRevealParameterCollection()
		{
			const FString AssetObjectPath = ObjectPath(OcclusionAssetPath, RevealCollectionAssetName);
			UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, *AssetObjectPath);
			const TArray<FName> ExpectedVectorParameters = { TEXT("CharacterCenterScreen"), TEXT("CursorCenterScreen") };
			const TArray<FName> ExpectedScalarParameters = {
				TEXT("CharacterValid"),
				TEXT("CursorValid"),
				TEXT("CharacterInnerRadiusScreen"),
				TEXT("CharacterOuterRadiusScreen"),
				TEXT("CursorInnerRadiusScreen"),
				TEXT("CursorOuterRadiusScreen"),
				TEXT("ViewportHeightOverWidth")
			};
			if (!Collection)
			{
				Collection = Cast<UMaterialParameterCollection>(
					FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(
						RevealCollectionAssetName,
						OcclusionAssetPath,
						UMaterialParameterCollection::StaticClass(),
						NewObject<UMaterialParameterCollectionFactoryNew>()));
			}

			if (!Collection)
			{
				return nullptr;
			}

			const bool bHasExpectedParameters =
				Collection->VectorParameters.Num() == ExpectedVectorParameters.Num() &&
				Collection->ScalarParameters.Num() == ExpectedScalarParameters.Num() &&
				Algo::AllOf(ExpectedVectorParameters, [Collection](const FName ParameterName)
				{
					return Collection->GetVectorParameterByName(ParameterName) != nullptr;
				}) &&
				Algo::AllOf(ExpectedScalarParameters, [Collection](const FName ParameterName)
				{
					return Collection->GetScalarParameterByName(ParameterName) != nullptr;
				});
			if (bHasExpectedParameters)
			{
				return Collection;
			}

			Collection->Modify();
			Collection->ScalarParameters.Reset();
			Collection->VectorParameters.Reset();
			AddVectorParameter(Collection, ExpectedVectorParameters[0]);
			AddVectorParameter(Collection, ExpectedVectorParameters[1]);
			AddScalarParameter(Collection, ExpectedScalarParameters[0], 0.0f);
			AddScalarParameter(Collection, ExpectedScalarParameters[1], 0.0f);
			AddScalarParameter(Collection, ExpectedScalarParameters[2], 0.1f);
			AddScalarParameter(Collection, ExpectedScalarParameters[3], 0.15f);
			AddScalarParameter(Collection, ExpectedScalarParameters[4], 0.1f);
			AddScalarParameter(Collection, ExpectedScalarParameters[5], 0.15f);
			AddScalarParameter(Collection, ExpectedScalarParameters[6], 0.5625f);
			Collection->StateId = FGuid::NewGuid();
			Collection->PostEditChange();
			Collection->MarkPackageDirty();
			return SaveAsset(Collection) ? Collection : nullptr;
		}

		UMaterial* EnsureRevealMaterial(UMaterialParameterCollection* Collection)
		{
			const FString AssetObjectPath = ObjectPath(OcclusionAssetPath, RevealMaterialAssetName);
			UMaterial* Material = LoadObject<UMaterial>(nullptr, *AssetObjectPath);
			if (!Material)
			{
				Material = Cast<UMaterial>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(
					RevealMaterialAssetName, OcclusionAssetPath, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
			}

			if (!Material || !Collection)
			{
				return nullptr;
			}

			Material->Modify();
			Material->GetExpressionCollection().Empty();
			Material->TwoSided = true;
			Material->BlendMode = BLEND_Masked;
			Material->OpacityMaskClipValue = 0.5f;
			Material->SetShadingModel(MSM_DefaultLit);

			UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
			if (!EditorOnlyData)
			{
				return nullptr;
			}

			UMaterialExpressionConstant3Vector* BaseColor = NewObject<UMaterialExpressionConstant3Vector>(Material);
			BaseColor->Material = Material;
			BaseColor->Constant = FLinearColor(0.16f, 0.34f, 0.16f, 1.0f);
			BaseColor->MaterialExpressionEditorX = -720;
			Material->GetExpressionCollection().AddExpression(BaseColor);

			UMaterialExpressionScreenPosition* ScreenPosition = NewObject<UMaterialExpressionScreenPosition>(Material);
			ScreenPosition->Material = Material;
			ScreenPosition->MaterialExpressionEditorX = -1040;
			ScreenPosition->MaterialExpressionEditorY = 260;
			Material->GetExpressionCollection().AddExpression(ScreenPosition);

			UMaterialExpressionCollectionParameter* CharacterCenter = AddCollectionParameter(Material, Collection, TEXT("CharacterCenterScreen"), -1040, 410);
			UMaterialExpressionCollectionParameter* CursorCenter = AddCollectionParameter(Material, Collection, TEXT("CursorCenterScreen"), -1040, 540);
			UMaterialExpressionCollectionParameter* CharacterValid = AddCollectionParameter(Material, Collection, TEXT("CharacterValid"), -740, 300);
			UMaterialExpressionCollectionParameter* CursorValid = AddCollectionParameter(Material, Collection, TEXT("CursorValid"), -740, 410);
			UMaterialExpressionCollectionParameter* CharacterInnerRadius = AddCollectionParameter(Material, Collection, TEXT("CharacterInnerRadiusScreen"), -740, 520);
			UMaterialExpressionCollectionParameter* CharacterOuterRadius = AddCollectionParameter(Material, Collection, TEXT("CharacterOuterRadiusScreen"), -740, 630);
			UMaterialExpressionCollectionParameter* CursorInnerRadius = AddCollectionParameter(Material, Collection, TEXT("CursorInnerRadiusScreen"), -450, 520);
			UMaterialExpressionCollectionParameter* CursorOuterRadius = AddCollectionParameter(Material, Collection, TEXT("CursorOuterRadiusScreen"), -450, 630);
			UMaterialExpressionCollectionParameter* ViewportHeightOverWidth = AddCollectionParameter(Material, Collection, TEXT("ViewportHeightOverWidth"), -450, 740);

			UMaterialExpressionCustom* OpacityMask = NewObject<UMaterialExpressionCustom>(Material);
			OpacityMask->Material = Material;
			OpacityMask->Description = TEXT("Screen-space reveal circles projected from world-radius character and cursor locations, with a fine dissolve ring");
			OpacityMask->OutputType = CMOT_Float1;
			OpacityMask->Code =
				TEXT("float2 screenDelta = ScreenUv - CharacterCenter.xy;\n")
				TEXT("float2 screenMetric = float2(1.0f, max(0.0001f, ViewportHeightOverWidth));\n")
				TEXT("float characterDistance = CharacterValid > 0.5f ? length(screenDelta * screenMetric) : 1000000.0f;\n")
				TEXT("float cursorDistance = CursorValid > 0.5f ? length((ScreenUv - CursorCenter.xy) * screenMetric) : 1000000.0f;\n")
				TEXT("float characterDissolve = saturate((characterDistance - CharacterInnerRadiusScreen) / max(0.00001f, CharacterOuterRadiusScreen - CharacterInnerRadiusScreen));\n")
				TEXT("float cursorDissolve = saturate((cursorDistance - CursorInnerRadiusScreen) / max(0.00001f, CursorOuterRadiusScreen - CursorInnerRadiusScreen));\n")
				TEXT("float dissolve = min(characterDissolve, cursorDissolve);\n")
				TEXT("float2 pixel = floor(ScreenUv * 2048.0f);\n")
				TEXT("float noise = frac(sin(dot(pixel, float2(127.1f, 311.7f))) * 43758.5453123f);\n")
				TEXT("return noise <= dissolve ? 1.0f : 0.0f;");
			OpacityMask->MaterialExpressionEditorX = -300;
			OpacityMask->MaterialExpressionEditorY = 450;
			Material->GetExpressionCollection().AddExpression(OpacityMask);

			auto AddInput = [OpacityMask](const TCHAR* Name, UMaterialExpression* Expression, int32 OutputIndex = 0)
			{
				FCustomInput Input;
				Input.InputName = Name;
				Input.Input.Connect(OutputIndex, Expression);
				OpacityMask->Inputs.Add(Input);
			};
			AddInput(TEXT("ScreenUv"), ScreenPosition, 1);
			AddInput(TEXT("CharacterCenter"), CharacterCenter);
			AddInput(TEXT("CursorCenter"), CursorCenter);
			AddInput(TEXT("CharacterValid"), CharacterValid);
			AddInput(TEXT("CursorValid"), CursorValid);
			AddInput(TEXT("CharacterInnerRadiusScreen"), CharacterInnerRadius);
			AddInput(TEXT("CharacterOuterRadiusScreen"), CharacterOuterRadius);
			AddInput(TEXT("CursorInnerRadiusScreen"), CursorInnerRadius);
			AddInput(TEXT("CursorOuterRadiusScreen"), CursorOuterRadius);
			AddInput(TEXT("ViewportHeightOverWidth"), ViewportHeightOverWidth);

			for (UMaterialExpression* Expression : Material->GetExpressions())
			{
				if (const UMaterialExpressionCollectionParameter* CollectionParameter = Cast<UMaterialExpressionCollectionParameter>(Expression);
					CollectionParameter && Collection->GetParameterName(CollectionParameter->ParameterId) != CollectionParameter->ParameterName)
				{
					UE_LOG(LogTunaSweeperEditor, Error, TEXT("Occlusion reveal material parameter binding is invalid: %s"), *CollectionParameter->ParameterName.ToString());
					return nullptr;
				}
			}

			EditorOnlyData->BaseColor.Connect(0, BaseColor);
			EditorOnlyData->OpacityMask.Connect(0, OpacityMask);
			EditorOnlyData->Roughness.UseConstant = true;
			EditorOnlyData->Roughness.Constant = 0.82f;
			EditorOnlyData->Metallic.UseConstant = true;
			EditorOnlyData->Metallic.Constant = 0.0f;
			Material->PostEditChange();
			Material->MarkPackageDirty();
			return SaveAsset(Material) ? Material : nullptr;
		}

		bool AttachSettingsToGameInstance(UTunaSweeperOcclusionRevealSettingsDataAsset* Settings)
		{
			UBlueprint* GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
			if (!GameInstanceBlueprint || !GameInstanceBlueprint->GeneratedClass)
			{
				return false;
			}

			UTunaSweeperGameInstance* Defaults = Cast<UTunaSweeperGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject());
			if (!Defaults)
			{
				return false;
			}

			GameInstanceBlueprint->Modify();
			Defaults->Modify();
			Defaults->OcclusionRevealSettingsDataAsset = TSoftObjectPtr<UTunaSweeperOcclusionRevealSettingsDataAsset>(FSoftObjectPath(Settings));
			GameInstanceBlueprint->MarkPackageDirty();
			FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);
			return SaveAsset(GameInstanceBlueprint);
		}
	}

	bool EnsureOcclusionRevealAssets()
	{
		UTunaSweeperOcclusionRevealSettingsDataAsset* Settings = EnsureRevealSettingsAsset();
		UMaterialParameterCollection* Collection = EnsureRevealParameterCollection();
		UMaterial* Material = EnsureRevealMaterial(Collection);
		UBlueprint* RevealBoxBlueprint = EnsureBlueprint(
			OcclusionEnvironmentAssetPath,
			RevealBoxBlueprintAssetName,
			ATunaSweeperOcclusionRevealBox::StaticClass());

		const bool bSucceeded = Settings && Collection && Material && RevealBoxBlueprint && AttachSettingsToGameInstance(Settings);
		if (bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Log, TEXT("Occlusion reveal assets created. Settings=%s Material=%s Blueprint=%s"),
				*Settings->GetPathName(), *Material->GetPathName(), *RevealBoxBlueprint->GetPathName());
		}
		return bSucceeded;
	}
}
