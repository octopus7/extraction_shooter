#include "TunaSweeperBuildTargetTool.h"

#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Settings/PlatformsMenuSettings.h"
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
	FString ResolveOutputDirectory(ETunaSweeperBuildTarget BuildTarget)
	{
		const TCHAR* StoreDirectory = TEXT("NoStore");
		switch (BuildTarget)
		{
		case ETunaSweeperBuildTarget::SteamFull:
		case ETunaSweeperBuildTarget::SteamDemo:
			StoreDirectory = TEXT("Steam");
			break;
		case ETunaSweeperBuildTarget::StoveFull:
		case ETunaSweeperBuildTarget::StoveDemo:
			StoreDirectory = TEXT("Stove");
			break;
		default:
			break;
		}

		const bool bDemo = BuildTarget == ETunaSweeperBuildTarget::NoStoreDemo ||
			BuildTarget == ETunaSweeperBuildTarget::SteamDemo ||
			BuildTarget == ETunaSweeperBuildTarget::StoveDemo;
		FString OutputDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Builds"),
			StoreDirectory,
			bDemo ? TEXT("Demo") : TEXT("Full"),
			TEXT("Windows")));
		FPaths::NormalizeDirectoryName(OutputDirectory);
		return OutputDirectory;
	}
}

void FTunaSweeperBuildTargetTool::Startup()
{
	if (const UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetDefault<UTunaSweeperBuildTargetSettings>())
	{
		SelectBuildTarget(BuildTargetSettings->BuildTarget);
	}

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
	Section.InsertPosition = FToolMenuInsert(TEXT("DataTools"), EToolMenuInsertType::Before);
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
	FToolMenuSection& DemoSection = Menu->FindOrAddSection(
		TEXT("Demo"),
		LOCTEXT("DemoBuildTargetsSection", "Demo"));
	FToolMenuSection& FullGameSection = Menu->FindOrAddSection(
		TEXT("FullGame"),
		LOCTEXT("FullGameBuildTargetsSection", "Full Game"));

	auto AddTarget = [this](
		FToolMenuSection& Section,
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

	AddTarget(DemoSection, TEXT("NoStoreDemo"), LOCTEXT("NoStoreDemo", "No Store"), LOCTEXT("NoStoreDemoTooltip", "Package the demo without a store integration."), ETunaSweeperBuildTarget::NoStoreDemo);
	AddTarget(DemoSection, TEXT("SteamDemo"), LOCTEXT("SteamDemo", "Steam"), LOCTEXT("SteamDemoTooltip", "Package the demo for Steam."), ETunaSweeperBuildTarget::SteamDemo);
	AddTarget(DemoSection, TEXT("StoveDemo"), LOCTEXT("StoveDemo", "STOVE"), LOCTEXT("StoveDemoTooltip", "Package the demo for STOVE."), ETunaSweeperBuildTarget::StoveDemo);

	AddTarget(FullGameSection, TEXT("NoStoreFull"), LOCTEXT("NoStoreFull", "No Store"), LOCTEXT("NoStoreFullTooltip", "Package the full game without a store integration."), ETunaSweeperBuildTarget::NoStoreFull);
	AddTarget(FullGameSection, TEXT("SteamFull"), LOCTEXT("SteamFull", "Steam"), LOCTEXT("SteamFullTooltip", "Package the full game for Steam."), ETunaSweeperBuildTarget::SteamFull);
	AddTarget(FullGameSection, TEXT("StoveFull"), LOCTEXT("StoveFull", "STOVE"), LOCTEXT("StoveFullTooltip", "Package the full game for STOVE."), ETunaSweeperBuildTarget::StoveFull);
}
void FTunaSweeperBuildTargetTool::SelectBuildTarget(ETunaSweeperBuildTarget BuildTarget) const
{
	UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	if (BuildTargetSettings->BuildTarget != BuildTarget)
	{
		BuildTargetSettings->BuildTarget = BuildTarget;
		BuildTargetSettings->UpdateSinglePropertyInConfigFile(
			BuildTargetSettings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UTunaSweeperBuildTargetSettings, BuildTarget)),
			BuildTargetSettings->GetDefaultConfigFilename());
	}

	UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();
	const FString TargetName = TunaSweeperBuildTargetTool::ResolveTargetName(BuildTarget);
	if (PackagingSettings->BuildTarget != TargetName)
	{
		PackagingSettings->BuildTarget = TargetName;
		PackagingSettings->UpdateSinglePropertyInConfigFile(
			PackagingSettings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UProjectPackagingSettings, BuildTarget)),
			PackagingSettings->GetDefaultConfigFilename());
	}
	UPlatformsMenuSettings* PlatformsMenuSettings = GetMutableDefault<UPlatformsMenuSettings>();
	PlatformsMenuSettings->PackageBuildTarget = TargetName;
	PlatformsMenuSettings->StagingDirectory.Path = TunaSweeperBuildTargetTool::ResolveOutputDirectory(BuildTarget);
	IFileManager::Get().MakeDirectory(*PlatformsMenuSettings->StagingDirectory.Path, true);
	const FString SerializedStagingDirectory = FString::Printf(
		TEXT("(Path=\"%s\")"),
		*PlatformsMenuSettings->StagingDirectory.Path);
	GConfig->SetString(
		TEXT("/Script/DeveloperToolSettings.PlatformsMenuSettings"),
		TEXT("PackageBuildTarget"),
		*PlatformsMenuSettings->PackageBuildTarget,
		GGameIni);
	GConfig->SetString(
		TEXT("/Script/DeveloperToolSettings.PlatformsMenuSettings"),
		TEXT("StagingDirectory"),
		*SerializedStagingDirectory,
		GGameIni);
	GConfig->Flush(false, GGameIni);

}

bool FTunaSweeperBuildTargetTool::IsBuildTargetSelected(ETunaSweeperBuildTarget BuildTarget) const
{
	const UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetDefault<UTunaSweeperBuildTargetSettings>();
	return BuildTargetSettings && BuildTargetSettings->BuildTarget == BuildTarget;
}

#undef LOCTEXT_NAMESPACE
