#include "Settings/TunaSweeperVersionSettings.h"

UTunaSweeperVersionSettings::UTunaSweeperVersionSettings()
{
	InternalBuildNumber = 1000;
	PublicVersionString = TEXT("v1.0.0");
	VersionCheckUrl = TEXT("https://tuna-sweeper-ver.pages.dev/version.json");
}
