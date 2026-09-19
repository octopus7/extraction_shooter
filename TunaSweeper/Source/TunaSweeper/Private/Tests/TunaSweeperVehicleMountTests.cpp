#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperVehicleMountTest,
	"TunaSweeper.Vehicle.MountInteraction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperVehicleMountTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>(FVector(0,0,45), FRotator::ZeroRotator, Spawn);
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector(0,150,90), FRotator::ZeroRotator, Spawn);
	auto* Other = World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector(0,-150,90), FRotator::ZeroRotator, Spawn);
	auto* Mount = ATV->MountComponent.Get();
	Mount->EngineStartSound = nullptr;
	Mount->EngineIdleSound = nullptr;
	Mount->EngineStopSound = nullptr;
	const auto Collision = Player->GetCapsuleComponent()->GetCollisionEnabled();
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	auto* Interactions = World->GetSubsystem<UTunaSweeperInteractionSubsystem>();
	Player->SetActorLocation(FVector(1000,0,90));
	TestFalse(TEXT("Remote interaction cannot mount"), Mount->RequestInteraction(Player));
	Player->SetActorLocation(FVector(0,150,90));
	const auto MovementMode = Player->GetCharacterMovement()->MovementMode.GetValue();
	TestTrue(TEXT("Shared interaction path mounts the player"), Mount->RequestInteraction(Player));
	TestTrue(TEXT("Player records the seat"), Player->GetVehicleMount() == Mount);
	TestTrue(TEXT("Possession stays on the original player"), Controller->GetPawn() == Player);
	TestTrue(TEXT("Player is attached to the vehicle"), Player->GetAttachParentActor() == ATV);
	TestEqual(TEXT("Character movement is disabled"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
	TestFalse(TEXT("Occupied interaction is unavailable"), Interactions->CanOfferInteraction(Mount));
	TestFalse(TEXT("Second rider cannot take an occupied seat"), Mount->TryMount(Other));
	Mount->UpdateStationaryHint(1.4f, 0);
	TestFalse(TEXT("Hint waits for the full delay"), Mount->IsDismountHintVisible());
	Mount->UpdateStationaryHint(0.11f, 0);
	TestTrue(TEXT("Hint appears when stationary"), Mount->IsDismountHintVisible());
	Mount->UpdateStationaryHint(0.01f, 100);
	TestFalse(TEXT("Movement hides the hint immediately"), Mount->IsDismountHintVisible());
	Mount->UpdateStationaryHint(0.1f, 0);
	TestFalse(TEXT("Movement resets the entire delay"), Mount->IsDismountHintVisible());
	TestFalse(TEXT("No ground rejects dismount without releasing the rider"), Mount->TryDismount());
	TestTrue(TEXT("Failed dismount stays mounted"), Player->IsMountedInVehicle());
	// A real collision floor exercises ground traces and capsule clearance.
	auto* Floor = World->SpawnActor<AActor>();
	auto* FloorBox = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(FloorBox);
	FloorBox->SetBoxExtent(FVector(1000,1000,10));
	FloorBox->SetCollisionProfileName(TEXT("BlockAll"));
	FloorBox->RegisterComponent();
	Floor->SetActorLocation(FVector(0,0,-10));
	auto* Blocker = World->SpawnActor<AActor>();
	auto* BlockerBox = NewObject<UBoxComponent>(Blocker);
	Blocker->SetRootComponent(BlockerBox);
	BlockerBox->SetBoxExtent(FVector(400,400,100));
	BlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
	BlockerBox->RegisterComponent();
	Blocker->SetActorLocation(FVector(0,0,110));
	TestFalse(TEXT("Blocked exit cannot place the rider through geometry"), Mount->TryDismount());
	Blocker->Destroy();
	TestTrue(TEXT("Safe ground permits dismount even before hint delay"), Mount->TryDismount());
	TestFalse(TEXT("Dismount clears both sides"), Player->IsMountedInVehicle() || Mount->GetRider());
	TestNull(TEXT("Dismount detaches player"), Player->GetAttachParentActor());
	TestEqual(TEXT("Collision restored"), Player->GetCapsuleComponent()->GetCollisionEnabled(), Collision);
	TestEqual(TEXT("Original movement mode restored"), int32(Player->GetCharacterMovement()->MovementMode.GetValue()), int32(MovementMode));
	Player->SetActorLocation(FVector(0,150,90));
	TestTrue(TEXT("Remount works"), Mount->RequestInteraction(Player));
	Mount->ReleaseRiderForEndPlay();
	TestFalse(TEXT("Vehicle cleanup releases character state"), Player->IsMountedInVehicle());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
