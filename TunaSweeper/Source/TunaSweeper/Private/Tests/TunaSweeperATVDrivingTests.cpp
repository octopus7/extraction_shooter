#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVDrivingTest,
	"TunaSweeper.Vehicle.Driving", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperATVDrivingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Ground = World->SpawnActor<AActor>();
	auto* Floor = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(Floor);
	Floor->SetBoxExtent(FVector(20000,20000,10));
	Floor->SetCollisionProfileName(TEXT("BlockAll"));
	Floor->RegisterComponent();
	Ground->SetActorLocation(FVector(0,0,-10));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* ATVClass = LoadClass<ATunaSweeperATVActor>(nullptr, TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
	if (!TestNotNull(TEXT("Saved ATV Blueprint"), ATVClass)) return false;
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>(ATVClass, FVector(0,0,30), FRotator::ZeroRotator, Spawn);
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector(0,140,90), FRotator::ZeroRotator, Spawn);
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	auto* Mount = ATV->MountComponent.Get();
	Mount->EngineStartSound = Mount->EngineIdleSound = Mount->EngineStopSound = nullptr;
	Mount->EngineDriveSound = Mount->EngineBoostSound = nullptr;
	auto* Movement = ATV->VehicleMovement.Get();
	TestNotNull(TEXT("Chassis Physics Asset is saved"), ATV->VehicleMesh->GetPhysicsAsset());
	TestEqual(TEXT("Four wheels initialized"), Movement->Wheels.Num(), 4);
	TestTrue(TEXT("Physics is enabled"), ATV->VehicleMesh->IsSimulatingPhysics());
	World->InitializeActorsForPlay(FURL());
	// This isolated world has no GameMode to dispatch BeginPlay to actors.
	World->GetWorldSettings()->NotifyBeginPlay();
	World->GetWorldSettings()->NotifyMatchStarted();
	World->BeginPlay();
	ATV->SetDriveInput(FVector2D(1,1));
	TestTrue(TEXT("Unoccupied input is rejected"), ATV->GetDriveInput().IsNearlyZero());
	TestTrue(TEXT("Mount through interaction"), Mount->RequestInteraction(Player));
	Mount->SetDriveInput(FVector2D(0,1));
	AddInfo(FString::Printf(TEXT("Wheel FL %s, mass %.1f, COM %s"), *ATV->VehicleMesh->GetSocketTransform(TEXT("wheel_FL"), RTS_Component).GetLocation().ToString(), ATV->VehicleMesh->GetMass(), *ATV->VehicleMesh->GetCenterOfMass().ToString()));
	TestEqual(TEXT("Mounted W input reaches vehicle"), float(ATV->GetDriveInput().Y), 1.0f);
	auto Step = [&](int32 Frames)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			World->Tick(LEVELTICK_All, 1.0f/60.0f);
			FPlatformProcess::Sleep(0.001f);
			++GFrameCounter;
		}
	};
	Step(360);
	AddInfo(FString::Printf(TEXT("Forward speed %.1f cm/s, position %s"), Movement->GetForwardSpeed(), *ATV->GetActorLocation().ToString()));
	TestTrue(TEXT("W physically drives forward"), ATV->GetActorLocation().X > 100 && Movement->GetForwardSpeed() > 100);
	int32 Contacts = 0;
	for (int32 Index = 0; Index < Movement->Wheels.Num(); ++Index) Contacts += Movement->GetWheelState(Index).bInContact ? 1 : 0;
	TestTrue(TEXT("Suspension supports the chassis on the ground"), Contacts >= 3);
	const FReferenceSkeleton& Skeleton = ATV->VehicleMesh->GetSkeletalMeshAsset()->GetRefSkeleton();
	const FTransform WheelRest = Skeleton.GetRefBonePose()[Skeleton.FindBoneIndex(TEXT("wheel_FL"))];
	const FTransform WheelPose = ATV->VehicleMesh->GetSocketTransform(TEXT("wheel_FL"), RTS_Component);
	TestTrue(TEXT("Wheel bone follows suspension travel"), FMath::IsNearlyEqual(float(WheelPose.GetLocation().Z - WheelRest.GetLocation().Z), Movement->Wheels[0]->GetSuspensionOffset(), 1.0f));
	TestFalse(TEXT("Wheel bone visibly rotates"), WheelPose.GetRotation().Equals(WheelRest.GetRotation(), 0.01f));
	TestFalse(TEXT("Spring bone stretches with suspension"), FMath::IsNearlyEqual(ATV->VehicleMesh->GetSocketTransform(TEXT("spring_FL"), RTS_Component).GetScale3D().X, 1.0, 0.01));
	Mount->SetDriveInput(FVector2D(0.5f,1));
	Step(45);
	if (Movement->Wheels.Num() == 4)
	{
		TestTrue(TEXT("Front wheels steer"), FMath::Abs(Movement->Wheels[0]->GetSteerAngle()) > 1);
		TestTrue(TEXT("Rear wheels remain unsteered"), FMath::IsNearlyZero(Movement->Wheels[2]->GetSteerAngle()));
		TestTrue(TEXT("Wheels rotate"), FMath::Abs(Movement->Wheels[0]->GetRotationAngle()) > 1);
		const FQuat HandleRest = Skeleton.GetRefBonePose()[Skeleton.FindBoneIndex(TEXT("handlebar"))].GetRotation();
		TestFalse(TEXT("Handlebar bone follows steering"), ATV->VehicleMesh->GetSocketTransform(TEXT("handlebar"), RTS_Component).GetRotation().Equals(HandleRest, 0.01f));
	}
	Mount->SetDriveInput(FVector2D(0,-1));
	Step(600);
	AddInfo(FString::Printf(TEXT("Reverse speed %.1f cm/s"), Movement->GetForwardSpeed()));
	TestTrue(TEXT("S brakes then reverses"), Movement->GetForwardSpeed() < -50);
	Mount->SetDriveInput(FVector2D(0,1));
	Mount->SetBoostInput(true);
	TestTrue(TEXT("Shift enables boost while accelerating forward"), ATV->IsBoosting());
	Step(600);
	AddInfo(FString::Printf(TEXT("Boost speed %.1f cm/s"), Movement->GetForwardSpeed()));
	TestTrue(TEXT("Shift physically accelerates beyond normal speed"), Movement->GetForwardSpeed() > ATV->NormalTopSpeed + 100);
	Player->CancelActiveGameplayActions();
	TestTrue(TEXT("UI cancellation clears held vehicle input"), ATV->GetDriveInput().IsNearlyZero());
	TestFalse(TEXT("UI cancellation clears boost"), ATV->IsBoosting());
	Mount->ReleaseRiderForEndPlay();
	TestTrue(TEXT("Dismount applies parking brake"), Movement->GetHandbrakeInput());
	TestTrue(TEXT("Dismount clears input"), ATV->GetDriveInput().IsNearlyZero());
	World->EndPlay(EEndPlayReason::Quit);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	World->RemoveFromRoot();
	return true;
}
#endif
