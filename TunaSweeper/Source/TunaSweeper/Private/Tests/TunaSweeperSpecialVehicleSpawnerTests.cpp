#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
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
#endif
