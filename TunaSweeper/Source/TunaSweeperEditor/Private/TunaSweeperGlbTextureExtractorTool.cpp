#include "TunaSweeperGlbTextureExtractorTool.h"

#include "HAL/PlatformProcess.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "TunaSweeperGlbTextureExtractorTool"

namespace TunaSweeperGlbTextureExtractorTool
{
	FString GetExecutablePath()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("Tools"),
			TEXT("GlbTextureExtractor"),
			TEXT("bin"),
			TEXT("Release"),
			TEXT("net10.0-windows"),
			TEXT("GlbTextureExtractor.exe")));
	}
}

void FTunaSweeperGlbTextureExtractorTool::Startup()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperGlbTextureExtractorTool::RegisterMenus));
}

void FTunaSweeperGlbTextureExtractorTool::Shutdown()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}
}

void FTunaSweeperGlbTextureExtractorTool::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
		TEXT("LevelEditor.MainMenu.TunaSweeper"),
		NAME_None,
		EMultiBoxType::Menu,
		false);
	FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
		TEXT("AssetTools"),
		LOCTEXT("TunaSweeperAssetToolsMenuSection", "Asset Tools"));
	Section.AddMenuEntry(
		TEXT("OpenGlbTextureExtractor"),
		LOCTEXT("OpenGlbTextureExtractor", "GLB Texture Extractor"),
		LOCTEXT("OpenGlbTextureExtractorTooltip", "Open the standalone GLB Texture Extractor release tool."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperGlbTextureExtractorTool::OpenGlbTextureExtractor)));
}

void FTunaSweeperGlbTextureExtractorTool::OpenGlbTextureExtractor() const
{
	const FString ExecutablePath = TunaSweeperGlbTextureExtractorTool::GetExecutablePath();
	if (!FPaths::FileExists(ExecutablePath))
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("MissingExecutableDialog", "Could not find the GLB Texture Extractor release executable:\n{0}"),
				FText::FromString(ExecutablePath)));
		return;
	}

	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
		*ExecutablePath,
		nullptr,
		true,
		false,
		false,
		nullptr,
		0,
		*FPaths::GetPath(ExecutablePath),
		nullptr);
	if (!ProcessHandle.IsValid())
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("LaunchFailedDialog", "Could not launch the GLB Texture Extractor:\n{0}"),
				FText::FromString(ExecutablePath)));
		return;
	}

	FPlatformProcess::CloseProc(ProcessHandle);
}

#undef LOCTEXT_NAMESPACE
