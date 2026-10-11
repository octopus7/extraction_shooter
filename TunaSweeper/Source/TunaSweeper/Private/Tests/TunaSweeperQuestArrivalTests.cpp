#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "UObject/UObjectGlobals.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/TunaSweeperQuestArrivalTrigger.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"

namespace TunaSweeperQuestData
{
	bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestArrivalContractTest,
	"TunaSweeper.Quest.Arrival.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestArrivalContractTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("An independently placeable arrival trigger is available"),
		FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperQuestArrivalTrigger")));
	TestTrue(TEXT("Location arrival is a distinct quest objective"),
		StaticEnum<ETunaSweeperObjectiveType>()->GetValueByNameString(TEXT("LocationReached")) != INDEX_NONE);
	TestTrue(TEXT("Mounted arrival is distinct from walking arrival"),
		StaticEnum<ETunaSweeperObjectiveType>()->GetValueByNameString(TEXT("VehicleLocationReached")) != INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestArrivalDataTest,
	"TunaSweeper.Quest.Arrival.DataValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestArrivalDataTest::RunTest(const FString& Parameters)
{
	auto Parse = [](const FString& Fields, FTunaSweeperObjectiveDefinition& Out)
	{
		TSharedPtr<FJsonObject> Json;
		const FString Text = TEXT("{\"objective_id\":\"visit\",") + Fields + TEXT("}");
		return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) &&
			TunaSweeperQuestData::ParseObjective(Json, Out);
	};
	for (const FString Type : {TEXT("location_reached"), TEXT("vehicle_location_reached")})
	{
		const FString Prefix = TEXT("\"type\":\"") + Type + TEXT("\"");
		FTunaSweeperObjectiveDefinition Objective;
		TestFalse(TEXT("Unspecified destination cannot match every station"), Parse(Prefix, Objective));
		TestFalse(TEXT("Empty destination rejected"), Parse(Prefix + TEXT(",\"location_id\":\"\""), Objective));
		TestFalse(TEXT("Numeric destination rejected"), Parse(Prefix + TEXT(",\"location_id\":17"), Objective));
		TestFalse(TEXT("Boolean destination rejected"), Parse(Prefix + TEXT(",\"location_id\":true"), Objective));
		TestTrue(TEXT("Destination with implicit one-visit count loads"),
			Parse(Prefix + TEXT(",\"location_id\":\"test.station.east\""), Objective));
		TestEqual(TEXT("Destination ID survives JSON parsing"), Objective.LocationId, FName(TEXT("test.station.east")));
		TestEqual(TEXT("Arrival type survives parsing"), Objective.Type, Type == TEXT("location_reached")
			? ETunaSweeperObjectiveType::LocationReached : ETunaSweeperObjectiveType::VehicleLocationReached);
		TestTrue(TEXT("Explicit one visit loads"), Parse(Prefix + TEXT(",\"location_id\":\"east\",\"required_count\":1"), Objective));
		for (const FString Count : {TEXT("0"), TEXT("2"), TEXT("1.5"), TEXT("-1"), TEXT("\"1\""), TEXT("null"), TEXT("true")})
		{
			TestFalse(TEXT("Arrival cannot be completed by repeated visits or malformed counts"),
				Parse(Prefix + TEXT(",\"location_id\":\"east\",\"required_count\":") + Count, Objective));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestArrivalTest,
	"TunaSweeper.Quest.Arrival.PlayerVehicleAndPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestArrivalTest::RunTest(const FString& Parameters)
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
	FContextAccess::Attach(Game, &Context);
	Context.OwningGameInstance = Game;
	World->SetGameInstance(Game);
	// Use real subsystems, but prevent access to the user's loadout and save files.
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game, nullptr); World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
	};
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	if (!TestNotNull(TEXT("Quest subsystem"), Quests)) return false;
	Quests->QuestDefinitions.Reset();
	Quests->QuestProgressById.Reset();
	Quests->bQuestDataLoaded = true;

	auto Objective = [](FName Id, FName Location, bool Vehicle = false)
	{
		FTunaSweeperObjectiveDefinition Result;
		Result.ObjectiveId = Id; Result.LocationId = Location;
		Result.Type = Vehicle ? ETunaSweeperObjectiveType::VehicleLocationReached : ETunaSweeperObjectiveType::LocationReached;
		return Result;
	};
	FTunaSweeperQuestDefinition Stations;
	Stations.QuestId = 900101;
	Stations.Objectives = {Objective(TEXT("east"), TEXT("test.east")), Objective(TEXT("west"), TEXT("test.west"))};
	Quests->QuestDefinitions.Add(Stations.QuestId, Stations);
	FTunaSweeperQuestDefinition Drive;
	Drive.QuestId = 900102; Drive.Objectives = {Objective(TEXT("drive"), TEXT("test.north"), true)};
	Quests->QuestDefinitions.Add(Drive.QuestId, Drive);
	FTunaSweeperQuestDefinition Fuel;
	Fuel.QuestId = 900103; Fuel.bCompleteOnAccept = true;
	Quests->QuestDefinitions.Add(Fuel.QuestId, Fuel);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Player = NewObject<ULocalPlayer>(GEngine);
	// This synthetic world does not run actor initialization/BeginPlay. Register the
	// controller as PostInitializeComponents does in a real game world.
	World->AddController(Controller);
	Controller->Possess(Player);
	TestTrue(TEXT("Fixture has an actual local player"), Player->IsLocallyControlled());
	auto* East = World->SpawnActor<ATunaSweeperQuestArrivalTrigger>(); East->LocationId = TEXT("test.east");
	auto* West = World->SpawnActor<ATunaSweeperQuestArrivalTrigger>(); West->LocationId = TEXT("test.west");
	West->SetActorLocation(FVector(2000, 0, 0));
	auto* North = World->SpawnActor<ATunaSweeperQuestArrivalTrigger>(); North->LocationId = TEXT("test.north");
	North->bRequireMountedVehicle = true;
	North->RequiredCompletedQuestIds = {Fuel.QuestId};
	North->RequiredCompletedProgressIds = {TEXT("test.vehicle.repaired"), TEXT("test.vehicle.fueled")};
	North->RequiredVehicleTag = TEXT("test.quest.vehicle");
	East->Tick(0.016f);
	TestEqual(TEXT("Arrival before acceptance gives no credit"), Quests->GetQuestState(Stations.QuestId), ETunaSweeperQuestState::Available);
	Quests->AcceptQuest(Stations.QuestId);
	Player->SetActorLocation(FVector(1000, 0, 0)); East->CheckArrivals();
	TestEqual(TEXT("Outside area is not arrival"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("east")), 0);
	Player->SetActorLocation(FVector::ZeroVector);
	East->SetActorScale3D(FVector::ZeroVector); East->CheckArrivals();
	TestEqual(TEXT("Collapsed volume cannot detect arrival"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("east")), 0);
	East->SetActorScale3D(FVector::OneVector);
	Controller->UnPossess(); East->CheckArrivals();
	TestEqual(TEXT("Uncontrolled character/NPC is not arrival"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("east")), 0);
	Controller->Possess(Player);
	// Polling catches acceptance/load while already inside without another overlap event.
	East->Tick(0.016f); East->Tick(0.016f); West->Tick(0.016f);
	TestEqual(TEXT("East is counted exactly once"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("east")), 1);
	TestEqual(TEXT("East does not complete west"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("west")), 0);
	TestEqual(TEXT("Both stations are required"), Quests->GetQuestState(Stations.QuestId), ETunaSweeperQuestState::Accepted);
	TArray<FTunaSweeperNumericQuestProgressSaveData> Saved;
	int32 Tracked = 0, Coins = 0;
	Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
	Quests->LoadQuestProgressFromSave(Saved, Tracked, Coins);
	East->Destroy(); East = World->SpawnActor<ATunaSweeperQuestArrivalTrigger>(); East->LocationId = TEXT("test.east");
	East->Tick(0.016f);
	TestEqual(TEXT("Recreated trigger and loaded quest retain one east visit"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("east")), 1);
	West->ArrivalArea->SetBoxExtent(FVector(200, 50, 100));
	West->SetActorRotation(FRotator(0, 90, 0)); West->SetActorScale3D(FVector(2, 1, 1));
	Player->SetActorLocation(FVector(2100, 0, 0)); West->CheckArrivals();
	TestEqual(TEXT("Rotated narrow side excludes player"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("west")), 0);
	Player->SetActorLocation(FVector(2000, 350, 150)); West->CheckArrivals();
	TestEqual(TEXT("Wrong floor is excluded"), Quests->GetObjectiveProgressCount(Stations.QuestId, TEXT("west")), 0);
	Player->SetActorLocation(FVector(2000, 350, 0)); West->CheckArrivals();
	TestEqual(TEXT("Rotated and scaled destination completes remaining station"), Quests->GetQuestState(Stations.QuestId), ETunaSweeperQuestState::RewardAvailable);
	Quests->ClaimQuestReward(Stations.QuestId); West->CheckArrivals();
	TestEqual(TEXT("Revisit preserves claimed reward"), Quests->GetQuestState(Stations.QuestId), ETunaSweeperQuestState::RewardCompleted);

	Player->SetActorLocation(FVector(0, 150, 90));
	Quests->AcceptQuest(Drive.QuestId);
	North->CheckArrivals();
	TestEqual(TEXT("Walking to vehicle destination is insufficient"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>(FVector(0, 0, 45), FRotator::ZeroRotator, Spawn);
	ATV->MountComponent->EngineStartSound = nullptr;
	ATV->MountComponent->EngineIdleSound = nullptr;
	ATV->MountComponent->EngineStopSound = nullptr;
	ATV->MountComponent->VehicleHudClass.Reset();
	TestTrue(TEXT("Actual player attaches to real vehicle"), ATV->MountComponent->TryMount(Player));
	North->CheckArrivals();
	TestEqual(TEXT("Missing prerequisites prevent vehicle credit"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	Quests->AcceptQuest(Fuel.QuestId);
	North->CheckArrivals();
	TestEqual(TEXT("Reward pending is not completed prerequisite"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	Quests->ClaimQuestReward(Fuel.QuestId);
	Game->UpdateWorldProgressState(TEXT("test.vehicle.repaired"), NAME_None, ETunaSweeperWorldProgressState::Completed, 1, 1, false);
	North->CheckArrivals();
	TestEqual(TEXT("Missing fuel progress blocks arrival"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	Game->UpdateWorldProgressState(TEXT("test.vehicle.fueled"), NAME_None, ETunaSweeperWorldProgressState::InProgress, 0, 1, false);
	North->CheckArrivals();
	TestEqual(TEXT("Incomplete fuel progress blocks arrival"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	Game->UpdateWorldProgressState(TEXT("test.vehicle.fueled"), NAME_None, ETunaSweeperWorldProgressState::Completed, 1, 1, false);
	North->CheckArrivals();
	TestEqual(TEXT("Different vehicle identity blocks arrival"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	ATV->Tags.Add(TEXT("test.quest.vehicle"));
	ATV->bVehicleDestroyed = true; North->CheckArrivals();
	TestEqual(TEXT("Destroyed vehicle cannot arrive"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	ATV->bVehicleDestroyed = false;
	North->RequiredCompletedQuestIds.Add(0); North->CheckArrivals();
	TestEqual(TEXT("Invalid prerequisite fails closed"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	North->RequiredCompletedQuestIds.Pop();
	North->RequiredCompletedProgressIds.Add(NAME_None); North->CheckArrivals();
	TestEqual(TEXT("Empty progress gate fails closed"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	North->RequiredCompletedProgressIds.Pop();
	ATV->MountComponent->ReleaseRiderForEndPlay();
	Player->SetActorLocation(FVector(0, 150, 90)); North->CheckArrivals();
	TestEqual(TEXT("Walking still fails with every prerequisite satisfied"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	TestTrue(TEXT("Mounting inside the arrival area works"), ATV->MountComponent->TryMount(Player));
	North->bRequireMountedVehicle = false; North->CheckArrivals();
	TestEqual(TEXT("Normal trigger cannot bypass vehicle mode and its gates"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	North->bRequireMountedVehicle = true;
	Controller->UnPossess(); North->CheckArrivals();
	TestEqual(TEXT("Empty/uncontrolled vehicle gives no credit"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	Controller->Possess(Player);
	ATV->SetActorLocation(FVector(2000, 0, 45), false, nullptr, ETeleportType::TeleportPhysics); North->CheckArrivals();
	TestEqual(TEXT("Mounted player outside area gives no credit"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 0);
	ATV->SetActorLocation(FVector(0, 0, 45), false, nullptr, ETeleportType::TeleportPhysics);
	North->Tick(0.016f); North->Tick(0.016f);
	TestEqual(TEXT("Correct mounted arrival completes drive"), Quests->GetQuestState(Drive.QuestId), ETunaSweeperQuestState::RewardAvailable);
	TestEqual(TEXT("Vehicle arrival counted once"), Quests->GetObjectiveProgressCount(Drive.QuestId, TEXT("drive")), 1);
	Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
	Quests->LoadQuestProgressFromSave(Saved, Tracked, Coins);
	TestEqual(TEXT("Vehicle completion survives load"), Quests->GetQuestState(Drive.QuestId), ETunaSweeperQuestState::RewardAvailable);
	return true;
}
#endif
