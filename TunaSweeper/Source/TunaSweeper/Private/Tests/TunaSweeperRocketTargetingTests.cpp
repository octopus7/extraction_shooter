#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Weapon/TunaSweeperTopAttackRocket.h"

namespace TunaRocketTargetTests
{
	struct FScene
	{
		UWorld* World;
		FScene()
		{
			const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}
		~FScene()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
		void SetFaction(AActor* Actor, uint8 FactionId)
		{
			auto* Faction = Actor->FindComponentByClass<UTunaSweeperFactionComponent>();
			if (!Faction)
			{
				Faction = NewObject<UTunaSweeperFactionComponent>(Actor);
				Actor->AddInstanceComponent(Faction);
				Faction->RegisterComponent();
			}
			Faction->SetFactionId(FactionId);
			World->GetSubsystem<UTunaSweeperFactionSubsystem>()->RegisterFactionActor(Actor);
		}
		AActor* Target(const FVector& Position, uint8 FactionId)
		{
			auto* Actor = World->SpawnActor<AActor>();
			auto* Root = NewObject<USphereComponent>(Actor);
			Actor->AddInstanceComponent(Root);
			Actor->SetRootComponent(Root);
			Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Root->RegisterComponent();
			Actor->SetActorLocation(Position);
			SetFaction(Actor, FactionId);
			return Actor;
		}
		ATunaSweeperTopAttackRocket* Rocket(AActor* Owner, AActor* LockedTarget, const FVector& Fallback, float Damage = 0.0f)
		{
			const FTransform Transform(FVector(0, 0, 100));
			auto* Result = World->SpawnActorDeferred<ATunaSweeperTopAttackRocket>(ATunaSweeperTopAttackRocket::StaticClass(), Transform, Owner);
			Result->TargetActor = LockedTarget;
			Result->bTargetLocked = LockedTarget != nullptr;
			Result->TargetLocation = Fallback;
			Result->Damage = Damage;
			Result->FinishSpawning(Transform);
			// Isolate target/damage behavior from collision tests covered by the flight suite.
			Result->Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Result->DispatchBeginPlay();
			return Result;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketTargetTrackingTest, "TunaSweeper.Rocket.TargetSelectionAndTracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketTargetTrackingTest::RunTest(const FString& Parameters)
{
	TunaRocketTargetTests::FScene Scene;
	auto* Owner = Scene.Target(FVector(-2000, 0, 0), 1);
	auto* Friend = Scene.Target(FVector(100, 0, 0), 1);
	auto* Enemy = Scene.Target(FVector(1000, 400, 0), 10);
	TestFalse(TEXT("Friendly cannot be locked"), ATunaSweeperTopAttackRocket::IsValidTarget(Owner, Friend));
	TestTrue(TEXT("Hostile can be locked"), ATunaSweeperTopAttackRocket::IsValidTarget(Owner, Enemy));
	auto* Automatic = Scene.Rocket(Owner, nullptr, FVector(800, -500, 0));
	Automatic->Tick(3.0f);
	TestTrue(TEXT("Automatic shot selects hostile over closer friend"), Automatic->GetActorLocation().Equals(Enemy->GetActorLocation(), 0.2f));
	Automatic->Destroy();

	auto* Locked = Scene.Rocket(Owner, Enemy, FVector(800, -500, 0));
	Locked->Tick(0.8f);
	Enemy->SetActorLocation(FVector(1400, 700, 0));
	Locked->Tick(0.4f);
	TestTrue(TEXT("Guidance projection follows moving locked target"), Locked->GuidanceDecal->GetComponentLocation().Equals(Enemy->GetActorLocation() + FVector(0, 0, 50), 0.2f));
	Locked->Tick(0.4f);
	const FVector DiveTarget = Enemy->GetActorLocation();
	Enemy->SetActorLocation(FVector(1600, 900, 0));
	Locked->Tick(1.0f);
	TestTrue(TEXT("Vertical terminal dive retains the sampled strike point"), Locked->GetActorLocation().Equals(DiveTarget, 0.2f));
	Locked->Destroy();

	auto* LostLock = Scene.Rocket(Owner, Enemy, FVector(800, -500, 0));
	LostLock->Tick(0.8f);
	const FVector LastTargetPoint = Enemy->GetActorLocation();
	Enemy->Destroy();
	Scene.Target(FVector(500, -200, 0), 10);
	LostLock->Tick(2.0f);
	TestTrue(TEXT("Lost locked target preserves last strike point without retargeting"), LostLock->GetActorLocation().Equals(LastTargetPoint, 0.2f));
	TestFalse(TEXT("Completed lost-target shot removes marker"), LostLock->GuidanceDecal->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketFactionDamageTest, "TunaSweeper.Rocket.BlastFactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketFactionDamageTest::RunTest(const FString& Parameters)
{
	TunaRocketTargetTests::FScene Scene;
	auto* Owner = Scene.Target(FVector(-2000, 0, 0), 1);
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Enemy = Scene.World->SpawnActor<ATunaSweeperEnemyCharacter>(FVector(100, 0, 100), FRotator::ZeroRotator, Spawn);
	auto* Friend = Scene.World->SpawnActor<ATunaSweeperEnemyCharacter>(FVector(-100, 0, 100), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Enemy damage receiver"), Enemy) || !TestNotNull(TEXT("Friendly damage receiver"), Friend)) return false;
	Scene.SetFaction(Enemy, 10); Scene.SetFaction(Friend, 1);
	for (auto* Receiver : {Enemy, Friend})
	{
		Receiver->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Receiver->GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
		Receiver->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	const float EnemyHealth = Enemy->GetHealth(), FriendHealth = Friend->GetHealth();
	auto* Rocket = Scene.Rocket(Owner, Enemy, FVector(0, 0, 100), 5.0f);
	Rocket->Tick(3.0f);
	TestEqual(TEXT("Blast has detonated"), Rocket->GetFlightPhase(), ETopAttackRocketPhase::Detonated);
	TestTrue(TEXT("Hostile in blast takes damage"), Enemy->GetHealth() < EnemyHealth);
	TestEqual(TEXT("Friendly in same blast retains health"), Friend->GetHealth(), FriendHealth);
	return true;
}
#endif
