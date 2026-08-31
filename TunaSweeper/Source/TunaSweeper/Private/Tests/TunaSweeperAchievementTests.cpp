#if WITH_DEV_AUTOMATION_TESTS

#include "Achievement/TunaSweeperAchievementModel.h"
#include "Misc/AutomationTest.h"

namespace TunaSweeperAchievementTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	const FName SteamPlatform(TEXT("Steam"));

	FTunaSweeperAchievementDefinition MakeDefinition(
		const TCHAR* AchievementId,
		ETunaSweeperAchievementConditionType ConditionType,
		const TCHAR* TargetId,
		int64 RequiredCount,
		const TCHAR* SteamId)
	{
		FTunaSweeperAchievementDefinition Definition;
		Definition.AchievementId = FName(AchievementId);
		Definition.ConditionType = ConditionType;
		Definition.TargetId = TargetId && TargetId[0] != 0 ? FName(TargetId) : NAME_None;
		Definition.RequiredCount = RequiredCount;
		Definition.PlatformIds.Add(SteamPlatform, SteamId);
		return Definition;
	}

	TArray<FTunaSweeperAchievementDefinition> MakeDefinitions()
	{
		return {
			MakeDefinition(TEXT("first_boss"), ETunaSweeperAchievementConditionType::SpecificEnemyFirstKill,
				TEXT("enemy.boss"), 1, TEXT("ACH_FIRST_BOSS")),
			MakeDefinition(TEXT("kill_one"), ETunaSweeperAchievementConditionType::TotalEnemyKills,
				TEXT(""), 1, TEXT("ACH_KILL_ONE")),
			MakeDefinition(TEXT("kill_three"), ETunaSweeperAchievementConditionType::TotalEnemyKills,
				TEXT(""), 3, TEXT("ACH_KILL_THREE")),
			MakeDefinition(TEXT("reach_lab"), ETunaSweeperAchievementConditionType::LocationReached,
				TEXT("location.lab"), 1, TEXT("ACH_REACH_LAB")),
			MakeDefinition(TEXT("claim_intro"), ETunaSweeperAchievementConditionType::QuestRewardClaimed,
				TEXT("quest.intro"), 1, TEXT("ACH_CLAIM_INTRO"))
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperAchievementDefinitionValidationTest,
	"TunaSweeper.Achievement.DefinitionValidation",
	TunaSweeperAchievementTests::TestFlags)

bool FTunaSweeperAchievementDefinitionValidationTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperAchievementTests;
	TArray<FTunaSweeperAchievementDefinition> Definitions = MakeDefinitions();
	FString Error;
	TestTrue(TEXT("Four supported condition shapes are valid"),
		TunaSweeperAchievementModel::ValidateDefinitions(Definitions, Error));

	const TArray<FString> ConfiguredIds = {
		TEXT("ACH_FIRST_BOSS"), TEXT("ACH_KILL_ONE"), TEXT("ACH_KILL_THREE"),
		TEXT("ACH_REACH_LAB"), TEXT("ACH_CLAIM_INTRO")
	};
	TestTrue(TEXT("Steam config id set matches JSON"),
		TunaSweeperAchievementModel::ValidateConfiguredPlatformIds(
			Definitions, SteamPlatform, ConfiguredIds, Error));

	TArray<FString> MissingIdConfig = ConfiguredIds;
	MissingIdConfig.Pop();
	TestFalse(TEXT("Missing Steam config id is rejected"),
		TunaSweeperAchievementModel::ValidateConfiguredPlatformIds(
			Definitions, SteamPlatform, MissingIdConfig, Error));

	Definitions[1].TargetId = FName(TEXT("invalid.target"));
	TestFalse(TEXT("Total kill definition rejects a target id"),
		TunaSweeperAchievementModel::ValidateDefinitions(Definitions, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperAchievementCollectionTest,
	"TunaSweeper.Achievement.Collection",
	TunaSweeperAchievementTests::TestFlags)

bool FTunaSweeperAchievementCollectionTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperAchievementTests;
	const TArray<FTunaSweeperAchievementDefinition> Definitions = MakeDefinitions();
	FTunaSweeperAchievementProgressState State;
	TArray<FName> NewlyUnlocked;

	TunaSweeperAchievementModel::RecordEnemyKilled(State, NAME_None);
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, NewlyUnlocked);
	TestEqual(TEXT("Unknown enemy still increments total kills"), State.TotalEnemyKills, static_cast<int64>(1));
	TestTrue(TEXT("One-kill achievement unlocks"), NewlyUnlocked.Contains(FName(TEXT("kill_one"))));
	TestFalse(TEXT("Specific enemy achievement remains locked"), State.UnlockedAchievementIds.Contains(FName(TEXT("first_boss"))));

	TunaSweeperAchievementModel::RecordEnemyKilled(State, FName(TEXT("enemy.boss")));
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, NewlyUnlocked);
	TestTrue(TEXT("First specific enemy kill unlocks once"), NewlyUnlocked.Contains(FName(TEXT("first_boss"))));

	TunaSweeperAchievementModel::RecordEnemyKilled(State, FName(TEXT("enemy.other")));
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, NewlyUnlocked);
	TestTrue(TEXT("Third total kill unlocks threshold"), NewlyUnlocked.Contains(FName(TEXT("kill_three"))));

	TunaSweeperAchievementModel::RecordEnemyKilled(State, FName(TEXT("enemy.boss")));
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, NewlyUnlocked);
	TestTrue(TEXT("Repeated target kill does not unlock again"), NewlyUnlocked.IsEmpty());

	TestTrue(TEXT("First location report changes state"),
		TunaSweeperAchievementModel::RecordLocationReached(State, FName(TEXT("location.lab"))));
	TestFalse(TEXT("Repeated location report is idempotent"),
		TunaSweeperAchievementModel::RecordLocationReached(State, FName(TEXT("location.lab"))));
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, NewlyUnlocked);
	TestTrue(TEXT("Location achievement unlocks"), NewlyUnlocked.Contains(FName(TEXT("reach_lab"))));

	TestTrue(TEXT("First quest reward claim changes state"),
		TunaSweeperAchievementModel::RecordQuestRewardClaimed(State, FName(TEXT("quest.intro"))));
	TestFalse(TEXT("Repeated quest reward claim is idempotent"),
		TunaSweeperAchievementModel::RecordQuestRewardClaimed(State, FName(TEXT("quest.intro"))));
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, NewlyUnlocked);
	TestTrue(TEXT("Claimed quest achievement unlocks"), NewlyUnlocked.Contains(FName(TEXT("claim_intro"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperAchievementPublishingStateTest,
	"TunaSweeper.Achievement.PublishingState",
	TunaSweeperAchievementTests::TestFlags)

bool FTunaSweeperAchievementPublishingStateTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperAchievementTests;
	const TArray<FTunaSweeperAchievementDefinition> Definitions = MakeDefinitions();
	FTunaSweeperAchievementProgressState State;
	State.UnlockedAchievementIds.Add(FName(TEXT("kill_one")));
	State.UnlockedAchievementIds.Add(FName(TEXT("kill_three")));

	TSet<FName> AttemptedIds;
	FName InternalId;
	FString PlatformId;
	TestTrue(TEXT("First pending unlock is selected"),
		TunaSweeperAchievementModel::FindNextPendingUnlock(
			Definitions, State, SteamPlatform, AttemptedIds, InternalId, PlatformId));
	TestEqual(TEXT("First pending internal id"), InternalId, FName(TEXT("kill_one")));
	AttemptedIds.Add(InternalId);

	TestTrue(TEXT("Failed attempted unlock is skipped for this sync"),
		TunaSweeperAchievementModel::FindNextPendingUnlock(
			Definitions, State, SteamPlatform, AttemptedIds, InternalId, PlatformId));
	TestEqual(TEXT("Next pending internal id"), InternalId, FName(TEXT("kill_three")));

	AttemptedIds.Reset();
	TestTrue(TEXT("Clearing attempts makes failed unlock retryable"),
		TunaSweeperAchievementModel::FindNextPendingUnlock(
			Definitions, State, SteamPlatform, AttemptedIds, InternalId, PlatformId));
	TestEqual(TEXT("Retry selects original pending id"), InternalId, FName(TEXT("kill_one")));

	TArray<FName> RemoteUnlocks;
	const TSet<FString> RemotePlatformIds = { TEXT("ACH_KILL_ONE"), TEXT("ACH_REACH_LAB") };
	TestTrue(TEXT("Remote query merges state"),
		TunaSweeperAchievementModel::MergePlatformState(
			Definitions, SteamPlatform, RemotePlatformIds, State, RemoteUnlocks));
	TestTrue(TEXT("Remote-only achievement becomes locally unlocked"),
		RemoteUnlocks.Contains(FName(TEXT("reach_lab"))));
	TestTrue(TEXT("Remote confirmation is persisted"),
		State.ConfirmedPlatformUnlockKeys.Contains(
			TunaSweeperAchievementModel::MakePlatformUnlockKey(SteamPlatform, TEXT("ACH_KILL_ONE"))));
	return true;
}

#endif
