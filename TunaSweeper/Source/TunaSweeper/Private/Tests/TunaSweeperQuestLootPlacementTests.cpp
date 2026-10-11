#if WITH_DEV_AUTOMATION_TESTS
#include "Subsystem/TunaSweeperRaidPlacementSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestLootPlacementTest,
	"TunaSweeper.RaidPlacement.QuestLootCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestLootPlacementTest::RunTest(const FString& Parameters)
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	UGameInstance* Game = NewObject<UGameInstance>(GEngine);
	FContextAccess::Attach(Game, &Context);
	Context.OwningGameInstance = Game;
	World->SetGameInstance(Game);
	Game->Init();
	ON_SCOPE_EXIT { Game->Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game, nullptr); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Raid = Game->GetSubsystem<UTunaSweeperRaidPlacementSubsystem>();
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	if (!TestNotNull(TEXT("Raid subsystem"), Raid) || !TestNotNull(TEXT("Quest subsystem"), Quests)) return false;

	// Exercise the real JSON loader without writing project data or player saves.
	const FString File = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("QuestLoot"), TEXT(".json"));
	ON_SCOPE_EXIT { IFileManager::Get().Delete(*File); };
	const FString Row = TEXT(R"({"level_name":"RaidMap","placement_id":1900,"loot_container_class":"/Script/TunaSweeper.TunaSweeperLootContainerActor","container_definition_id":7009,"contents_id":8013,"spawn_chance":0)" );
	FFileHelper::SaveStringToFile(TEXT("[") + Row + TEXT(",\"required_active_quest_id\":900001}]"), *File);
	TestTrue(TEXT("Quest condition JSON loads"), Raid->LoadLootPlacements(File));
	TestEqual(TEXT("One quest placement parsed"), Raid->LootPlacementDefinitions.Num(), 1);
	if (Raid->LootPlacementDefinitions.Num() != 1) return false;
	const auto Placement = Raid->LootPlacementDefinitions[0];
	TestEqual(TEXT("Quest ID remains an integer"), Placement.RequiredActiveQuestId, 900001);
	for (const FString Bad : {TEXT("0"), TEXT("-1"), TEXT("1.5"), TEXT("2147483648"), TEXT("\"900001\""), TEXT("null")})
	{
		Raid->LootPlacementDefinitions.Reset();
		FFileHelper::SaveStringToFile(TEXT("[") + Row + TEXT(",\"required_active_quest_id\":") + Bad + TEXT("}]"), *File);
		AddExpectedError(TEXT("invalid required_active_quest_id"), EAutomationExpectedErrorFlags::Contains, 1);
		Raid->LoadLootPlacements(File);
		TestEqual(TEXT("Invalid ID cannot degrade to an ordinary loot spawn"), Raid->LootPlacementDefinitions.Num(), 0);
	}
	FTunaSweeperQuestDefinition Definition;
	Definition.QuestId = 900001;
	FTunaSweeperObjectiveDefinition Objective;
	Objective.ObjectiveId = TEXT("deliver"); Objective.Type = ETunaSweeperObjectiveType::InteractionCompleted; Objective.RequiredCount = 1;
	Definition.Objectives.Add(Objective);
	Quests->QuestDefinitions.Reset(); Quests->QuestDefinitions.Add(Definition.QuestId, Definition);
	Quests->bQuestDataLoaded = true;
	TestFalse(TEXT("Not accepted means absent"), Raid->IsQuestLootActive(Definition.QuestId));
	Raid->LastSpawnedWorld = World;
	auto* Anchor = World->SpawnActor<ATunaSweeperRaidPlacementAnchor>();
	auto& Runtime = Raid->QuestLootPlacements.AddDefaulted_GetRef();
	Runtime.Definition = Placement; Runtime.Anchor = Anchor;
	Raid->RefreshQuestLootPlacements();
	TestFalse(TEXT("No actor is created before acceptance"), Runtime.Container.IsValid());
	FTunaSweeperNumericQuestProgressSaveData Progress; Progress.QuestId = Definition.QuestId;
	Progress.State = ETunaSweeperQuestState::Accepted;
	Quests->LoadQuestProgressFromSave({Progress}, 0, 0);
	TestTrue(TEXT("Accepted bypasses zero percent spawn chance"), Raid->IsQuestLootActive(Definition.QuestId));
	auto* Container = Runtime.Container.Get();
	if (!TestNotNull(TEXT("Load/quest event creates the active parcel"), Container)) return false;
	TestFalse(TEXT("Accepted parcel is visible"), Container->IsHidden());
	for (auto State : {ETunaSweeperQuestState::RewardAvailable, ETunaSweeperQuestState::RewardCompleted, ETunaSweeperQuestState::Available})
	{
		Progress.State = State; Quests->LoadQuestProgressFromSave({Progress}, 0, 0);
		TestTrue(TEXT("Inactive parcel hides on progress event"), Container->IsHidden());
		TestFalse(TEXT("Hidden parcel collision disabled"), Container->GetActorEnableCollision());
	}
	Progress.State = ETunaSweeperQuestState::Accepted; Quests->LoadQuestProgressFromSave({Progress}, 0, 0);
	TestEqual(TEXT("Reactivation retains original container rather than refilling"), Runtime.Container.Get(), Container);
	TestFalse(TEXT("Reactivation shows same parcel"), Container->IsHidden());
	Quests->QuestDefinitions.Reset(); Quests->OnQuestProgressChanged.Broadcast();
	TestTrue(TEXT("Unknown quest fails closed"), Container->IsHidden());
	TestFalse(TEXT("Ordinary zero percent spawn stays disabled"), Raid->ShouldSpawnAtPlacement(1900, 0));
	TestTrue(TEXT("Ordinary guaranteed spawn stays enabled"), Raid->ShouldSpawnAtPlacement(1900, 1));
	return true;
}
#endif
