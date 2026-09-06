#include "StylizedWaterBodyActor.h"

#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Selection.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "StylizedWaterEditor"
DEFINE_LOG_CATEGORY_STATIC(LogStylizedWaterEditor, Log, All);

namespace StylizedWaterEditor
{
	const FString InternalAssetPath = TEXT("/StylizedWater/Generated/Internal");
	const FString DepthGradientTextureName = TEXT("T_WaterDepthGradient");
	const FString MaterialName = TEXT("M_StylizedWaterSurface");
	const FString MaterialInstanceName = TEXT("MI_StylizedWater_CalmAnime");
	const FString ShoreMaterialName = TEXT("M_StylizedWaterShoreOverlay");
	const FString ShoreMaterialInstanceName = TEXT("MI_StylizedWater_ShoreOverlay");
	const FString BlueprintName = TEXT("BP_StylizedWaterBody_Internal");

	struct FGeneratedAssets
	{
		TObjectPtr<UTexture2D> DepthGradientTexture;
		TObjectPtr<UMaterial> Material;
		TObjectPtr<UMaterialInstanceConstant> MaterialInstance;
		TObjectPtr<UMaterial> ShoreMaterial;
		TObjectPtr<UMaterialInstanceConstant> ShoreMaterialInstance;
		TObjectPtr<UBlueprint> Blueprint;

		bool IsComplete() const
		{
			return DepthGradientTexture && Material && MaterialInstance && ShoreMaterial && ShoreMaterialInstance && Blueprint && Blueprint->GeneratedClass;
		}
	};

	FString ObjectPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
	}

	FGeneratedAssets LoadPluginAssets()
	{
		FGeneratedAssets Assets;
		Assets.DepthGradientTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath(InternalAssetPath, DepthGradientTextureName));
		Assets.Material = LoadObject<UMaterial>(nullptr, *ObjectPath(InternalAssetPath, MaterialName));
		Assets.MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *ObjectPath(InternalAssetPath, MaterialInstanceName));
		Assets.ShoreMaterial = LoadObject<UMaterial>(nullptr, *ObjectPath(InternalAssetPath, ShoreMaterialName));
		Assets.ShoreMaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *ObjectPath(InternalAssetPath, ShoreMaterialInstanceName));
		Assets.Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath(InternalAssetPath, BlueprintName));
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
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FStylizedWaterEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}
	}

private:
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

		StylizedWaterEditor::FGeneratedAssets Assets = StylizedWaterEditor::LoadPluginAssets();
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
		WaterBody->SetTemplateShoreMaterialInstance(Assets.ShoreMaterialInstance);
		WaterBody->ApplyPreset(Preset, false);
		WaterBody->FinishSpawning(SpawnTransform);
		WaterBody->MarkPackageDirty();

		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(WaterBody, true, true, true);
	}

};

IMPLEMENT_MODULE(FStylizedWaterEditorModule, StylizedWaterEditor)

#undef LOCTEXT_NAMESPACE
