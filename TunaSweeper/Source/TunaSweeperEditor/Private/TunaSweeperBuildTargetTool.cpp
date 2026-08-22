#include "TunaSweeperBuildTargetTool.h"

#include "Async/Async.h"
#include "FileHelpers.h"
#include "HAL/PlatformProcess.h"
#include "IUATHelperModule.h"
#include "Misc/App.h"
#include "Misc/MessageDialog.h"
#include "Misc/MonitoredProcess.h"
#include "Styling/AppStyle.h"
#include "UnrealEdMisc.h"
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

	const TCHAR* ResolveBuildConfigurationName(EProjectPackagingBuildConfigurations BuildConfiguration)
	{
		switch (BuildConfiguration)
		{
		case EProjectPackagingBuildConfigurations::PPBC_Debug: return TEXT("Debug");
		case EProjectPackagingBuildConfigurations::PPBC_DebugGame: return TEXT("DebugGame");
		case EProjectPackagingBuildConfigurations::PPBC_Test: return TEXT("Test");
		case EProjectPackagingBuildConfigurations::PPBC_Shipping: return TEXT("Shipping");
		case EProjectPackagingBuildConfigurations::PPBC_Development:
		default: return TEXT("Development");
		}
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
	FToolMenuSection& PackagingSection = Menu->FindOrAddSection(TEXT("Packaging"));
	PackagingSection.AddMenuEntry(
		TEXT("Packaging"),
		LOCTEXT("PackagingToggle", "Packaging"),
		LOCTEXT("PackagingToggleTooltip", "Package immediately after selecting a build target."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTunaSweeperBuildTargetTool::TogglePackaging),
			FCanExecuteAction::CreateRaw(this, &FTunaSweeperBuildTargetTool::CanTogglePackaging),
			FIsActionChecked::CreateRaw(this, &FTunaSweeperBuildTargetTool::IsPackagingEnabled)),
		EUserInterfaceActionType::ToggleButton);
	PackagingSection.AddMenuEntry(
		TEXT("Run"),
		LOCTEXT("RunToggle", "Run"),
		LOCTEXT("RunToggleTooltip", "Run the packaged game after packaging succeeds."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTunaSweeperBuildTargetTool::ToggleRun),
			FCanExecuteAction::CreateRaw(this, &FTunaSweeperBuildTargetTool::CanToggleRun),
			FIsActionChecked::CreateRaw(this, &FTunaSweeperBuildTargetTool::IsRunEnabled)),
		EUserInterfaceActionType::ToggleButton);

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
				FCanExecuteAction::CreateRaw(this, &FTunaSweeperBuildTargetTool::CanSelectBuildTarget),
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
void FTunaSweeperBuildTargetTool::SelectBuildTarget(ETunaSweeperBuildTarget BuildTarget)
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

	if (bPackagingEnabled)
	{
		StartPackaging(BuildTarget);
	}

}

bool FTunaSweeperBuildTargetTool::IsBuildTargetSelected(ETunaSweeperBuildTarget BuildTarget) const
{
	const UTunaSweeperBuildTargetSettings* BuildTargetSettings = GetDefault<UTunaSweeperBuildTargetSettings>();
	return BuildTargetSettings && BuildTargetSettings->BuildTarget == BuildTarget;
}

bool FTunaSweeperBuildTargetTool::CanSelectBuildTarget() const
{
	return !bPackagingInProgress;
}

void FTunaSweeperBuildTargetTool::TogglePackaging()
{
	if (bPackagingInProgress)
	{
		return;
	}

	bPackagingEnabled = !bPackagingEnabled;
	if (!bPackagingEnabled)
	{
		bRunEnabled = false;
	}
}

bool FTunaSweeperBuildTargetTool::IsPackagingEnabled() const
{
	return bPackagingEnabled;
}

bool FTunaSweeperBuildTargetTool::CanTogglePackaging() const
{
	return !bPackagingInProgress;
}

void FTunaSweeperBuildTargetTool::ToggleRun()
{
	if (CanToggleRun())
	{
		bRunEnabled = !bRunEnabled;
	}
}

bool FTunaSweeperBuildTargetTool::IsRunEnabled() const
{
	return bRunEnabled;
}

bool FTunaSweeperBuildTargetTool::CanToggleRun() const
{
	return bPackagingEnabled && !bPackagingInProgress;
}

void FTunaSweeperBuildTargetTool::StartPackaging(ETunaSweeperBuildTarget BuildTarget)
{
	if (bPackagingInProgress)
	{
		return;
	}

	const FString UatPath = FSerializedUATProcess::GetUATPath();
	if (!FPaths::FileExists(UatPath))
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("MissingUatDialog", "Could not find Unreal AutomationTool:\n{0}"),
				FText::FromString(UatPath)));
		return;
	}

	FEditorFileUtils::SaveDirtyPackages(
		false,
		true,
		true,
		false,
		false,
		false);

	const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();
	EProjectPackagingBuildConfigurations BuildConfiguration = PackagingSettings->ForDistribution
		? EProjectPackagingBuildConfigurations::PPBC_Shipping
		: PackagingSettings->BuildConfiguration;
	const FString BuildConfigurationName = TunaSweeperBuildTargetTool::ResolveBuildConfigurationName(BuildConfiguration);
	const FString TargetName = TunaSweeperBuildTargetTool::ResolveTargetName(BuildTarget);
	const FString OutputDirectory = TunaSweeperBuildTargetTool::ResolveOutputDirectory(BuildTarget);
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	const FString CommandletExecutable = FUnrealEdMisc::Get().GetExecutableForCommandlets();
	const FString PackagedExecutable = FPaths::Combine(OutputDirectory, TEXT("TunaSweeper.exe"));

	IFileManager::Get().MakeDirectory(*OutputDirectory, true);

	FString PackageOptions(TEXT("-stage -archive -package -build"));
	if (PackagingSettings->FullRebuild)
	{
		PackageOptions += TEXT(" -clean");
	}
	if (PackagingSettings->UsePakFile || PackagingSettings->bUseIoStore)
	{
		PackageOptions += TEXT(" -pak");
		if (PackagingSettings->bUseIoStore)
		{
			PackageOptions += TEXT(" -iostore");
		}
		if (PackagingSettings->bCompressed)
		{
			PackageOptions += TEXT(" -compressed");
		}
	}
	if (PackagingSettings->IncludePrerequisites)
	{
		PackageOptions += TEXT(" -prereqs");
	}
	if (PackagingSettings->bSkipEditorContent)
	{
		PackageOptions += TEXT(" -SkipCookingEditorContent");
	}
	if (PackagingSettings->ForDistribution)
	{
		PackageOptions += TEXT(" -distribution");
	}
	if (PackagingSettings->bGenerateChunks)
	{
		PackageOptions += TEXT(" -manifests");
	}
	if (BuildConfiguration == EProjectPackagingBuildConfigurations::PPBC_Shipping && !PackagingSettings->IncludeDebugFiles)
	{
		PackageOptions += TEXT(" -nodebuginfo");
	}

	const FString InstalledOption = FApp::IsEngineInstalled() ? TEXT(" -installed") : FString();
	const FString CommandLine = FString::Printf(
		TEXT("-ScriptsForProject=\"%s\" BuildCookRun -nop4 -utf8output -nocompileeditor -skipbuildeditor -cook -project=\"%s\" -target=%s -unrealexe=\"%s\" -platform=Win64%s -SkipCookingErrorSummary -JsonStdOut %s -archivedirectory=\"%s\" -clientconfig=%s"),
		*ProjectPath,
		*ProjectPath,
		*TargetName,
		*CommandletExecutable,
		*InstalledOption,
		*PackageOptions,
		*OutputDirectory,
		*BuildConfigurationName);

	bPackagingInProgress = true;
	const bool bRunAfterPackaging = bRunEnabled;
	IUATHelperModule::Get().CreateUatTask(
		CommandLine,
		LOCTEXT("WindowsPlatform", "Windows"),
		FText::Format(LOCTEXT("PackagingTargetTask", "Packaging {0}"), FText::FromString(TargetName)),
		LOCTEXT("PackagingTask", "Packaging"),
		FAppStyle::GetBrush(TEXT("MainFrame.PackageProject")),
		nullptr,
		[this, bRunAfterPackaging, PackagedExecutable](FString Result, double)
		{
			AsyncTask(ENamedThreads::GameThread, [this, bRunAfterPackaging, PackagedExecutable, Result]()
			{
				bPackagingInProgress = false;
				if (bRunAfterPackaging && Result == TEXT("Completed"))
				{
					LaunchPackagedBuild(PackagedExecutable);
				}
			});
		},
		OutputDirectory);
}

void FTunaSweeperBuildTargetTool::LaunchPackagedBuild(const FString& ExecutablePath) const
{
	if (!FPaths::FileExists(ExecutablePath))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("MissingPackagedExecutableDialog", "Packaging completed, but the executable was not found:\n{0}"),
			FText::FromString(ExecutablePath)));
		return;
	}

	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
		*ExecutablePath, TEXT(""), true, false, false, nullptr, 0, *FPaths::GetPath(ExecutablePath), nullptr);
	if (!ProcessHandle.IsValid())
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("PackagedExecutableLaunchFailedDialog", "Could not run the packaged game:\n{0}"),
			FText::FromString(ExecutablePath)));
		return;
	}

	FPlatformProcess::CloseProc(ProcessHandle);
}

#undef LOCTEXT_NAMESPACE
