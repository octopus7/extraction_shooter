#include "Game/TunaSweeperGameInstance.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/TunaSweeperStartupLogoFlow.h"

namespace
{
	constexpr const TCHAR* Section = TEXT("TunaSweeper.StartupLogo");
	constexpr const TCHAR* DateKey = TEXT("MainLastCompletedLocalDate");
}

bool UTunaSweeperGameInstance::TryBeginDeveloperLogo()
{
	bool bSeenToday = false;
	// Do not even read the date while disabled; existing records must have no effect.
	if (bDeveloperLogoOncePerDay && GConfig)
	{
		FString LastDate;
		GConfig->GetString(Section, DateKey, LastDate, GGameUserSettingsIni);
		bSeenToday = LastDate == FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
	}
	const bool bShow = TunaSweeperStartupLogo::ShouldShow(
		TunaSweeperBuildFlavor::IsDemo(), bDeveloperLogoAttemptedThisSession,
		bDeveloperLogoOncePerDay, bSeenToday);
	// Consume the startup opportunity even when the daily policy skips it.
	// This prevents a title revisit at midnight from playing an unexpected logo.
	bDeveloperLogoAttemptedThisSession = true;
	return bShow;
}

void UTunaSweeperGameInstance::RecordDeveloperLogoCompleted()
{
	if (!bDeveloperLogoOncePerDay || TunaSweeperBuildFlavor::IsDemo() || !GConfig) return;
	GConfig->SetString(Section, DateKey, *FDateTime::Now().ToString(TEXT("%Y-%m-%d")), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}
