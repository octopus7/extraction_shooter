#include "TunaSweeperEditorSetupShared.h"

DEFINE_LOG_CATEGORY(LogTunaSweeperEditor);

class FTunaSweeperEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (IsRunningCommandlet())
		{
			return;
		}

		TunaSweeperMapCaptureActorDetails::Register();

		FString UiTextureImportSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureSource="), UiTextureImportSource))
		{
			TunaSweeperEditorSetup::ImportUiTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureQuit")))
			{
				return;
			}
		}

		FString AudioImportSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioSource="), AudioImportSource))
		{
			TunaSweeperEditorSetup::ImportAudioFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioQuit")))
			{
				return;
			}
		}

		FString MemoStorageTextureSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureSource="), MemoStorageTextureSource))
		{
			TunaSweeperEditorSetup::ImportMemoStorageDeviceTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureQuit")))
			{
				return;
			}
		}

		FString RollingBomberSpawnerTextureSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureSource="), RollingBomberSpawnerTextureSource))
		{
			TunaSweeperEditorSetup::ImportRollingBomberSpawnerTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureQuit")))
			{
				return;
			}
		}

		FString SandbagCoverTextureSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureSource="), SandbagCoverTextureSource))
		{
			TunaSweeperEditorSetup::ImportSandbagCoverTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureQuit")))
			{
				return;
			}
		}

		LevelOpenTool = MakeUnique<FTunaSweeperLevelOpenTool>();
		LevelOpenTool->Startup();

		FMSoundTool = MakeUnique<FTunaSweeperFMSoundTool>();
		FMSoundTool->Startup();

		GlbTextureExtractorTool = MakeUnique<FTunaSweeperGlbTextureExtractorTool>();
		GlbTextureExtractorTool->Startup();

		TunaSweeperEditorSetup::RunEditorOneShotSetup_ToCleanupOnExplicitRequest();
	}

	virtual void ShutdownModule() override
	{
		TunaSweeperMapCaptureActorDetails::Unregister();

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

		if (GlbTextureExtractorTool)
		{
			GlbTextureExtractorTool->Shutdown();
			GlbTextureExtractorTool.Reset();
		}
	}

private:
	TUniquePtr<FTunaSweeperLevelOpenTool> LevelOpenTool;
	TUniquePtr<FTunaSweeperFMSoundTool> FMSoundTool;
	TUniquePtr<FTunaSweeperGlbTextureExtractorTool> GlbTextureExtractorTool;
};

IMPLEMENT_MODULE(FTunaSweeperEditorModule, TunaSweeperEditor)
