#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystem/TunaSweeperRaidPlacementSubsystem.h"

#include "Misc/AutomationTest.h"

namespace TunaSweeperRaidPlacementTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperRaidPlacementDeterministicRollTest,
	"TunaSweeper.RaidPlacement.DeterministicRoll",
	TunaSweeperRaidPlacementTests::TestFlags)

bool FTunaSweeperRaidPlacementDeterministicRollTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const float First = UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48271, 101);
	const float Interleaved = UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48271, 202);
	const float Second = UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48271, 101);

	TestTrue(TEXT("Roll is in [0, 1)"), First >= 0.0f && First < 1.0f);
	TestEqual(TEXT("Same RaidSeed and PlacementId reproduce exactly"), First, Second);
	TestNotEqual(TEXT("PlacementId contributes to the roll"), First, Interleaved);
	TestNotEqual(TEXT("RaidSeed contributes to the roll"), First, UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(48272, 101));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
