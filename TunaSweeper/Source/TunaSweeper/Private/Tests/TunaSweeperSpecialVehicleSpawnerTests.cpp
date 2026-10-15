#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperSpecialVehicleSpawner.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSpecialVehicleSpawnerTest,
	"TunaSweeper.Vehicle.SpecialSpawner.OnePerVisit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperSpecialVehicleSpawnerTest::RunTest(const FString& Parameters)
{
	UClass* VehicleClass = LoadClass<ATunaSweeperATVActor>(nullptr,
		TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
	if (!TestNotNull(TEXT("Configured ATV Blueprint exists"), VehicleClass)) return false;
	UClass* SpawnerClass = LoadClass<ATunaSweeperSpecialVehicleSpawner>(nullptr,
		TEXT("/Game/Blueprints/Spawners/BP_SpecialATVSpawner.BP_SpecialATVSpawner_C"));
	if (!TestNotNull(TEXT("Reusable spawner Blueprint exists"), SpawnerClass)) return false;
	for (int32 Visit = 0; Visit < 2; ++Visit)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		ON_SCOPE_EXIT
		{
			World->EndPlay(EEndPlayReason::Quit);
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		};
		const FVector Position(650, -400, 150);
		auto* Spawner = World->SpawnActor<ATunaSweeperSpecialVehicleSpawner>(SpawnerClass, Position, FRotator(0, 35, 0));
		TestEqual(TEXT("Saved spawner Blueprint selects the ATV Blueprint"), Spawner->VehicleClass.Get(), VehicleClass);
		TestNull(TEXT("Vehicle is not created during editor-style construction"), Spawner->SpawnedVehicle.Get());
		World->InitializeActorsForPlay(FURL());
		World->GetWorldSettings()->NotifyBeginPlay();
		World->GetWorldSettings()->NotifyMatchStarted();
		World->BeginPlay();
		auto* Vehicle = Spawner->SpawnedVehicle.Get();
		if (!TestNotNull(TEXT("Every fresh visit spawns an ATV on BeginPlay"), Vehicle)) return false;
		TestEqual(TEXT("Spawner uses the selected Blueprint class"), Vehicle->GetClass(), VehicleClass);
		TestTrue(TEXT("Vehicle starts at the spawn marker"), Vehicle->GetActorLocation().Equals(Position, .1));
		TestTrue(TEXT("Vehicle follows marker orientation"), Vehicle->GetActorRotation().Equals(FRotator(0, 35, 0), .1));
		TestEqual(TEXT("Vehicle stays in its spawner's level"), Vehicle->GetLevel(), Spawner->GetLevel());
		TestEqual(TEXT("Repeated evaluation returns the same vehicle"), Spawner->SpawnVehicle(), Vehicle);
		int32 Count = 0;
		for (TActorIterator<ATunaSweeperATVActor> It(World); It; ++It) ++Count;
		TestEqual(TEXT("Repeated evaluation cannot duplicate the ATV"), Count, 1);
		Vehicle->Destroy();
		TestNull(TEXT("A destroyed vehicle is not recreated repeatedly in the same visit"), Spawner->SpawnVehicle());
	}
	UWorld* EditorWorld = UWorld::CreateWorld(EWorldType::Editor, false);
	auto* EditorSpawner = EditorWorld->SpawnActor<ATunaSweeperSpecialVehicleSpawner>();
	EditorSpawner->VehicleClass = VehicleClass;
	TestNull(TEXT("Editor-world invocation cannot create a gameplay vehicle"), EditorSpawner->SpawnVehicle());
	EditorWorld->DestroyWorld(false);
	EditorWorld->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSpecialVehicleWorldProgressTest,
	"TunaSweeper.Vehicle.SpecialSpawner.WorldProgressRepairAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperSpecialVehicleWorldProgressTest::RunTest(const FString& Parameters)
{
	UClass* VehicleClass = LoadClass<ATunaSweeperATVActor>(nullptr,
		TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
	if (!TestNotNull(TEXT("Configured ATV Blueprint exists"), VehicleClass)) return false;
	const FName FacilityId(TEXT("test.facility.atv_warehouse"));
	TArray<uint8> SavedBytes;
	for (int32 Visit = 0; Visit < 2; ++Visit)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
		Game->bInventoryStateInitialized = true; // Never touch the player's inventory/save slot.
		World->SetGameInstance(Game);
		ON_SCOPE_EXIT
		{
			World->EndPlay(EEndPlayReason::Quit);
			World->SetGameInstance(nullptr);
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		};
		if (Visit == 1)
		{
			auto* Loaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(SavedBytes));
			if (!TestNotNull(TEXT("Existing save format restores facility progress"), Loaded)) return false;
			for (const auto& State : Loaded->WorldProgressStates)
				Game->WorldProgressStatesById.Add(State.ObjectId, State);
		}
		auto* Spawner = World->SpawnActor<ATunaSweeperSpecialVehicleSpawner>();
		if (!TestNotNull(TEXT("Gated spawner exists"), Spawner)) return false;
		Spawner->VehicleClass = VehicleClass;
		Spawner->RequiredWorldProgressObjectId = FacilityId;
		World->InitializeActorsForPlay(FURL());
		World->GetWorldSettings()->NotifyBeginPlay();
		World->GetWorldSettings()->NotifyMatchStarted();
		World->BeginPlay();
		if (Visit == 0)
		{
			TestNull(TEXT("Unrepaired warehouse cannot spawn on BeginPlay"), Spawner->SpawnedVehicle.Get());
			TestNull(TEXT("Explicit spawn also respects the repair requirement"), Spawner->SpawnVehicle());
			Game->UpdateWorldProgressState(TEXT("test.facility.other"), NAME_None,
				ETunaSweeperWorldProgressState::Completed, 1, 1, false);
			TestNull(TEXT("A different repaired facility cannot unlock the warehouse"), Spawner->SpawnedVehicle.Get());
			Game->UpdateWorldProgressState(FacilityId, NAME_None,
				ETunaSweeperWorldProgressState::InProgress, 1, 1, false);
			TestNull(TEXT("Quantity alone cannot unlock an incomplete repair"), Spawner->SpawnedVehicle.Get());
			Game->UpdateWorldProgressState(FacilityId, NAME_None,
				ETunaSweeperWorldProgressState::Completed, 1, 1, false);
		}
		auto* Vehicle = Spawner->SpawnedVehicle.Get();
		if (!TestNotNull(Visit == 0 ? TEXT("Repair notification immediately spawns the ATV")
			: TEXT("Fresh visit spawns from restored repair completion"), Vehicle)) return false;
		Game->OnWorldProgressChanged.Broadcast();
		TestEqual(TEXT("Repeated progress notifications retain the same ATV"), Spawner->SpawnedVehicle.Get(), Vehicle);
		int32 Count = 0;
		for (TActorIterator<ATunaSweeperATVActor> It(World); It; ++It) ++Count;
		TestEqual(TEXT("Only one ATV exists after repeated notifications"), Count, 1);
		Vehicle->Destroy();
		Game->OnWorldProgressChanged.Broadcast();
		TestNull(TEXT("Repair notifications cannot replace a destroyed ATV during the same visit"), Spawner->SpawnVehicle());
		if (Visit == 0)
		{
			auto* Save = NewObject<UTunaSweeperSaveGame>();
			Game->WorldProgressStatesById.GenerateValueArray(Save->WorldProgressStates);
			if (!TestTrue(TEXT("Facility completion serializes without a vehicle save field"),
				UGameplayStatics::SaveGameToMemory(Save, SavedBytes))) return false;
		}
		Spawner->Destroy();
		TestFalse(TEXT("Ending the spawner removes its progress subscription"), Game->OnWorldProgressChanged.IsBoundToObject(Spawner));
	}
	return true;
}
#endif
