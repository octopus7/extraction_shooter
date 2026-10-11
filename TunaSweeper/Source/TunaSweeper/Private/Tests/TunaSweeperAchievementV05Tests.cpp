#if WITH_DEV_AUTOMATION_TESTS
#include "Achievement/TunaSweeperAchievementModel.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperSafeSave.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

namespace TunaAchievementV05Tests
{
	constexpr auto Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	FTunaSweeperAchievementDefinition Definition(const TCHAR* Id, ETunaSweeperAchievementConditionType Type, int64 Count = 1)
	{
		FTunaSweeperAchievementDefinition Result;
		Result.AchievementId = FName(Id);
		Result.ConditionType = Type;
		Result.RequiredCount = Count;
		return Result;
	}
	FString FixturePath() { return FPaths::ProjectSavedDir() / TEXT("Automation/AchievementsV05Restart.sav"); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementExtractionBoundaries, "TunaSweeper.Achievement.ExtractionBoundaries", TunaAchievementV05Tests::Flags)
bool FTunaAchievementExtractionBoundaries::RunTest(const FString& Parameters)
{
	using namespace TunaAchievementV05Tests;
	const TArray<FTunaSweeperAchievementDefinition> Definitions = {
		Definition(TEXT("11"), ETunaSweeperAchievementConditionType::ExtractionHealth),
		Definition(TEXT("12"), ETunaSweeperAchievementConditionType::TimedExtraction, 5)};
	const auto Check = [&](bool Success, float Health, float MaxHealth, int32 Kills, double Seconds, bool Low, bool Timed)
	{
		FTunaSweeperAchievementProgressState State;
		FTunaSweeperAchievementSnapshot Snapshot;
		Snapshot.bSuccessfulExtraction = Success; Snapshot.Health = Health; Snapshot.MaxHealth = MaxHealth;
		Snapshot.RaidKills = Kills; Snapshot.RaidSeconds = Seconds;
		TArray<FName> Unlocked;
		TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, Unlocked, Snapshot);
		TestEqual(TEXT("Health predicate"), State.UnlockedAchievementIds.Contains(FName(TEXT("11"))), Low);
		TestEqual(TEXT("Same-raid time predicate"), State.UnlockedAchievementIds.Contains(FName(TEXT("12"))), Timed);
		TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, State, Unlocked, Snapshot);
		TestTrue(TEXT("No duplicate unlock notifications"), Unlocked.IsEmpty());
	};
	Check(true, 10, 100, 5, 295, true, true);
	Check(true, 10, 100, 5, 305, true, true);
	Check(true, 9.99f, 100, 5, 294.999, true, false);
	Check(true, 10.01f, 100, 5, 305.001, false, false);
	Check(true, 80, 100, 4, 300, false, false); // healed before extraction, insufficient kills
	Check(false, 1, 100, 5, 300, false, false); // request/failure never qualifies
	Check(true, 0, 100, 0, 0, false, false);
	Check(true, 10, 0, 0, 0, false, false);
	Check(true, std::numeric_limits<float>::quiet_NaN(), 100, 5, std::numeric_limits<double>::quiet_NaN(), false, false);
	FTunaSweeperAchievementRaidState Raid;
	Raid.Begin(40);
	for (int32 Index = 0; Index < 5; ++Index) Raid.RecordKill();
	TestEqual(TEXT("Same raid tracks kills"), Raid.Kills, 5);
	Raid.Reset(); Raid.RecordKill();
	TestEqual(TEXT("Failed raid does not retain kills"), Raid.Kills, 0);
	Raid.Begin(900);
	TestEqual(TEXT("Next raid starts empty"), Raid.Kills, 0);
	TestEqual(TEXT("Next raid replaces time origin"), Raid.StartedAt, 900.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementSnapshots, "TunaSweeper.Achievement.CurrentStateAndFacts", TunaAchievementV05Tests::Flags)
bool FTunaAchievementSnapshots::RunTest(const FString& Parameters)
{
	using namespace TunaAchievementV05Tests;
	auto Rich = Definition(TEXT("17"), ETunaSweeperAchievementConditionType::CurrentItemQuantity, 100);
	Rich.TargetItemId = 3004;
	auto Home = Definition(TEXT("05"), ETunaSweeperAchievementConditionType::AllFacilitiesRestored);
	auto Lift = Definition(TEXT("04"), ETunaSweeperAchievementConditionType::GameplayEvent);
	Lift.EventType = ETunaSweeperAchievementEvent::FacilityUsed; Lift.TargetId = TEXT("test.lift");
	FTunaSweeperAchievementProgressState State;
	FTunaSweeperAchievementSnapshot Snapshot;
	TArray<FName> Unlocked;
	Snapshot.OwnedItemCounts.Add(3004, 60);
	TunaSweeperAchievementModel::EvaluateDefinitions({Rich, Home, Lift}, State, Unlocked, Snapshot);
	Snapshot.OwnedItemCounts[3004] = 40;
	TunaSweeperAchievementModel::EvaluateDefinitions({Rich, Home, Lift}, State, Unlocked, Snapshot);
	TestTrue(TEXT("Separate holdings never accumulate; empty facility list never unlocks"), Unlocked.IsEmpty());
	Snapshot.OwnedItemCounts[3004] = 99;
	TunaSweeperAchievementModel::EvaluateDefinitions({Rich}, State, Unlocked, Snapshot);
	TestTrue(TEXT("99 remains locked"), Unlocked.IsEmpty());
	Snapshot.OwnedItemCounts[3004] = 100;
	TunaSweeperAchievementModel::EvaluateDefinitions({Rich}, State, Unlocked, Snapshot);
	TestTrue(TEXT("100 simultaneous unlocks"), Unlocked.Contains(FName(TEXT("17"))));
	Home.RequiredFacilityIds = {TEXT("test.facility.a"), TEXT("test.facility.b")};
	Snapshot.RestoredFacilityIds.Add(TEXT("test.facility.a"));
	TunaSweeperAchievementModel::EvaluateDefinitions({Home}, State, Unlocked, Snapshot);
	TestTrue(TEXT("Partial repairs remain locked"), Unlocked.IsEmpty());
	Snapshot.RestoredFacilityIds.Add(TEXT("test.facility.b"));
	TunaSweeperAchievementModel::EvaluateDefinitions({Home}, State, Unlocked, Snapshot);
	TestTrue(TEXT("Complete configured list unlocks"), Unlocked.Contains(FName(TEXT("05"))));
	TunaSweeperAchievementModel::RecordQuestRewardClaimed(State, 4);
	TunaSweeperAchievementModel::RecordGameplayEvent(State, ETunaSweeperAchievementEvent::FacilityRelocated, TEXT("test.lift"));
	TunaSweeperAchievementModel::EvaluateDefinitions({Lift}, State, Unlocked, Snapshot);
	TestTrue(TEXT("Quest rewards or other facts do not imply lift use"), Unlocked.IsEmpty());
	TestFalse(TEXT("Unset event target rejected"), TunaSweeperAchievementModel::RecordGameplayEvent(State, ETunaSweeperAchievementEvent::FacilityUsed, NAME_None));
	TunaSweeperAchievementModel::RecordGameplayEvent(State, ETunaSweeperAchievementEvent::FacilityUsed, TEXT("test.lift"));
	Lift.bEnabled = false;
	TunaSweeperAchievementModel::EvaluateDefinitions({Lift}, State, Unlocked, Snapshot);
	TestTrue(TEXT("Disabled definition cannot unlock"), Unlocked.IsEmpty());
	Lift.bEnabled = true;
	TunaSweeperAchievementModel::EvaluateDefinitions({Lift}, State, Unlocked, Snapshot);
	TestTrue(TEXT("Actual use unlocks"), Unlocked.Contains(FName(TEXT("04"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementCatalog, "TunaSweeper.Achievement.CatalogAndLocalization", TunaAchievementV05Tests::Flags)
bool FTunaAchievementCatalog::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
	TStrongObjectPtr<UTunaSweeperAchievementSubsystem> Achievements(NewObject<UTunaSweeperAchievementSubsystem>(Owner.Get()));
	TStrongObjectPtr<UTunaSweeperTextSubsystem> Text(NewObject<UTunaSweeperTextSubsystem>(Owner.Get()));
	if (!TestTrue(TEXT("Production catalog loads"), Achievements->LoadAchievementDefinitions())) return false;
	const auto Definitions = Achievements->GetAchievementDefinitions();
	TestEqual(TEXT("20 fixed slots"), Definitions.Num(), 20);
	const TCHAR* Names[][3] = {
		{TEXT("첫 외출"), TEXT("First Outing"), TEXT("初めてのお出かけ")},
		{TEXT("붉은 신고식"), TEXT("First Blood"), TEXT("紅の洗礼")},
		{TEXT("새로운 가족"), TEXT("New Family"), TEXT("新しい家族")},
		{TEXT("발사"), TEXT("Blast Off!"), TEXT("発射！")},
		{TEXT("아늑한 집"), TEXT("Home Sweet Home"), TEXT("居心地のいい我が家")},
		{TEXT("골목대장"), TEXT("King of the Block"), TEXT("路地裏の番長")},
		{TEXT("마을"), TEXT("The Town"), TEXT("街")},
		{TEXT("또 다른 가족?"), TEXT("Another Family Member?"), TEXT("また家族が増えた？")},
		{TEXT("문명의 맛"), TEXT("Now That’s Civilization!"), TEXT("これが文明か！")},
		{TEXT("토끼사냥"), TEXT("Rabbit Hunt"), TEXT("ウサギ狩り")},
		{TEXT("구사일생"), TEXT("Close Call"), TEXT("九死に一生")},
		{TEXT("퇴근합니다"), TEXT("Clocking Out"), TEXT("定時で帰ります！")},
		{TEXT("미지와 조우"), TEXT("Encounter with the Unknown"), TEXT("未知との遭遇")},
		{TEXT(""), TEXT(""), TEXT("")},
		{TEXT("요술봉"), TEXT("Magic Wand"), TEXT("魔法のステッキ")},
		{TEXT("안전운전"), TEXT("Drive Safely"), TEXT("安全運転")},
		{TEXT("부자"), TEXT("Rich"), TEXT("お金持ち")},
		{TEXT("동네최강"), TEXT("Top Dog"), TEXT("町内最強")},
		{TEXT("황무지"), TEXT("Wasteland"), TEXT("荒野")},
		{TEXT("결말"), TEXT("The End"), TEXT("結末")}};
	const ETunaSweeperItemTextLanguage Languages[] = {ETunaSweeperItemTextLanguage::Korean, ETunaSweeperItemTextLanguage::English, ETunaSweeperItemTextLanguage::Japanese};
	for (int32 Index = 0; Index < Definitions.Num(); ++Index)
	{
		TestEqual(TEXT("Fixed ID order"), Definitions[Index].AchievementId.ToString(), FString::Printf(TEXT("%02d"), Index + 1));
		if (Index == 13) { TestFalse(TEXT("Reserved slot disabled"), TunaSweeperAchievementModel::CanUnlock(Definitions[Index])); continue; }
		for (int32 Language = 0; Language < 3; ++Language)
		{
			FText Resolved;
			TestTrue(TEXT("Localized name resolves through existing table"), Text->TryGetTextByKey(Definitions[Index].NameStringKey, Languages[Language], Resolved));
			TestEqual(TEXT("Exact v0.5 spelling"), Resolved.ToString(), FString(Names[Index][Language]));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementPersistence, "TunaSweeper.Achievement.PersistenceRoundTrip", TunaAchievementV05Tests::Flags)
bool FTunaAchievementPersistence::RunTest(const FString& Parameters)
{
	using namespace TunaAchievementV05Tests;
	FTunaSweeperAchievementProgressState State;
	for (int32 Index = 0; Index < 100; ++Index) TunaSweeperAchievementModel::RecordEnemyKilled(State, TEXT("test.enemy"));
	TArray<FName> Unlocked;
	TunaSweeperAchievementModel::EvaluateDefinitions({Definition(TEXT("18"), ETunaSweeperAchievementConditionType::TotalEnemyKills, 100)}, State, Unlocked);
	TunaSweeperAchievementModel::RecordGameplayEvent(State, ETunaSweeperAchievementEvent::FacilityUsed, TEXT("test.lift"));
	State.UnlockedAchievementIds.Add(TEXT("14")); // sanitize legacy/malformed saves
	TStrongObjectPtr<UTunaSweeperAchievementSaveGame> Save(NewObject<UTunaSweeperAchievementSaveGame>());
	Save->SaveVersion = 2;
	TunaSweeperAchievementModel::CaptureProgress(State, *Save);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FixturePath()), true);
	const auto Validator = [](const USaveGame& Candidate) { return Candidate.IsA<UTunaSweeperAchievementSaveGame>(); };
	if (!TestTrue(TEXT("Safe-save fixture writes"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save.Get(), FixturePath(), Validator))) return false;
	TStrongObjectPtr<UTunaSweeperAchievementSaveGame> Loaded(Cast<UTunaSweeperAchievementSaveGame>(TunaSweeperSafeSave::LoadSaveFileWithRecovery(FixturePath(), {}, Validator)));
	if (!TestNotNull(TEXT("Safe-save fixture reloads"), Loaded.Get())) return false;
	FTunaSweeperAchievementProgressState Restored;
	TunaSweeperAchievementModel::RestoreProgress(*Loaded, Restored);
	TestEqual(TEXT("Cumulative kills survive serialization"), Restored.TotalEnemyKills, int64(100));
	TestTrue(TEXT("Unlock survives serialization"), Restored.UnlockedAchievementIds.Contains(TEXT("18")));
	TestFalse(TEXT("Reserved unlock discarded"), Restored.UnlockedAchievementIds.Contains(TEXT("14")));
	TestTrue(TEXT("Actual-use fact survives"), Restored.GameplayFacts.Contains(TunaSweeperAchievementModel::MakeGameplayFactKey(ETunaSweeperAchievementEvent::FacilityUsed, TEXT("test.lift"))));
	TunaSweeperAchievementModel::EvaluateDefinitions({Definition(TEXT("18"), ETunaSweeperAchievementConditionType::TotalEnemyKills, 100)}, Restored, Unlocked);
	TestTrue(TEXT("Restoring does not notify twice"), Unlocked.IsEmpty());
	Loaded->SaveVersion = 1; Loaded->GameplayFacts.Reset();
	TunaSweeperAchievementModel::RestoreProgress(*Loaded, Restored);
	TestEqual(TEXT("v1 total is preserved"), Restored.TotalEnemyKills, int64(100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementRestartRead, "TunaSweeper.AchievementRestart.Read", TunaAchievementV05Tests::Flags)
bool FTunaAchievementRestartRead::RunTest(const FString& Parameters)
{
	const auto Validator = [](const USaveGame& Candidate) { return Candidate.IsA<UTunaSweeperAchievementSaveGame>(); };
	TStrongObjectPtr<UTunaSweeperAchievementSaveGame> Save(Cast<UTunaSweeperAchievementSaveGame>(
		TunaSweeperSafeSave::LoadSaveFileWithRecovery(TunaAchievementV05Tests::FixturePath(), {}, Validator)));
	if (!TestNotNull(TEXT("Previous process fixture exists"), Save.Get())) return false;
	FTunaSweeperAchievementProgressState State;
	TunaSweeperAchievementModel::RestoreProgress(*Save, State);
	TestEqual(TEXT("Restart cumulative kills"), State.TotalEnemyKills, int64(100));
	TestTrue(TEXT("Restart unlock state"), State.UnlockedAchievementIds.Contains(TEXT("18")));
	TestFalse(TEXT("Restart reserved state"), State.UnlockedAchievementIds.Contains(TEXT("14")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementMigration, "TunaSweeper.Achievement.VersionOneUpgrade", TunaAchievementV05Tests::Flags)
bool FTunaAchievementMigration::RunTest(const FString& Parameters)
{
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Automation") / (FGuid::NewGuid().ToString() + TEXT(".sav"));
	ON_SCOPE_EXIT { TunaSweeperSafeSave::DeleteSaveArtifacts(Path); };
	TStrongObjectPtr<UTunaSweeperAchievementSaveGame> Save(NewObject<UTunaSweeperAchievementSaveGame>());
	Save->SaveVersion = 1; Save->BuildFlavor = TEXT("test"); Save->DistributionNamespace = TEXT("test");
	Save->TotalEnemyKills = 99; Save->UnlockedAchievementIds = {TEXT("02")};
	const auto Validator = [](const USaveGame& Candidate) { return TunaSweeperAchievementModel::IsSupportedSave(Candidate, TEXT("test"), TEXT("test")); };
	if (!TestTrue(TEXT("Legacy v1 is accepted on disk"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save.Get(), Path, Validator))) return false;
	Save->SaveVersion = 2; Save->TotalEnemyKills = 100; Save->UnlockedAchievementIds.Add(TEXT("18"));
	if (!TestTrue(TEXT("v2 replaces existing v1 through fail-closed path"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save.Get(), Path, Validator))) return false;
	TStrongObjectPtr<UTunaSweeperAchievementSaveGame> Loaded(Cast<UTunaSweeperAchievementSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator)));
	if (!TestNotNull(TEXT("Upgraded disk save loads"), Loaded.Get())) return false;
	TestEqual(TEXT("Upgraded version"), Loaded->SaveVersion, 2);
	TestEqual(TEXT("New cumulative count persisted"), Loaded->TotalEnemyKills, int64(100));
	TestTrue(TEXT("New unlock persisted"), Loaded->UnlockedAchievementIds.Contains(TEXT("18")));
	TestFalse(TEXT("Wrong account/build namespace rejected"), TunaSweeperAchievementModel::IsSupportedSave(*Loaded, TEXT("other"), TEXT("test")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementLifecycleTest, "TunaSweeper.Achievement.ExtractionLifecycle", TunaAchievementV05Tests::Flags)
bool FTunaAchievementLifecycleTest::RunTest(const FString& Parameters)
{
	// The transient instance is deliberately not initialized: no real player saves,
	// online subsystem, or toast widgets participate in this lifecycle fixture.
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("AchievementDestination"));
	if (!TestNotNull(TEXT("Transient destination world"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>());
	FContextAccess::Attach(Instance.Get(), &Context);
	World->SetGameInstance(Instance.Get());
	ON_SCOPE_EXIT
	{
		World->SetGameInstance(nullptr); FContextAccess::Attach(Instance.Get(), nullptr);
		World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
	};
	TStrongObjectPtr<UTunaSweeperAchievementSubsystem> Subject(NewObject<UTunaSweeperAchievementSubsystem>(Instance.Get()));
	TestTrue(TEXT("Fixture permits gameplay reporting"), Subject->IsGameplayReportingAllowed());
	Subject->bRefreshingSnapshot = true; // isolate extraction from inventory initialization
	Subject->Definitions = {
		TunaAchievementV05Tests::Definition(TEXT("11"), ETunaSweeperAchievementConditionType::ExtractionHealth),
		TunaAchievementV05Tests::Definition(TEXT("12"), ETunaSweeperAchievementConditionType::TimedExtraction, 5)};
	const auto CompletedHold = [&]()
	{
		// CreateWorld's object name does not name its generated outer package.
		Subject->PendingExtractionDestination = FName(*World->GetMapName());
		Subject->PendingExtraction.Health = 10; Subject->PendingExtraction.MaxHealth = 100;
		Subject->PendingExtraction.RaidSeconds = 295; Subject->PendingExtraction.RaidKills = 5;
		Subject->GameplayWorld.Reset();
	};
	CompletedHold();
	Subject->DiscardPendingExtraction(World); // same handler used by OnTravelFailure and non-extraction travel
	Subject->HandleGameplayWorldReady(World);
	TestFalse(TEXT("Failed travel then unrelated arrival cannot unlock health"), Subject->IsAchievementUnlocked(TEXT("11")));
	TestFalse(TEXT("Failed travel then unrelated arrival cannot unlock time"), Subject->IsAchievementUnlocked(TEXT("12")));
	CompletedHold();
	Subject->CancelRaid(); // death
	Subject->HandleGameplayWorldReady(World);
	TestFalse(TEXT("Death then rescue cannot count as successful extraction"), Subject->IsAchievementUnlocked(TEXT("11")));
	CompletedHold();
	Subject->PendingExtractionDestination = TEXT("OtherDestination");
	Subject->HandleGameplayWorldReady(World);
	TestFalse(TEXT("Wrong destination does not commit"), Subject->IsAchievementUnlocked(TEXT("11")));
	CompletedHold();
	Subject->HandleGameplayWorldReady(World);
	TestTrue(TEXT("Confirmed arrival commits sampled health"), Subject->IsAchievementUnlocked(TEXT("11")));
	TestTrue(TEXT("Confirmed arrival commits completed-hold time"), Subject->IsAchievementUnlocked(TEXT("12")));
	TestTrue(TEXT("Arrival consumes pending evidence"), Subject->PendingExtractionDestination.IsNone());
	TestFalse(TEXT("Non-raid destination leaves no active raid"), Subject->RaidState.bActive);
	TestEqual(TEXT("Exactly one notification queued per local unlock"), Subject->PendingNotifications.Num(), 2);
	Subject->HandleGameplayWorldReady(World);
	TestEqual(TEXT("Duplicate world-ready event does not notify twice"), Subject->PendingNotifications.Num(), 2);
	return true;
}
#endif
