#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystem/TunaSweeperHousingSubsystem.h"

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

namespace TunaSweeperHousingSubsystemTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperHousingSubsystemDisabledTest,
	"TunaSweeper.Housing.SubsystemDisabled",
	TunaSweeperHousingSubsystemTests::TestFlags)

bool FTunaSweeperHousingSubsystemDisabledTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	const UTunaSweeperHousingSubsystem* HousingSubsystem =
		NewObject<UTunaSweeperHousingSubsystem>(GameInstance);

	TestNotNull(TEXT("The housing subsystem class remains available for deferred removal"), HousingSubsystem);
	TestFalse(
		TEXT("Game instances do not create the disabled housing subsystem"),
		HousingSubsystem && HousingSubsystem->ShouldCreateSubsystem(nullptr));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
