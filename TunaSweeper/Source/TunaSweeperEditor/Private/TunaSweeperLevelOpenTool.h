#pragma once

#include "CoreMinimal.h"

class UToolMenu;

class FTunaSweeperLevelOpenTool
{
public:
	void Startup();
	void Shutdown();

private:
	void RegisterMenus();
	void PopulateOpenLevelMenu(UToolMenu* Menu);
	void OpenLevel(FString MapPackagePath) const;
	void ToggleUseBoxRaidLevel();
	bool CanToggleUseBoxRaidLevel() const;
	bool IsUsingBoxRaidLevel() const;
};
