#include "TunaSweeperEditorSetupShared.h"

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
#include "Materials/MaterialExpressionWorldPosition.h"
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

			Collection->Modify();
			Collection->ScalarParameters.Reset();
			Collection->VectorParameters.Reset();
			AddVectorParameter(Collection, TEXT("CharacterCenter"));
			AddVectorParameter(Collection, TEXT("CursorCenter"));
			AddScalarParameter(Collection, TEXT("CursorValid"), 0.0f);
			AddScalarParameter(Collection, TEXT("InnerRadiusCm"), 200.0f);
			AddScalarParameter(Collection, TEXT("OuterRadiusCm"), 300.0f);
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

			UMaterialExpressionWorldPosition* WorldPosition = NewObject<UMaterialExpressionWorldPosition>(Material);
			WorldPosition->Material = Material;
			WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
			WorldPosition->MaterialExpressionEditorX = -1040;
			WorldPosition->MaterialExpressionEditorY = 260;
			Material->GetExpressionCollection().AddExpression(WorldPosition);

			UMaterialExpressionCollectionParameter* CharacterCenter = AddCollectionParameter(Material, Collection, TEXT("CharacterCenter"), -1040, 410);
			UMaterialExpressionCollectionParameter* CursorCenter = AddCollectionParameter(Material, Collection, TEXT("CursorCenter"), -1040, 540);
			UMaterialExpressionCollectionParameter* CursorValid = AddCollectionParameter(Material, Collection, TEXT("CursorValid"), -740, 410);
			UMaterialExpressionCollectionParameter* InnerRadius = AddCollectionParameter(Material, Collection, TEXT("InnerRadiusCm"), -740, 540);
			UMaterialExpressionCollectionParameter* OuterRadius = AddCollectionParameter(Material, Collection, TEXT("OuterRadiusCm"), -740, 670);

			UMaterialExpressionCustom* OpacityMask = NewObject<UMaterialExpressionCustom>(Material);
			OpacityMask->Material = Material;
			OpacityMask->Description = TEXT("World-space character and cursor reveal with a fully removed center and dither dissolve ring");
			OpacityMask->OutputType = CMOT_Float1;
			OpacityMask->Code =
				TEXT("float2 worldXY = WorldPos.xy;\n")
				TEXT("float characterDistance = length(worldXY - CharacterCenter.xy);\n")
				TEXT("float cursorDistance = CursorValid > 0.5f ? length(worldXY - CursorCenter.xy) : 1000000.0f;\n")
				TEXT("float distanceToReveal = min(characterDistance, cursorDistance);\n")
				TEXT("float innerRadius = max(0.0f, InnerRadiusCm);\n")
				TEXT("float outerRadius = max(innerRadius + 1.0f, OuterRadiusCm);\n")
				TEXT("float dissolve = saturate((distanceToReveal - innerRadius) / (outerRadius - innerRadius));\n")
				TEXT("float2 cell = floor(worldXY / 7.0f);\n")
				TEXT("float noise = frac(sin(dot(cell, float2(12.9898f, 78.233f))) * 43758.5453f);\n")
				TEXT("return distanceToReveal <= innerRadius ? 0.0f : (noise <= dissolve ? 1.0f : 0.0f);");
			OpacityMask->MaterialExpressionEditorX = -300;
			OpacityMask->MaterialExpressionEditorY = 450;
			Material->GetExpressionCollection().AddExpression(OpacityMask);

			auto AddInput = [OpacityMask](const TCHAR* Name, UMaterialExpression* Expression)
			{
				FCustomInput Input;
				Input.InputName = Name;
				Input.Input.Connect(0, Expression);
				OpacityMask->Inputs.Add(Input);
			};
			AddInput(TEXT("WorldPos"), WorldPosition);
			AddInput(TEXT("CharacterCenter"), CharacterCenter);
			AddInput(TEXT("CursorCenter"), CursorCenter);
			AddInput(TEXT("CursorValid"), CursorValid);
			AddInput(TEXT("InnerRadiusCm"), InnerRadius);
			AddInput(TEXT("OuterRadiusCm"), OuterRadius);

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
