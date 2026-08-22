#include "TunaSweeperBuildTargetTool.h"

#include "Settings/ProjectPackagingSettings.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "TunaSweeperBuildTargetTool"

namespace TunaSweeperBuildTargetTool
{
	void EnsureTunaSweeperTopMenu()
	{
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
	}

	const TCHAR* ResolveTargetName(ETunaSweeperBuildTarget BuildTarget)
	{
		switch (BuildTarget)
		{
		case ETunaSweeperBuildTarget::NoStoreDemo: return TEXT("TunaSweeperNoStoreDemo");
		case ETunaSweeperBuildTarget::SteamFull: return TEXT("TunaSweeper");
		case ETunaSweeperBuildTarget::SteamDemo: return TEXT("TunaSweeperDemo");
		case ETunaSweeperBuildTarget::StoveFull: return TEXT("TunaSweeperStove");
		case ETunaSweeperBuildTarget::StoveDemo: return TEXT("TunaSweeperStoveDemo");
		case ETunaSweeperBuildTarget::NoStoreFull:
		default: return TEXT("TunaSweeperNoStore");
		}
	}
}

void FTunaSweeperBuildTargetTool::Startup()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperBuildTargetTool::RegisterMenus));
}

void FTunaSweeperBuildTargetTool::Shutdown()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}
}

void FTunaSweeperBuildTargetTool::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	TunaSweeperBuildTargetTool::EnsureTunaSweeperTopMenu();

	UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
		TEXT("LevelEditor.MainMenu.TunaSweeper"),
		NAME_None,
		EMultiBoxType::Menu,
		false);
	FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
		TEXT("Build"),
		LOCTEXT("BuildMenuSection", "Build"));
	Section.AddSubMenu(
		TEXT("BuildTarget"),
		LOCTEXT("BuildTargetSubMenu", "Build Target"),
		LOCTEXT("BuildTargetSubMenuTooltip", "Select the packaging target and editor preview configuration."),
		FNewToolMenuChoice(FNewToolMenuDelegate::CreateRaw(this, &FTunaSweeperBuildTargetTool::PopulateBuildTargetMenu)));

	if (const UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetDefault<UTunaSweeperBuildTargetSettings>())
	{
		SelectBuildTarget(BuildTargetSettings->BuildTarget);
	}
}

void FTunaSweeperBuildTargetTool::PopulateBuildTargetMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("BuildTarget"));
	auto AddTarget = [this, &Section](
		const FName EntryName,
		const FText& Label,
		const FText& Tooltip,
		ETunaSweeperBuildTarget BuildTarget)
	{
		Section.AddMenuEntry(
			EntryName,
			Label,
			Tooltip,
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FTunaSweeperBuildTargetTool::SelectBuildTarget, BuildTarget),
				FCanExecuteAction(),
				FIsActionChecked::CreateRaw(this, &FTunaSweeperBuildTargetTool::IsBuildTargetSelected, BuildTarget)),
			EUserInterfaceActionType::RadioButton);
	};

	AddTarget(TEXT("NoStoreFull"), LOCTEXT("NoStoreFull", "No Store - Full Game"), LOCTEXT("NoStoreFullTooltip", "Package the full game without a store integration."), ETunaSweeperBuildTarget::NoStoreFull);
	AddTarget(TEXT("NoStoreDemo"), LOCTEXT("NoStoreDemo", "No Store - Demo"), LOCTEXT("NoStoreDemoTooltip", "Package the demo without a store integration."), ETunaSweeperBuildTarget::NoStoreDemo);
	AddTarget(TEXT("SteamFull"), LOCTEXT("SteamFull", "Steam - Full Game"), LOCTEXT("SteamFullTooltip", "Package the full game for Steam."), ETunaSweeperBuildTarget::SteamFull);
	AddTarget(TEXT("SteamDemo"), LOCTEXT("SteamDemo", "Steam - Demo"), LOCTEXT("SteamDemoTooltip", "Package the demo for Steam."), ETunaSweeperBuildTarget::SteamDemo);
	AddTarget(TEXT("StoveFull"), LOCTEXT("StoveFull", "STOVE - Full Game"), LOCTEXT("StoveFullTooltip", "Package the full game for STOVE."), ETunaSweeperBuildTarget::StoveFull);
	AddTarget(TEXT("StoveDemo"), LOCTEXT("StoveDemo", "STOVE - Demo"), LOCTEXT("StoveDemoTooltip", "Package the demo for STOVE."), ETunaSweeperBuildTarget::StoveDemo);
}

void FTunaSweeperBuildTargetTool::SelectBuildTarget(ETunaSweeperBuildTarget BuildTarget) const
{
	UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	BuildTargetSettings->BuildTarget = BuildTarget;
	BuildTargetSettings->TryUpdateDefaultConfigFile();

	UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();
	PackagingSettings->BuildTarget = TunaSweeperBuildTargetTool::ResolveTargetName(BuildTarget);
	PackagingSettings->TryUpdateDefaultConfigFile();
}

bool FTunaSweeperBuildTargetTool::IsBuildTargetSelected(ETunaSweeperBuildTarget BuildTarget) const
{
	const UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetDefault<UTunaSweeperBuildTargetSettings>();
	return BuildTargetSettings && BuildTargetSettings->BuildTarget == BuildTarget;
}

#undef LOCTEXT_NAMESPACE
