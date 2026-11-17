#pragma once
#include "CoreMinimal.h"

class FTunaSweeperItemGrantTool
{
public:
	void Startup();
	void Shutdown();
private:
	void RegisterMenus();
	void Open();
	TSharedRef<class SDockTab> Spawn(const class FSpawnTabArgs& Args);
};
