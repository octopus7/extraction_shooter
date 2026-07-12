#pragma once

#include "CoreMinimal.h"

class FTunaSweeperFMSoundTool
{
public:
	void Startup();
	void Shutdown();

	/** Renders the default rifle presentation SFX as editable WAV source files. */
	static bool RenderWeaponPresentationWavs(
		FString& OutFireWavPath,
		FString& OutReloadStartWavPath,
		FString& OutReloadCompleteWavPath);

private:
	void RegisterMenus();
	void OpenToolWindow();
	TSharedRef<class SDockTab> SpawnToolTab(const class FSpawnTabArgs& SpawnTabArgs);
};
