#include "TunaSweeperLevelOpenTool.h"

#include "FileHelpers.h"
#include "Engine/GameInstance.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"
#include "Settings/TunaSweeperLanguage.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "ToolMenus.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "TunaSweeperLevelOpenTool"

namespace TunaSweeperLevelOpenTool
{
	const TCHAR* IntroMapPackagePath = TEXT("/Game/Maps/IntroMap");
	const TCHAR* BunkerMapPackagePath = TEXT("/Game/Maps/BunkerMap");

	FText MenuText(const TCHAR* Key)
	{
		static TStrongObjectPtr<UGameInstance> TextOwner(NewObject<UGameInstance>());
		static TStrongObjectPtr<UTunaSweeperTextSubsystem> TextData(NewObject<UTunaSweeperTextSubsystem>(TextOwner.Get()));
		ETunaSweeperItemTextLanguage Language = ETunaSweeperItemTextLanguage::English;
		TunaSweeperLanguage::TryParseLanguageCode(
			FInternationalization::Get().GetCurrentCulture()->GetName(), Language);
		return TextData->ResolveText(FName(Key), Language, FText::GetEmpty());
	}

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
	FToolMenuEntry& UseBoxRaidLevelEntry = Section.AddMenuEntry(
		TEXT("UseBoxRaidLevel"),
		LOCTEXT("UseBoxRaidLevel", "Use Box Raid Level"),
		LOCTEXT("UseBoxRaidLevelTooltip", "Use DemoBoxRaidMap instead of DemoRaidMap for demo targets."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTunaSweeperLevelOpenTool::ToggleUseBoxRaidLevel),
			FCanExecuteAction::CreateRaw(this, &FTunaSweeperLevelOpenTool::CanToggleUseBoxRaidLevel),
			FIsActionChecked::CreateRaw(this, &FTunaSweeperLevelOpenTool::IsUsingBoxRaidLevel)),
		EUserInterfaceActionType::ToggleButton);
	UseBoxRaidLevelEntry.InsertPosition = FToolMenuInsert(
		TEXT("OpenLevel"),
		EToolMenuInsertType::After);
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
	const auto AddLevelEntry = [this, &Section](FName EntryName, const TCHAR* LabelKey, const FString& PackagePath)
	{
		const FText Label = TunaSweeperLevelOpenTool::MenuText(LabelKey);
		Section.AddMenuEntry(EntryName, Label,
			FText::Format(TunaSweeperLevelOpenTool::MenuText(TEXT("ui.editor.open_level.tooltip")), Label),
			FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperLevelOpenTool::OpenLevel, PackagePath)));
	};
	AddLevelEntry(TEXT("OpenRaidLevel"), TEXT("ui.editor.open_level.raid_test"), TunaSweeperLevelOpenTool::GetRaidMapPackagePath());
	AddLevelEntry(TEXT("OpenRaidForestLevel"), TEXT("ui.editor.open_level.raid_forest"), TEXT("/Game/MainRaid/RaidForest"));
	AddLevelEntry(TEXT("OpenRaidVillageLevel"), TEXT("ui.editor.open_level.raid_village"), TEXT("/Game/MainRaid/RaidVillage"));
	AddLevelEntry(TEXT("OpenRaidPlainsLevel"), TEXT("ui.editor.open_level.raid_plains"), TEXT("/Game/MainRaid/RaidPlains"));
	Section.AddMenuEntry(
		TEXT("OpenBunkerLevel"),
		LOCTEXT("OpenBunkerLevel", "Bunker"),
		LOCTEXT("OpenBunkerLevelTooltip", "Open the Bunker level."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperLevelOpenTool::OpenLevel, FString(TunaSweeperLevelOpenTool::BunkerMapPackagePath))));
	AddLevelEntry(TEXT("OpenBasementLevel"), TEXT("ui.editor.open_level.basement"), TEXT("/Game/Environment/Basement/Maps/L_Basement"));
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

void FTunaSweeperLevelOpenTool::ToggleUseBoxRaidLevel()
{
	if (!CanToggleUseBoxRaidLevel())
	{
		return;
	}

	UTunaSweeperBuildTargetSettings* Settings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	Settings->bUseBoxRaidLevel = !Settings->bUseBoxRaidLevel;
	Settings->UpdateSinglePropertyInConfigFile(
		Settings->GetClass()->FindPropertyByName(
			GET_MEMBER_NAME_CHECKED(UTunaSweeperBuildTargetSettings, bUseBoxRaidLevel)),
		Settings->GetDefaultConfigFilename());
}

bool FTunaSweeperLevelOpenTool::CanToggleUseBoxRaidLevel() const
{
	return TunaSweeperBuildFlavor::IsDemo();
}

bool FTunaSweeperLevelOpenTool::IsUsingBoxRaidLevel() const
{
	const UTunaSweeperBuildTargetSettings* Settings = GetDefault<UTunaSweeperBuildTargetSettings>();
	return TunaSweeperBuildFlavor::IsDemo() && Settings && Settings->bUseBoxRaidLevel;
}

#undef LOCTEXT_NAMESPACE
