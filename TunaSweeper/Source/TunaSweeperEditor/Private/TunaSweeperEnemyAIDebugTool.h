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
	void ToggleSkirtPhysicsDebugDraw();
	bool IsSkirtPhysicsDebugDrawEnabled() const;
	TSharedRef<class SDockTab> SpawnToolTab(const class FSpawnTabArgs& SpawnTabArgs);
};
