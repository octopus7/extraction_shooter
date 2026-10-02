#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "LoopRailStation.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Player/TunaSweeperPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTrainCameraTest,
	"TunaSweeper.Player.Camera.TrainRide", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperTrainCameraTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		World->RemoveFromRoot();
	};

	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>(
		ATunaSweeperTopDownCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	auto* Controller = World->SpawnActor<ATunaSweeperPlayerController>();
	Controller->SetAsLocalPlayerController();
	Controller->Possess(Character);
	auto* Boom = Character->FindComponentByClass<USpringArmComponent>();
	auto* Camera = Character->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Player camera boom"), Boom) || !TestNotNull(TEXT("Player camera"), Camera)) return false;
	if (!TestTrue(TEXT("Controlled player is local"), Character->IsLocallyControlled())) return false;

	auto* Track = World->SpawnActor<ALoopRailTrack>();
	auto* Train = World->SpawnActor<ALoopRailTrain>();
	Train->Track = Track;
	Train->RebuildTrain();
	Train->ResetTrain();
	if (!TestNotNull(TEXT("Passenger car floor"), Train->GetVehicleFloor(1))) return false;

	auto SettleCamera = [&]()
	{
		for (int32 Frame = 0; Frame < 180; ++Frame) Character->Tick(1.0f / 60.0f);
	};
	auto ReachCruise = [&]()
	{
		Train->SetRunning(true);
		for (int32 Frame = 0; Frame < 360; ++Frame) Train->Tick(1.0f / 60.0f);
	};
	float NormalModeDistances[3];
	for (float& Distance : NormalModeDistances)
	{
		SettleCamera();
		Distance = Boom->TargetArmLength;
		Character->CyclePlayerCameraMode();
	}

	// The train must change only the ridden player's distance, preserving the selected mode.
	for (int32 Mode = 0; Mode < 3; ++Mode)
	{
		Character->SetBase(nullptr);
		SettleCamera();
		const float NormalDistance = Boom->TargetArmLength;
		const float NormalFOV = Camera->FieldOfView;
		const FRotator NormalRotation = Boom->GetRelativeRotation();
		const ETunaSweeperPlayerCameraMode SelectedMode = Character->GetPlayerCameraMode();

		Train->SetRunning(false);
		Character->SetBase(Train->GetVehicleFloor(1));
		SettleCamera();
		TestTrue(TEXT("Boarding a stopped train keeps normal distance"), FMath::IsNearlyEqual(Boom->TargetArmLength, NormalDistance, 0.5f));

		ReachCruise();
		TestTrue(TEXT("Fixture train reaches cruising speed"), Train->GetSpeed() > 590.0f);
		Character->Tick(1.0f / 60.0f);
		TestTrue(TEXT("Rider remains based on the passenger floor"), Character->GetMovementBase() == Train->GetVehicleFloor(1));
		TestTrue(TEXT("Departure begins widening the view"), Boom->TargetArmLength > NormalDistance);
		TestTrue(TEXT("Departure blends rather than snapping to cruising distance"), Boom->TargetArmLength < NormalDistance * 1.1f);
		SettleCamera();
		TestTrue(TEXT("A ridden moving train widens distance to 160 percent"), FMath::IsNearlyEqual(Boom->TargetArmLength, NormalDistance * 1.6f, 1.0f));
		const float MovingModeDistance = Boom->TargetArmLength;
		TestTrue(TEXT("Train zoom preserves camera FOV"), FMath::IsNearlyEqual(Camera->FieldOfView, NormalFOV, 0.01f));
		TestTrue(TEXT("Train zoom preserves camera angle"), Boom->GetRelativeRotation().Equals(NormalRotation, 0.01f));
		TestEqual(TEXT("Train zoom preserves selected camera mode"), Character->GetPlayerCameraMode(), SelectedMode);
		Character->CyclePlayerCameraMode();
		SettleCamera();
		TestTrue(TEXT("Changing camera mode while riding uses the new mode's widened distance"),
			FMath::IsNearlyEqual(Boom->TargetArmLength, NormalModeDistances[(Mode + 1) % 3] * 1.6f, 1.0f));
		Character->CyclePlayerCameraMode();
		Character->CyclePlayerCameraMode();
		SettleCamera();

		Train->SetRunning(false);
		SettleCamera();
		TestTrue(TEXT("Pausing the train restores normal distance"), FMath::IsNearlyEqual(Boom->TargetArmLength, NormalDistance, 1.0f));
		const float PausedDistance = Boom->TargetArmLength;
		ReachCruise();
		SettleCamera();
		Character->SetBase(nullptr);
		Character->Tick(1.0f / 60.0f);
		TestTrue(TEXT("Leaving a moving train blends toward normal distance"), Boom->TargetArmLength > NormalDistance && Boom->TargetArmLength < MovingModeDistance);
		SettleCamera();
		TestTrue(TEXT("Leaving a moving train restores this mode's normal distance"), FMath::IsNearlyEqual(Boom->TargetArmLength, NormalDistance, 1.0f));
		AddInfo(FString::Printf(TEXT("Mode %d: stationary %.2f cm; moving %.2f cm; paused %.2f cm; off train %.2f cm"),
			Mode, NormalDistance, MovingModeDistance, PausedDistance, Boom->TargetArmLength));
		Character->CyclePlayerCameraMode();
	}

	Character->SetBase(Train->GetVehicleFloor(1));
	Train->SetRunning(false);
	SettleCamera();
	const float BeforeInputLockDistance = Boom->TargetArmLength;
	ReachCruise();
	SettleCamera();
	if (TestNotNull(TEXT("Walkable connection belongs to the train"), Train->GetConnectionFloor(1)))
	{
		Character->SetBase(Train->GetConnectionFloor(1));
		SettleCamera();
		TestTrue(TEXT("Crossing onto the connection keeps the moving train camera distance"),
			FMath::IsNearlyEqual(Boom->TargetArmLength, BeforeInputLockDistance * 1.6f, 1.0f));
		Character->SetBase(Train->GetVehicleFloor(1));
	}
	Controller->SetDemoEndingInputLock(true);
	TestTrue(TEXT("Dialogue input lock is active"), Controller->IsDialogueSequenceActive());
	Train->SetRunning(false);
	SettleCamera();
	TestTrue(TEXT("Stopping restores distance while gameplay input is locked"),
		FMath::IsNearlyEqual(Boom->TargetArmLength, BeforeInputLockDistance, 1.0f));
	Controller->SetDemoEndingInputLock(false);
	SettleCamera();
	Controller->SetDemoEndingInputLock(true);
	ReachCruise();
	SettleCamera();
	TestTrue(TEXT("Departure widens distance while gameplay input is locked"),
		FMath::IsNearlyEqual(Boom->TargetArmLength, BeforeInputLockDistance * 1.6f, 1.0f));
	Controller->SetDemoEndingInputLock(false);

	// A real station stop must restore the close view without any explicit camera notification.
	Character->SetBase(Train->GetVehicleFloor(1));
	ReachCruise();
	SettleCamera();
	const float MovingDistance = Boom->TargetArmLength;
	auto* Station = World->SpawnActor<ALoopRailStation>();
	Station->Track = Track;
	Station->DwellSeconds = 10.0f;
	Station->SetActorLocation(Track->Sample(Train->GetHeadDistance() + 2000.0).GetLocation());
	for (int32 Frame = 0; Frame < 1800 && !Train->IsDwelling(); ++Frame) Train->Tick(1.0f / 60.0f);
	TestTrue(TEXT("Fixture train arrives and dwells at the station"), Train->IsDwelling());
	SettleCamera();
	TestTrue(TEXT("Station dwell restores the close view"), FMath::IsNearlyEqual(Boom->TargetArmLength, MovingDistance / 1.6f, 1.0f));

	Station->Destroy();
	Train->ResetTrain();
	ReachCruise();
	SettleCamera();
	Controller->UnPossess();
	SettleCamera();
	TestTrue(TEXT("Uncontrolled character does not retain riding zoom"), FMath::IsNearlyEqual(Boom->TargetArmLength, MovingDistance / 1.6f, 1.0f));
	return true;
}

#endif
