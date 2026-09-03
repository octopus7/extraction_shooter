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
	const bool bOriginalUseBoxRaidLevel = Settings->bUseBoxRaidLevel;

	Settings->BuildTarget = ETunaSweeperBuildTarget::NoStoreDemo;
	Settings->bUseBoxRaidLevel = false;
	TestTrue(TEXT("Demo target is Demo flavor"), TunaSweeperBuildFlavor::IsDemo());
	TestEqual(TEXT("Demo flavor name"), TunaSweeperBuildFlavor::GetName(), FName(TEXT("Demo")));
	TestTrue(
		TEXT("Demo saves use Demo root"),
		TunaSweeperBuildFlavor::GetSaveGameDirectory().EndsWith(TEXT("SaveGames/Demo")));
	TestTrue(
		TEXT("Demo quest definitions are public Content data"),
		TunaSweeperBuildFlavor::GetQuestDefinitionsPath().EndsWith(TEXT("Content/Data/QuestDefinitions.json")));
	TestTrue(
		TEXT("Demo scenarios are public Content data"),
		TunaSweeperBuildFlavor::GetScenarioDefinitionsPath().EndsWith(TEXT("Content/Data/ScenarioDefinitions.json")));
	TestTrue(
		TEXT("Demo scenario strings are public Content data"),
		TunaSweeperBuildFlavor::GetScenarioTextStringsPath().EndsWith(TEXT("Content/Data/ScenarioTextStrings.csv")));
	TestEqual(TEXT("Demo exposes one save slot"), TunaSweeperBuildFlavor::GetMaximumSaveSlotIndex(), 1);
	TestEqual(TEXT("Demo raid role uses DemoRaidMap"), TunaSweeperBuildFlavor::GetRaidGameplayLevelName(), FName(TEXT("DemoRaidMap")));
	TestEqual(
		TEXT("Logical RaidMap resolves to the Demo raid role"),
		TunaSweeperBuildFlavor::ResolveGameplayLevelName(FName(TEXT("RaidMap"))),
		FName(TEXT("DemoRaidMap")));
	TestTrue(
		TEXT("Demo placement data uses public Content data"),
		TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("EnemySpawns.json"))
			.EndsWith(TEXT("Content/Data/EnemySpawns.json")));
	TestTrue(
		TEXT("DemoRaidMap asset exists"),
		FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/DemoRaidMap.umap"))));

	Settings->bUseBoxRaidLevel = true;
	TestEqual(
		TEXT("Demo box raid option uses DemoBoxRaidMap"),
		TunaSweeperBuildFlavor::GetRaidGameplayLevelName(),
		FName(TEXT("DemoBoxRaidMap")));
	TestEqual(
		TEXT("Logical RaidMap resolves to the selected demo box raid role"),
		TunaSweeperBuildFlavor::ResolveGameplayLevelName(FName(TEXT("RaidMap"))),
		FName(TEXT("DemoBoxRaidMap")));
	TestTrue(
		TEXT("DemoBoxRaidMap asset exists"),
		FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/DemoBoxRaidMap.umap"))));

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
	TestEqual(TEXT("Main exposes three save slots"), TunaSweeperBuildFlavor::GetMaximumSaveSlotIndex(), 3);
	TestFalse(
		TEXT("Main raid role is not the Demo raid map"),
		TunaSweeperBuildFlavor::GetRaidGameplayLevelName() == FName(TEXT("DemoRaidMap")));
	TestEqual(
		TEXT("Full target ignores the demo box raid option"),
		TunaSweeperBuildFlavor::GetRaidGameplayLevelName(),
		FName(TEXT("/Game/MainRaid/RaidMap")));
	TestFalse(
		TEXT("Main placement data does not use the Demo public placement file"),
		TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("EnemySpawns.json"))
			.EndsWith(TEXT("Content/Data/EnemySpawns.json")));
	TestFalse(
		TEXT("Main scenarios do not use the Demo public scenario pack"),
		TunaSweeperBuildFlavor::GetScenarioDefinitionsPath().EndsWith(TEXT("Content/Data/ScenarioDefinitions.json")));
	TestFalse(
		TEXT("Main scenario strings do not use the Demo public scenario strings"),
		TunaSweeperBuildFlavor::GetScenarioTextStringsPath().EndsWith(TEXT("Content/Data/ScenarioTextStrings.csv")));

	Settings->BuildTarget = OriginalTarget;
	Settings->bUseBoxRaidLevel = bOriginalUseBoxRaidLevel;
	return true;
}

#endif
