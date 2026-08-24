#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/TunaSweeperBuildFlavor.h"

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"

namespace TunaSweeperBuildFlavorTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperBuildFlavorPathTest,
	"TunaSweeper.BuildFlavor.Paths",
	TunaSweeperBuildFlavorTests::TestFlags)

bool FTunaSweeperBuildFlavorPathTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UTunaSweeperBuildTargetSettings* Settings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	const ETunaSweeperBuildTarget OriginalTarget = Settings->BuildTarget;

	Settings->BuildTarget = ETunaSweeperBuildTarget::NoStoreDemo;
	TestTrue(TEXT("Demo target is Demo flavor"), TunaSweeperBuildFlavor::IsDemo());
	TestEqual(TEXT("Demo flavor name"), TunaSweeperBuildFlavor::GetName(), FName(TEXT("Demo")));
	TestTrue(
		TEXT("Demo saves use Demo root"),
		TunaSweeperBuildFlavor::GetSaveGameDirectory().EndsWith(TEXT("SaveGames/Demo")));
	TestTrue(
		TEXT("Demo quest definitions are public Content data"),
		TunaSweeperBuildFlavor::GetQuestDefinitionsPath().EndsWith(TEXT("Content/Data/QuestDefinitions.json")));

	Settings->BuildTarget = ETunaSweeperBuildTarget::NoStoreFull;
	TestFalse(TEXT("Full target is Main flavor"), TunaSweeperBuildFlavor::IsDemo());
	TestEqual(TEXT("Main flavor name"), TunaSweeperBuildFlavor::GetName(), FName(TEXT("Main")));
	TestTrue(
		TEXT("Main saves use Main root"),
		TunaSweeperBuildFlavor::GetSaveGameDirectory().EndsWith(TEXT("SaveGames/Main")));
	TestNotEqual(
		TEXT("Demo and Main save roots are siblings, not the same path"),
		FPaths::GetCleanFilename(TunaSweeperBuildFlavor::GetSaveGameDirectory()),
		FString(TEXT("Demo")));

	Settings->BuildTarget = OriginalTarget;
	return true;
}

#endif
