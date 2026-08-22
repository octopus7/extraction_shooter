#include "Settings/TunaSweeperBuildTargetSettings.h"

FString UTunaSweeperBuildTargetSettings::GetDistributionChannel() const
{
	switch (BuildTarget)
	{
	case ETunaSweeperBuildTarget::SteamFull:
	case ETunaSweeperBuildTarget::SteamDemo: return TEXT("Steam");
	case ETunaSweeperBuildTarget::StoveFull:
	case ETunaSweeperBuildTarget::StoveDemo: return TEXT("Stove");
	case ETunaSweeperBuildTarget::NoStoreFull:
	case ETunaSweeperBuildTarget::NoStoreDemo:
	default: return TEXT("None");
	}
}

bool UTunaSweeperBuildTargetSettings::IsDemoBuild() const
{
	return BuildTarget == ETunaSweeperBuildTarget::NoStoreDemo ||
		BuildTarget == ETunaSweeperBuildTarget::SteamDemo ||
		BuildTarget == ETunaSweeperBuildTarget::StoveDemo;
}
