#pragma once
#include "CoreMinimal.h"
class FTunaSweeperQuestCheatTool
{
public:
    void Startup();
    void Shutdown();
private:
    void RegisterMenus();
    void Open();
    TSharedRef<class SDockTab> Spawn(const class FSpawnTabArgs& Args);
    FDelegateHandle StandaloneHandle;
};
