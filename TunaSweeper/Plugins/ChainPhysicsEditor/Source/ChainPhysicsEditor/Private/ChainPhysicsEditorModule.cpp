#include "Modules/ModuleManager.h"

#include "Framework/Docking/TabManager.h"
#include "SChainPhysicsPanel.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "ChainPhysicsEditorModule"

namespace
{
	const FName ChainPhysicsTabName(TEXT("ChainPhysicsSetup"));
}

class FChainPhysicsEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			ChainPhysicsTabName,
			FOnSpawnTab::CreateRaw(this, &FChainPhysicsEditorModule::SpawnTab))
			.SetDisplayName(LOCTEXT("TabTitle", "Chain Physics Setup"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Detect bone chains and configure their Physics Asset and Anim Blueprint Rigid Body setup."))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "PhysicsAssetEditor.Tabs.Body"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FChainPhysicsEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}
		if (TSharedPtr<SDockTab> LiveTab = FGlobalTabmanager::Get()->FindExistingLiveTab(ChainPhysicsTabName))
		{
			LiveTab->RequestCloseTab();
		}
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ChainPhysicsTabName);
	}

private:
	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs&)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SChainPhysicsPanel)
			];
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
		FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
		if (!MainSection.FindEntry(TEXT("TunaSweeper")))
		{
			FToolMenuEntry& Entry = MainSection.AddSubMenu(
				TEXT("TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
				FNewToolMenuChoice());
			Entry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
		}

		UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
			TEXT("LevelEditor.MainMenu.TunaSweeper"), NAME_None, EMultiBoxType::Menu, false);
		FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
			TEXT("AnimationTools"), LOCTEXT("AnimationTools", "Animation Tools"));
		Section.AddMenuEntry(
			TEXT("OpenChainPhysicsSetup"),
			LOCTEXT("MenuEntry", "Chain Physics Setup"),
			LOCTEXT("MenuEntryTooltip", "Detect secondary-motion chains and configure dedicated PA and AnimBP Rigid Body setup."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "PhysicsAssetEditor.Tabs.Body"),
			FUIAction(FExecuteAction::CreateRaw(this, &FChainPhysicsEditorModule::OpenTab)));
	}

	void OpenTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ChainPhysicsTabName);
	}
};

IMPLEMENT_MODULE(FChainPhysicsEditorModule, ChainPhysicsEditor)

#undef LOCTEXT_NAMESPACE
