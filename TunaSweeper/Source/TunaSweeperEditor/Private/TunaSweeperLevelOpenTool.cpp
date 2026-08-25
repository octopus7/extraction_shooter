#include "TunaSweeperLevelOpenTool.h"

#include "FileHelpers.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "TunaSweeperLevelOpenTool"

namespace TunaSweeperLevelOpenTool
{
	const TCHAR* IntroMapPackagePath = TEXT("/Game/Maps/IntroMap");
	const TCHAR* BunkerMapPackagePath = TEXT("/Game/Maps/BunkerMap");

	FString GetRaidMapPackagePath()
	{
		const FString RaidLevelName = TunaSweeperBuildFlavor::GetRaidGameplayLevelName().ToString();
		return RaidLevelName.StartsWith(TEXT("/"))
			? RaidLevelName
			: FString::Printf(TEXT("/Game/Maps/%s"), *RaidLevelName);
	}

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
}

void FTunaSweeperLevelOpenTool::Startup()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperLevelOpenTool::RegisterMenus));
}

void FTunaSweeperLevelOpenTool::Shutdown()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}
}

void FTunaSweeperLevelOpenTool::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	TunaSweeperLevelOpenTool::EnsureTunaSweeperTopMenu();

	UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
		TEXT("LevelEditor.MainMenu.TunaSweeper"),
		NAME_None,
		EMultiBoxType::Menu,
		false);
	FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
		TEXT("Levels"),
		LOCTEXT("TunaSweeperLevelsMenuSection", "Levels"));
	Section.AddSubMenu(
		TEXT("OpenLevel"),
		LOCTEXT("OpenLevelSubMenu", "Open Level"),
		LOCTEXT("OpenLevelSubMenuTooltip", "Open a TunaSweeper level in the editor."),
		FNewToolMenuChoice(FNewToolMenuDelegate::CreateRaw(this, &FTunaSweeperLevelOpenTool::PopulateOpenLevelMenu)));
}

void FTunaSweeperLevelOpenTool::PopulateOpenLevelMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("Levels"));
	Section.AddMenuEntry(
		TEXT("OpenIntroLevel"),
		LOCTEXT("OpenIntroLevel", "Intro"),
		LOCTEXT("OpenIntroLevelTooltip", "Open the Intro level."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperLevelOpenTool::OpenLevel, FString(TunaSweeperLevelOpenTool::IntroMapPackagePath))));
	Section.AddMenuEntry(
		TEXT("OpenRaidLevel"),
		LOCTEXT("OpenRaidLevel", "Raid"),
		LOCTEXT("OpenRaidLevelTooltip", "Open the Raid level."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperLevelOpenTool::OpenLevel, TunaSweeperLevelOpenTool::GetRaidMapPackagePath())));
	Section.AddMenuEntry(
		TEXT("OpenBunkerLevel"),
		LOCTEXT("OpenBunkerLevel", "Bunker"),
		LOCTEXT("OpenBunkerLevelTooltip", "Open the Bunker level."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperLevelOpenTool::OpenLevel, FString(TunaSweeperLevelOpenTool::BunkerMapPackagePath))));
}

void FTunaSweeperLevelOpenTool::OpenLevel(FString MapPackagePath) const
{
	const FString MapFilename = FPackageName::LongPackageNameToFilename(
		MapPackagePath,
		FPackageName::GetMapPackageExtension());
	if (!FPaths::FileExists(MapFilename))
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("MissingLevelDialog", "Could not find level file:\n{0}"),
				FText::FromString(MapFilename)));
		return;
	}

	UEditorLoadingAndSavingUtils::LoadMap(MapFilename);
}

#undef LOCTEXT_NAMESPACE
