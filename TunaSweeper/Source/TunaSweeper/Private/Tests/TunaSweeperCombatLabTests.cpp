#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/TunaSweeperCombatLabDecision.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatLabThreatTest,
	"TunaSweeper.Combat.Lab.ProjectileThreat", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperCombatLabThreatTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperCombatLab;
	TestTrue(TEXT("Closing projectile intersects player within reaction horizon"), IsIncomingThreat(FVector(600, 0, 0), FVector(-3000, 0, 0)));
	TestFalse(TEXT("Departing projectile must not spend stamina"), IsIncomingThreat(FVector(200, 0, 0), FVector(3000, 0, 0)));
	TestFalse(TEXT("Parallel safe lane must not trigger roll"), IsIncomingThreat(FVector(600, 180, 0), FVector(-3000, 0, 0)));
	TestFalse(TEXT("Distant future threat is not an instant dodge"), IsIncomingThreat(FVector(2000, 0, 0), FVector(-1000, 0, 0)));
	TestFalse(TEXT("Stationary effect is not a projectile threat"), IsIncomingThreat(FVector(20, 0, 0), FVector::ZeroVector));
	return true;
}
#endif
