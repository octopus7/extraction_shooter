#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"
#include "Engine/Texture2D.h"
#include "FileMediaSource.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperStartupLogoPolicyTest,
	"TunaSweeper.StartupLogo.PolicyAndAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperStartupLogoPolicyTest::RunTest(const FString&)
{
	UTunaSweeperBuildTargetSettings* Settings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	const auto OriginalTarget = Settings->BuildTarget;
	const TCHAR* Section = TEXT("TunaSweeper.StartupLogo");
	const TCHAR* Key = TEXT("MainLastCompletedLocalDate");
	FString OriginalDate;
	const bool bHadDate = GConfig->GetString(Section, Key, OriginalDate, GGameUserSettingsIni);
	const FString Today = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
	GConfig->SetString(Section, Key, *Today, GGameUserSettingsIni);
	Settings->BuildTarget = ETunaSweeperBuildTarget::NoStoreFull;
	UTunaSweeperGameInstance* Instance = NewObject<UTunaSweeperGameInstance>();
	TestFalse(TEXT("Daily limit defaults OFF"), Instance->bDeveloperLogoOncePerDay);
	TestTrue(TEXT("OFF ignores a persisted date from today"), Instance->TryBeginDeveloperLogo());
	TestFalse(TEXT("Title revisit skips logo"), Instance->TryBeginDeveloperLogo());
	GConfig->SetString(Section, Key, TEXT("sentinel-date"), GGameUserSettingsIni);
	Instance->RecordDeveloperLogoCompleted();
	FString After;
	GConfig->GetString(Section, Key, After, GGameUserSettingsIni);
	TestEqual(TEXT("OFF does not write completion date"), After, FString(TEXT("sentinel-date")));
	Instance->bDeveloperLogoOncePerDay = true;
	Instance->bDeveloperLogoAttemptedThisSession = false;
	GConfig->SetString(Section, Key, *Today, GGameUserSettingsIni);
	TestFalse(TEXT("ON skips today's completed logo"), Instance->TryBeginDeveloperLogo());
	Instance->bDeveloperLogoAttemptedThisSession = false;
	GConfig->SetString(Section, Key, TEXT("2000-01-01"), GGameUserSettingsIni);
	TestTrue(TEXT("ON permits a new calendar day"), Instance->TryBeginDeveloperLogo());
	Settings->BuildTarget = ETunaSweeperBuildTarget::NoStoreDemo;
	Instance->bDeveloperLogoAttemptedThisSession = false;
	Instance->bDeveloperLogoOncePerDay = false;
	TestFalse(TEXT("Demo bypasses the Main-only intro"), Instance->TryBeginDeveloperLogo());
	Settings->BuildTarget = OriginalTarget;
	if (bHadDate) GConfig->SetString(Section, Key, *OriginalDate, GGameUserSettingsIni);
	else GConfig->RemoveKey(Section, Key, GGameUserSettingsIni);
	// No flush: tests never persist synthetic dates or alter gameplay saves.
	UTexture2D* Logo = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Title/T_DevTunaLogoWhite.T_DevTunaLogoWhite"));
	TestNotNull(TEXT("Imported white logo is loadable"), Logo);
	if (Logo)
	{
		TestEqual(TEXT("Logo uses UI texture group"), Logo->LODGroup, TEXTUREGROUP_UI);
		// Runtime dimensions can still be the async compilation placeholder in an editor test.
		TestTrue(TEXT("Logo source is landscape"), Logo->Source.GetSizeX() > Logo->Source.GetSizeY());
	}
	UFileMediaSource* Movie = LoadObject<UFileMediaSource>(nullptr, TEXT("/Game/Movies/MS_DevTunaCut.MS_DevTunaCut"));
	TestNotNull(TEXT("Developer movie source is loadable"), Movie);
	if (Movie) TestTrue(TEXT("Developer movie path is valid"), Movie->Validate());
	return true;
}
#endif
