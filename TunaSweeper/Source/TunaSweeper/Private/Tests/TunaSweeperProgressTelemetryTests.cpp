#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Subsystem/TunaSweeperProgressTelemetrySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperProgressTelemetryQuestTest,
	"TunaSweeper.ProgressTelemetry.QuestMilestones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperProgressTelemetryQuestTest::RunTest(const FString& Parameters)
{
	using EState = ETunaSweeperQuestState;
	using TunaSweeperProgressTelemetry::QuestMilestone;
	TestEqual(TEXT("Accepting is recorded independently"),
		FString(QuestMilestone(EState::Available, EState::Accepted, false)), FString(TEXT("accepted")));
	TestEqual(TEXT("A real objective transition is recorded"),
		FString(QuestMilestone(EState::Accepted, EState::RewardAvailable, true)), FString(TEXT("objective_met")));
	TestNull(TEXT("Auto completion with no objectives does not invent an objective milestone"),
		QuestMilestone(EState::Accepted, EState::RewardAvailable, false));
	TestEqual(TEXT("Claiming a reward is distinct from reaching the objective"),
		FString(QuestMilestone(EState::RewardAvailable, EState::RewardCompleted, true)), FString(TEXT("reward_claimed")));
	TestNull(TEXT("Losing items and reverting to accepted is not another acceptance"),
		QuestMilestone(EState::RewardAvailable, EState::Accepted, true));
	TestNull(TEXT("Repeating a completed state is ignored"),
		QuestMilestone(EState::RewardCompleted, EState::RewardCompleted, true));
	return true;
}

#endif
