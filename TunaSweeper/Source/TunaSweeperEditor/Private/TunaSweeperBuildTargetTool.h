#pragma once

#include "CoreMinimal.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"

class UToolMenu;

class FTunaSweeperBuildTargetTool
{
public:
	void Startup();
	void Shutdown();

private:
	void RegisterMenus();
	void PopulateBuildTargetMenu(UToolMenu* Menu);
	void SelectBuildTarget(ETunaSweeperBuildTarget BuildTarget) const;
	bool IsBuildTargetSelected(ETunaSweeperBuildTarget BuildTarget) const;
};
