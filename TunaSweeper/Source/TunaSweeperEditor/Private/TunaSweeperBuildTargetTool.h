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
	void SelectBuildTarget(ETunaSweeperBuildTarget BuildTarget);
	bool IsBuildTargetSelected(ETunaSweeperBuildTarget BuildTarget) const;
	bool CanSelectBuildTarget() const;
	void TogglePackaging();
	bool IsPackagingEnabled() const;
	bool CanTogglePackaging() const;
	void ToggleClean();
	bool IsCleanEnabled() const;
	bool CanToggleClean() const;
	void ToggleRun();
	bool IsRunEnabled() const;
	bool CanToggleRun() const;
	void StartPackaging(ETunaSweeperBuildTarget BuildTarget);
	void LaunchPackagedBuild(const FString& ExecutablePath) const;

	bool bPackagingEnabled = false;
	bool bCleanEnabled = false;
	bool bRunEnabled = false;
	bool bPackagingInProgress = false;
};
