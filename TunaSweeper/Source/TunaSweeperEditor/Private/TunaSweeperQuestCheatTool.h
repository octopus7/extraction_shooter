#pragma once
#include "CoreMinimal.h"
class FTunaSweeperQuestCheatTool
{
public:
    void Startup();
    void Shutdown();
    static bool IsEditingLocked();
private:
    void RegisterMenus();
    void Open();
    TSharedRef<class SDockTab> Spawn(const class FSpawnTabArgs& Args);
    FDelegateHandle StandaloneHandle;
};
