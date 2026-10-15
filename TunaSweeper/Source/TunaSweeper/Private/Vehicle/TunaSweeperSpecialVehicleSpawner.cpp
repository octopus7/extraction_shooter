#include "Vehicle/TunaSweeperSpecialVehicleSpawner.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Vehicle/TunaSweeperATVActor.h"

ATunaSweeperSpecialVehicleSpawner::ATunaSweeperSpecialVehicleSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SetRootComponent(SpawnPoint);
}

void ATunaSweeperSpecialVehicleSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::HandleWorldProgressChanged);
	SpawnVehicle();
}

void ATunaSweeperSpecialVehicleSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		Game->OnWorldProgressChanged.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperSpecialVehicleSpawner::HandleWorldProgressChanged()
{
	SpawnVehicle();
}

bool ATunaSweeperSpecialVehicleSpawner::ShouldSpawnVehicle_Implementation() const
{
	if (RequiredWorldProgressObjectId.IsNone()) return true;
	const auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	FTunaSweeperWorldProgressSaveData State;
	return Game && Game->TryGetWorldProgressState(RequiredWorldProgressObjectId, State)
		&& State.State == ETunaSweeperWorldProgressState::Completed;
}

ATunaSweeperATVActor* ATunaSweeperSpecialVehicleSpawner::SpawnVehicle()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !HasAuthority() || IsActorBeingDestroyed()) return nullptr;
	if (IsValid(SpawnedVehicle) && !SpawnedVehicle->IsActorBeingDestroyed()) return SpawnedVehicle;
	if (bSpawnedThisVisit || !VehicleClass || !ShouldSpawnVehicle()) return nullptr;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.OverrideLevel = GetLevel();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// Set before SpawnActor so BeginPlay callbacks cannot create a duplicate.
	bSpawnedThisVisit = true;
	const FTransform SpawnTransform(SpawnPoint->GetComponentRotation(), SpawnPoint->GetComponentLocation());
	SpawnedVehicle = World->SpawnActor<ATunaSweeperATVActor>(VehicleClass, SpawnTransform, Params);
	if (!SpawnedVehicle) bSpawnedThisVisit = false;
	return SpawnedVehicle;
}
