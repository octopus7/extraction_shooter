#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperATVContactTests
{
struct FScene
{
	UWorld* World = nullptr;
	ATunaSweeperATVActor* ATV = nullptr;
	ATunaSweeperTopDownCharacter* Player = nullptr;
	APawn* Shooter = nullptr;

	FScene()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		auto* Ground = World->SpawnActor<AActor>();
		auto* Floor = NewObject<UBoxComponent>(Ground);
		Ground->SetRootComponent(Floor);
		Floor->SetBoxExtent(FVector(20000, 20000, 10));
		Floor->SetCollisionProfileName(TEXT("BlockAll"));
		Floor->RegisterComponent();
		Ground->SetActorLocation(FVector(0, 0, -10));
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		UClass* ATVClass = LoadClass<ATunaSweeperATVActor>(nullptr, TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
		UClass* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr, TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
		if (!ATVClass || !PlayerClass) return;
		ATV = World->SpawnActor<ATunaSweeperATVActor>(ATVClass, FVector(0, 0, 30), FRotator::ZeroRotator, Spawn);
		Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass, FVector(-240, 0, 90), FRotator::ZeroRotator, Spawn);
		// A local AI controller drives the real player capsule in this viewport-less world.
		World->SpawnActor<AAIController>()->Possess(Player);
		auto* Mount = ATV->MountComponent.Get();
		Mount->EngineStartSound = Mount->EngineIdleSound = Mount->EngineStopSound = nullptr;
		Mount->EngineDriveSound = Mount->EngineBoostSound = nullptr;
		Shooter = World->SpawnActor<APawn>();
		auto* Faction = NewObject<UTunaSweeperFactionComponent>(Shooter);
		Shooter->AddInstanceComponent(Faction);
		Faction->SetFactionId(TunaSweeperFactionIds::Enemy);
		Faction->RegisterComponent();
		World->InitializeActorsForPlay(FURL());
		World->GetWorldSettings()->NotifyBeginPlay();
		World->GetWorldSettings()->NotifyMatchStarted();
		World->BeginPlay();
	}
	~FScene()
	{
		World->EndPlay(EEndPlayReason::Quit);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	}
	void Step(int32 Frames = 1)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			World->Tick(LEVELTICK_All, 1.0f / 60.0f);
			FPlatformProcess::Sleep(0.001f);
			++GFrameCounter;
		}
	}
	ATunaSweeperProjectile* Fire(const FVector& Location, const FVector& Velocity)
	{
		const FTransform Transform(Velocity.Rotation(), Location);
		auto* Shot = World->SpawnActorDeferred<ATunaSweeperProjectile>(ATunaSweeperProjectile::StaticClass(),
			Transform, Shooter, Shooter, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Shot->SetDamageAmount(1);
		Shot->FinishSpawning(Transform);
		auto* Movement = Shot->FindComponentByClass<UProjectileMovementComponent>();
		Movement->MaxSpeed = Velocity.Size();
		Movement->Velocity = Velocity;
		return Shot;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVPedestrianContactTest,
	"TunaSweeper.Vehicle.PedestrianContact", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperATVPedestrianContactTest::RunTest(const FString& Parameters)
{
	TunaSweeperATVContactTests::FScene Scene;
	if (!TestNotNull(TEXT("Saved ATV BP"), Scene.ATV) || !TestNotNull(TEXT("Player BP"), Scene.Player)) return false;
	Scene.Step(180);
	const FVector Parked = Scene.ATV->GetActorLocation();
	for (const FVector Direction : {FVector::ForwardVector, FVector::RightVector})
	{
		Scene.Player->GetCharacterMovement()->StopMovementImmediately();
		Scene.Player->SetActorLocation(Parked - Direction * 240 + FVector(0, 0, 90), false, nullptr, ETeleportType::TeleportPhysics);
		for (int32 Frame = 0; Frame < 240; ++Frame)
		{
			Scene.Player->AddMovementInput(Direction);
			Scene.Step();
		}
		const float Displacement = FVector::Dist2D(Parked, Scene.ATV->GetActorLocation());
		AddInfo(FString::Printf(TEXT("Pushing along %s: parked displacement %.3f cm, player %s"), *Direction.ToString(), Displacement, *Scene.Player->GetActorLocation().ToString()));
		TestTrue(TEXT("Test character actually walks up to the ATV"), FVector::DotProduct(Scene.Player->GetActorLocation() - Parked, Direction) > -200);
		TestTrue(TEXT("Walking into a parked ATV does not move it"), Displacement < 2);
		TestTrue(TEXT("ATV still blocks the walking player"), FVector::DotProduct(Scene.Player->GetActorLocation() - Parked, Direction) < -70);
	}
	TestTrue(TEXT("Parking keeps chassis gravity and suspension active"), Scene.ATV->VehicleMesh->IsSimulatingPhysics());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVProjectileContactTest,
	"TunaSweeper.Vehicle.ProjectileContact", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperATVProjectileContactTest::RunTest(const FString& Parameters)
{
	TunaSweeperATVContactTests::FScene Scene;
	if (!TestNotNull(TEXT("Saved ATV BP"), Scene.ATV) || !TestNotNull(TEXT("Player BP"), Scene.Player)) return false;
	Scene.Step(180);
	// Use the same channel, response filter and wheel-sized sphere as Chaos suspension queries.
	auto* ProbeShot = Scene.Fire(FVector(0, 0, 1000), FVector(0, 3000, 0));
	ProbeShot->FindComponentByClass<UProjectileMovementComponent>()->Deactivate();
	FHitResult WheelHit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ATVProjectileGroundTest), false, Scene.ATV);
	const bool bBulletIsGround = Scene.World->SweepSingleByChannel(WheelHit, FVector(0, 0, 1100), FVector(0, 0, 900),
		FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(33), Query,
		FCollisionResponseParams(Scene.ATV->VehicleMovement->WheelTraceCollisionResponses));
	TestFalse(TEXT("Bullets cannot become suspension ground contacts"), bBulletIsGround);
	TestFalse(TEXT("Swept bullets do not participate in rigid-body contact solving"),
		CastChecked<USphereComponent>(ProbeShot->GetRootComponent())->IsPhysicsCollisionEnabled());
	ProbeShot->Destroy();
	Scene.Player->SetActorLocation(Scene.ATV->GetActorLocation() + FVector(0, 140, 90));
	if (!TestTrue(TEXT("Mount for driving under fire"), Scene.ATV->MountComponent->TryMount(Scene.Player))) return false;
	Scene.ATV->MountComponent->SetDriveInput(FVector2D(0, 1));
	Scene.Step(360);
	TArray<TWeakObjectPtr<ATunaSweeperProjectile>> Shots;
	const float VehicleHealthBefore = Scene.ATV->CurrentDurability;
	const float RiderHealthBeforeBodyHits = Scene.Player->GetVitalsComponent()->GetVitalsState().Health;
	float MaxVerticalSpeed = 0;
	float MaxTilt = 0;
	for (int32 Frame = 0; Frame < 120; ++Frame)
	{
		if (Frame % 4 == 0)
		{
			const FVector Target = Scene.ATV->GetActorTransform().TransformPosition(FVector(-5, 0, 60));
			Shots.Add(Scene.Fire(Target - FVector(0, 90, 0), FVector(0, 3000, 0) + Scene.ATV->GetVelocity()));
		}
		Scene.Step();
		MaxVerticalSpeed = FMath::Max(MaxVerticalSpeed, float(FMath::Abs(Scene.ATV->GetVelocity().Z)));
		const FRotator Rotation = Scene.ATV->GetActorRotation();
		MaxTilt = FMath::Max(MaxTilt, float(FMath::Max(FMath::Abs(Rotation.Pitch), FMath::Abs(Rotation.Roll))));
	}
	int32 HitCount = 0;
	for (const auto& Shot : Shots) HitCount += !Shot.IsValid() || Shot->IsActorBeingDestroyed() ? 1 : 0;
	AddInfo(FString::Printf(TEXT("Shots stopped by chassis %d/%d, max vertical speed %.2f cm/s, tilt %.2f degrees"), HitCount, Shots.Num(), MaxVerticalSpeed, MaxTilt));
	TestTrue(TEXT("Actual projectile sweeps still hit the moving chassis"), HitCount >= 25);
	TestEqual(TEXT("Each actual bullet reduces vehicle durability once"), VehicleHealthBefore - Scene.ATV->CurrentDurability, float(HitCount));
	TestEqual(TEXT("Chassis hits do not also damage the rider"), Scene.Player->GetVitalsComponent()->GetVitalsState().Health, RiderHealthBeforeBodyHits);
	TestTrue(TEXT("Gunfire does not launch the driving ATV"), MaxVerticalSpeed < 100 && MaxTilt < 10);
	TestTrue(TEXT("Driving continues under fire"), Scene.ATV->VehicleMovement->GetForwardSpeed() > 500);
	// Aim above the chassis at the seated capsule: the collision fix must not make the rider invulnerable.
	const float HealthBefore = Scene.Player->GetVitalsComponent()->GetVitalsState().Health;
	const float VehicleHealthBeforeRiderHit = Scene.ATV->CurrentDurability;
	const FVector RiderTarget = Scene.Player->GetCapsuleComponent()->GetComponentLocation() + FVector(0, 0, 45);
	Scene.Fire(RiderTarget - FVector(0, 90, 0), FVector(0, 3000, 0) + Scene.ATV->GetVelocity());
	Scene.Step(12);
	TestTrue(TEXT("Enemy projectile still damages the seated rider"), Scene.Player->GetVitalsComponent()->GetVitalsState().Health < HealthBefore);
	TestEqual(TEXT("Rider hit does not also damage the vehicle"), Scene.ATV->CurrentDurability, VehicleHealthBeforeRiderHit);
	return true;
}
#endif
