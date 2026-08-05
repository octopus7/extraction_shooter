#include "TunaSweeperEditorSetupShared.h"
#include "TunaSweeperEnemyAIDebugTool.h"
#include "TunaSweeperLunaSkirtPhysicsSetup.h"
#include "TunaSweeperQuadrupedPresetSetup.h"

#include "Containers/Ticker.h"
#include "Editor.h"

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

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLunaSkirtPhysicsSetup")))
		{
			LunaSkirtSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForLunaSkirtSetup);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperQuadrupedAnimSetup")))
		{
			QuadrupedSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForQuadrupedSetup);
			return;
		}

		bStandardEditorSetupStarted = true;
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

		EnemyAIDebugTool = MakeUnique<FTunaSweeperEnemyAIDebugTool>();
		EnemyAIDebugTool->Startup();

		FMSoundTool = MakeUnique<FTunaSweeperFMSoundTool>();
		FMSoundTool->Startup();

		GlbTextureExtractorTool = MakeUnique<FTunaSweeperGlbTextureExtractorTool>();
		GlbTextureExtractorTool->Startup();

		TunaSweeperEditorSetup::RunEditorOneShotSetup_ToCleanupOnExplicitRequest();
	}

	virtual void ShutdownModule() override
	{
		if (LunaSkirtSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(LunaSkirtSetupInitializedHandle);
			LunaSkirtSetupInitializedHandle.Reset();
		}

		if (LunaSkirtSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(LunaSkirtSetupTickerHandle);
			LunaSkirtSetupTickerHandle.Reset();
		}

		if (QuadrupedSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(QuadrupedSetupInitializedHandle);
			QuadrupedSetupInitializedHandle.Reset();
		}

		if (QuadrupedSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(QuadrupedSetupTickerHandle);
			QuadrupedSetupTickerHandle.Reset();
		}

		if (bStandardEditorSetupStarted)
		{
			TunaSweeperMapCaptureActorDetails::Unregister();
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
	void OnEditorInitializedForLunaSkirtSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(LunaSkirtSetupInitializedHandle);
		LunaSkirtSetupInitializedHandle.Reset();
		LunaSkirtSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunLunaSkirtSetupAfterInitialization));
	}

	bool RunLunaSkirtSetupAfterInitialization(float)
	{
		LunaSkirtSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperLunaSkirtPhysicsSetup::Run();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLunaSkirtPhysicsSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperLunaSkirtPhysicsSetup"));
		}
		return false;
	}

	void OnEditorInitializedForQuadrupedSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(QuadrupedSetupInitializedHandle);
		QuadrupedSetupInitializedHandle.Reset();
		QuadrupedSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunQuadrupedSetupAfterInitialization));
	}

	bool RunQuadrupedSetupAfterInitialization(float)
	{
		QuadrupedSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperQuadrupedPresetSetup::Run();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperQuadrupedAnimSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperQuadrupedAnimSetup"));
		}
		return false;
	}

	bool bStandardEditorSetupStarted = false;
	FDelegateHandle LunaSkirtSetupInitializedHandle;
	FTSTicker::FDelegateHandle LunaSkirtSetupTickerHandle;
	FDelegateHandle QuadrupedSetupInitializedHandle;
	FTSTicker::FDelegateHandle QuadrupedSetupTickerHandle;
	TUniquePtr<FTunaSweeperLevelOpenTool> LevelOpenTool;
	TUniquePtr<FTunaSweeperEnemyAIDebugTool> EnemyAIDebugTool;
	TUniquePtr<FTunaSweeperFMSoundTool> FMSoundTool;
	TUniquePtr<FTunaSweeperGlbTextureExtractorTool> GlbTextureExtractorTool;
};

IMPLEMENT_MODULE(FTunaSweeperEditorModule, TunaSweeperEditor)
