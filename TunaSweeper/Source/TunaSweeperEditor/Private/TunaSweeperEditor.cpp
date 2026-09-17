#include "TunaSweeperEditorAssetImport.h"
#include "TunaSweeperBuildTargetTool.h"
#include "TunaSweeperEnemyAIDebugTool.h"
#include "TunaSweeperFMSoundTool.h"
#include "TunaSweeperGlbTextureExtractorTool.h"
#include "TunaSweeperLevelOpenTool.h"
#include "TunaSweeperMapCaptureActorDetails.h"
#include "TunaSweeperTitleCreditsRemoval.h"

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"

class FTunaSweeperEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRemoveTitleCredits")))
		{
			const bool bSucceeded = TunaSweeperTitleCreditsRemoval::Run();
			if (!bSucceeded)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to remove title credits widgets."));
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("Removed title credits widgets."));
			}
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([bSucceeded](float)
			{
				FPlatformMisc::RequestExitWithStatus(
					false,
					bSucceeded ? 0 : 1,
					TEXT("TunaSweeperRemoveTitleCredits"));
				return false;
			}));
			return;
		}

		if (IsRunningCommandlet())
		{
			return;
		}

		bStandardEditorSetupStarted = true;
		TunaSweeperMapCaptureActorDetails::Register();

		FString UiTextureImportSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureSource="), UiTextureImportSource))
		{
			TunaSweeperEditorAssetImport::ImportUiTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureQuit")))
			{
				return;
			}
		}

		FString AudioImportSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioSource="), AudioImportSource))
		{
			TunaSweeperEditorAssetImport::ImportAudioFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioQuit")))
			{
				return;
			}
		}

		BuildTargetTool = MakeUnique<FTunaSweeperBuildTargetTool>();
		BuildTargetTool->Startup();

		LevelOpenTool = MakeUnique<FTunaSweeperLevelOpenTool>();
		LevelOpenTool->Startup();

		EnemyAIDebugTool = MakeUnique<FTunaSweeperEnemyAIDebugTool>();
		EnemyAIDebugTool->Startup();

		FMSoundTool = MakeUnique<FTunaSweeperFMSoundTool>();
		FMSoundTool->Startup();

		GlbTextureExtractorTool = MakeUnique<FTunaSweeperGlbTextureExtractorTool>();
		GlbTextureExtractorTool->Startup();

	}

	virtual void ShutdownModule() override
	{
		if (bStandardEditorSetupStarted)
		{
			TunaSweeperMapCaptureActorDetails::Unregister();
		}

		if (BuildTargetTool)
		{
			BuildTargetTool->Shutdown();
			BuildTargetTool.Reset();
		}

		if (LevelOpenTool)
		{
			LevelOpenTool->Shutdown();
			LevelOpenTool.Reset();
		}

		if (FMSoundTool)
		{
			FMSoundTool->Shutdown();
			FMSoundTool.Reset();
		}

		if (EnemyAIDebugTool)
		{
			EnemyAIDebugTool->Shutdown();
			EnemyAIDebugTool.Reset();
		}

		if (GlbTextureExtractorTool)
		{
			GlbTextureExtractorTool->Shutdown();
			GlbTextureExtractorTool.Reset();
		}
	}

private:
	bool bStandardEditorSetupStarted = false;
	TUniquePtr<FTunaSweeperBuildTargetTool> BuildTargetTool;
	TUniquePtr<FTunaSweeperLevelOpenTool> LevelOpenTool;
	TUniquePtr<FTunaSweeperEnemyAIDebugTool> EnemyAIDebugTool;
	TUniquePtr<FTunaSweeperFMSoundTool> FMSoundTool;
	TUniquePtr<FTunaSweeperGlbTextureExtractorTool> GlbTextureExtractorTool;
};

IMPLEMENT_MODULE(FTunaSweeperEditorModule, TunaSweeperEditor)
