#pragma once

#include "CoreMinimal.h"

class FTunaSweeperEnemyAIDebugTool
{
public:
	void Startup();
	void Shutdown();

private:
	void RegisterMenus();
	void OpenToolWindow();
	TSharedRef<class SDockTab> SpawnToolTab(const class FSpawnTabArgs& SpawnTabArgs);
};
