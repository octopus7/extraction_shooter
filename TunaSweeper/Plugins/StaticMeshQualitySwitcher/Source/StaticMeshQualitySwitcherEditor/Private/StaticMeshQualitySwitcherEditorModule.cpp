#include "Modules/ModuleManager.h"

#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "SStaticMeshQualitySwitcherPanel.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "StaticMeshQualitySwitcherEditorModule"

namespace
{
	const FName StaticMeshQualitySwitcherTabName(TEXT("StaticMeshQualitySwitcher"));
}

class FStaticMeshQualitySwitcherEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			StaticMeshQualitySwitcherTabName,
			FOnSpawnTab::CreateRaw(this, &FStaticMeshQualitySwitcherEditorModule::SpawnSwitcherTab))
			.SetDisplayName(LOCTEXT("TabTitle", "Static Mesh Quality Switcher"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Switch placed Static Mesh Components between original and low-quality mesh pairs."))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FStaticMeshQualitySwitcherEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StaticMeshQualitySwitcherTabName);
	}

private:
	TSharedRef<SDockTab> SpawnSwitcherTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SStaticMeshQualitySwitcherPanel)
			];
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection("Tools");
		Section.AddMenuEntry(
			"OpenStaticMeshQualitySwitcher",
			LOCTEXT("OpenToolLabel", "Static Mesh Quality Switcher"),
			LOCTEXT("OpenToolTooltip", "Open the editor-only static mesh quality switching tool."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"),
			FUIAction(FExecuteAction::CreateRaw(this, &FStaticMeshQualitySwitcherEditorModule::OpenSwitcherTab)));
	}

	void OpenSwitcherTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(StaticMeshQualitySwitcherTabName);
	}
};

IMPLEMENT_MODULE(FStaticMeshQualitySwitcherEditorModule, StaticMeshQualitySwitcherEditor)

#undef LOCTEXT_NAMESPACE
