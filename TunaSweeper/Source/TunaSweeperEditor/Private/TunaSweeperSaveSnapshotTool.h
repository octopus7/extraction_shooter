#pragma once
#include "CoreMinimal.h"

class FTunaSweeperSaveSnapshotTool
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
