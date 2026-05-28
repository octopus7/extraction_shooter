#pragma once

#include "CoreMinimal.h"

class FTunaSweeperFMSoundTool
{
public:
	void Startup();
	void Shutdown();

private:
	void RegisterMenus();
	void OpenToolWindow();
	TSharedRef<class SDockTab> SpawnToolTab(const class FSpawnTabArgs& SpawnTabArgs);
};
