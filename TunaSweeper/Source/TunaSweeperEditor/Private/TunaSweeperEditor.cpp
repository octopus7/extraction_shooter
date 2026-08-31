#include "TunaSweeperEditorSetupShared.h"
#include "TunaSweeperBuildTargetTool.h"
#include "TunaSweeperEnemyAIDebugTool.h"
#include "TunaSweeperLunaMk2SideTailPhysicsSetup.h"
#include "TunaSweeperLunaSkirtPhysicsSetup.h"
#include "TunaSweeperQuadrupedPresetSetup.h"
#include "TunaSweeperGarageDoorSetup.h"
#include "TunaSweeperLocationBlendCameraSetup.h"
#include "TunaSweeperBoilingPotSetup.h"
#include "TunaSweeperWallCopingSetup.h"

#include "Containers/Ticker.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY(LogTunaSweeperEditor);

namespace TunaSweeperRaidPlacementAnchorAssets
{
	bool CreateAssets();
}

class FTunaSweeperEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (IsRunningCommandlet())
		{
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRaidPlacementAnchorAssets")))
		{
			RaidPlacementAnchorAssetsInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForRaidPlacementAnchorAssets);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperWallCopingSetup")))
		{
			WallCopingSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForWallCopingSetup);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperBoilingPotSetup")))
		{
			BoilingPotSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForBoilingPotSetup);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLunaSkirtPhysicsSetup")))
		{
			LunaSkirtSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForLunaSkirtSetup);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLunaMk2SideTailPhysicsSetup")))
		{
			LunaMk2SideTailSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForLunaMk2SideTailSetup);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperGarageDoorSetup")))
		{
			GarageDoorSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForGarageDoorSetup);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLocationBlendCameraSetup")))
		{
			LocationBlendCameraSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForLocationBlendCameraSetup);
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperJumpInputSetup")))
		{
			JumpInputSetupInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
				this,
				&FTunaSweeperEditorModule::OnEditorInitializedForJumpInputSetup);
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

		TunaSweeperEditorSetup::RunEditorOneShotSetup_ToCleanupOnExplicitRequest();
	}

	virtual void ShutdownModule() override
	{
		if (WallCopingSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(WallCopingSetupInitializedHandle);
			WallCopingSetupInitializedHandle.Reset();
		}

		if (WallCopingSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(WallCopingSetupTickerHandle);
			WallCopingSetupTickerHandle.Reset();
		}

		if (BoilingPotSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(BoilingPotSetupInitializedHandle);
			BoilingPotSetupInitializedHandle.Reset();
		}

		if (BoilingPotSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(BoilingPotSetupTickerHandle);
			BoilingPotSetupTickerHandle.Reset();
		}

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

		if (LunaMk2SideTailSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(LunaMk2SideTailSetupInitializedHandle);
			LunaMk2SideTailSetupInitializedHandle.Reset();
		}

		if (LunaMk2SideTailSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(LunaMk2SideTailSetupTickerHandle);
			LunaMk2SideTailSetupTickerHandle.Reset();
		}

		if (GarageDoorSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(GarageDoorSetupInitializedHandle);
			GarageDoorSetupInitializedHandle.Reset();
		}

		if (GarageDoorSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GarageDoorSetupTickerHandle);
			GarageDoorSetupTickerHandle.Reset();
		}

		if (LocationBlendCameraSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(LocationBlendCameraSetupInitializedHandle);
			LocationBlendCameraSetupInitializedHandle.Reset();
		}

		if (LocationBlendCameraSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(LocationBlendCameraSetupTickerHandle);
			LocationBlendCameraSetupTickerHandle.Reset();
		}

		if (JumpInputSetupInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(JumpInputSetupInitializedHandle);
			JumpInputSetupInitializedHandle.Reset();
		}

		if (JumpInputSetupTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(JumpInputSetupTickerHandle);
			JumpInputSetupTickerHandle.Reset();
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

		if (RaidPlacementAnchorAssetsInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(RaidPlacementAnchorAssetsInitializedHandle);
			RaidPlacementAnchorAssetsInitializedHandle.Reset();
		}

		if (RaidPlacementAnchorAssetsTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(RaidPlacementAnchorAssetsTickerHandle);
			RaidPlacementAnchorAssetsTickerHandle.Reset();
		}

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
	void OnEditorInitializedForBoilingPotSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(BoilingPotSetupInitializedHandle);
		BoilingPotSetupInitializedHandle.Reset();
		BoilingPotSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunBoilingPotSetupAfterInitialization));
	}

	bool RunBoilingPotSetupAfterInitialization(float)
	{
		BoilingPotSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperBoilingPotSetup::Run();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperBoilingPotSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperBoilingPotSetup"));
		}
		return false;
	}

	void OnEditorInitializedForWallCopingSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(WallCopingSetupInitializedHandle);
		WallCopingSetupInitializedHandle.Reset();
		WallCopingSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunWallCopingSetupAfterInitialization));
	}

	bool RunWallCopingSetupAfterInitialization(float)
	{
		WallCopingSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperWallCopingSetup::Run();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperWallCopingSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperWallCopingSetup"));
		}
		return false;
	}

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

	void OnEditorInitializedForLunaMk2SideTailSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(LunaMk2SideTailSetupInitializedHandle);
		LunaMk2SideTailSetupInitializedHandle.Reset();
		LunaMk2SideTailSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunLunaMk2SideTailSetupAfterInitialization));
	}

	bool RunLunaMk2SideTailSetupAfterInitialization(float)
	{
		LunaMk2SideTailSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperLunaMk2SideTailPhysicsSetup::Run();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLunaMk2SideTailPhysicsSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperLunaMk2SideTailPhysicsSetup"));
		}
		return false;
	}

	void OnEditorInitializedForGarageDoorSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(GarageDoorSetupInitializedHandle);
		GarageDoorSetupInitializedHandle.Reset();
		GarageDoorSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunGarageDoorSetupAfterInitialization));
	}

	bool RunGarageDoorSetupAfterInitialization(float)
	{
		GarageDoorSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperGarageDoorSetup::Run();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperGarageDoorSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperGarageDoorSetup"));
		}
		return false;
	}

	void OnEditorInitializedForLocationBlendCameraSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(LocationBlendCameraSetupInitializedHandle);
		LocationBlendCameraSetupInitializedHandle.Reset();
		LocationBlendCameraSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunLocationBlendCameraSetupAfterInitialization));
	}

	bool RunLocationBlendCameraSetupAfterInitialization(float)
	{
		LocationBlendCameraSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperLocationBlendCameraSetup::Run();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLocationBlendCameraSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperLocationBlendCameraSetup"));
		}
		return false;
	}

	void OnEditorInitializedForJumpInputSetup(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(JumpInputSetupInitializedHandle);
		JumpInputSetupInitializedHandle.Reset();
		JumpInputSetupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunJumpInputSetupAfterInitialization));
	}

	bool RunJumpInputSetupAfterInitialization(float)
	{
		JumpInputSetupTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperEditorSetup::EnsureJumpInputAssets();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperJumpInputSetupQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperJumpInputSetup"));
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

	void OnEditorInitializedForRaidPlacementAnchorAssets(double)
	{
		FEditorDelegates::OnEditorInitialized.Remove(RaidPlacementAnchorAssetsInitializedHandle);
		RaidPlacementAnchorAssetsInitializedHandle.Reset();
		RaidPlacementAnchorAssetsTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FTunaSweeperEditorModule::RunRaidPlacementAnchorAssetsAfterInitialization));
	}

	bool RunRaidPlacementAnchorAssetsAfterInitialization(float)
	{
		RaidPlacementAnchorAssetsTickerHandle.Reset();
		const bool bSucceeded = TunaSweeperRaidPlacementAnchorAssets::CreateAssets();
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRaidPlacementAnchorAssetsQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperRaidPlacementAnchorAssets"));
		}
		return false;
	}

	bool bStandardEditorSetupStarted = false;
	FDelegateHandle BoilingPotSetupInitializedHandle;
	FTSTicker::FDelegateHandle BoilingPotSetupTickerHandle;
	FDelegateHandle WallCopingSetupInitializedHandle;
	FTSTicker::FDelegateHandle WallCopingSetupTickerHandle;
	FDelegateHandle LunaSkirtSetupInitializedHandle;
	FTSTicker::FDelegateHandle LunaSkirtSetupTickerHandle;
	FDelegateHandle LunaMk2SideTailSetupInitializedHandle;
	FTSTicker::FDelegateHandle LunaMk2SideTailSetupTickerHandle;
	FDelegateHandle GarageDoorSetupInitializedHandle;
	FTSTicker::FDelegateHandle GarageDoorSetupTickerHandle;
	FDelegateHandle LocationBlendCameraSetupInitializedHandle;
	FTSTicker::FDelegateHandle LocationBlendCameraSetupTickerHandle;
	FDelegateHandle JumpInputSetupInitializedHandle;
	FTSTicker::FDelegateHandle JumpInputSetupTickerHandle;
	FDelegateHandle QuadrupedSetupInitializedHandle;
	FTSTicker::FDelegateHandle QuadrupedSetupTickerHandle;
	FDelegateHandle RaidPlacementAnchorAssetsInitializedHandle;
	FTSTicker::FDelegateHandle RaidPlacementAnchorAssetsTickerHandle;
	TUniquePtr<FTunaSweeperBuildTargetTool> BuildTargetTool;
	TUniquePtr<FTunaSweeperLevelOpenTool> LevelOpenTool;
	TUniquePtr<FTunaSweeperEnemyAIDebugTool> EnemyAIDebugTool;
	TUniquePtr<FTunaSweeperFMSoundTool> FMSoundTool;
	TUniquePtr<FTunaSweeperGlbTextureExtractorTool> GlbTextureExtractorTool;
};

IMPLEMENT_MODULE(FTunaSweeperEditorModule, TunaSweeperEditor)
