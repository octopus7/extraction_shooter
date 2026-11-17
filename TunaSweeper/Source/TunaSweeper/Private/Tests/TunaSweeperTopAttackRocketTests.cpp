#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/DecalComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Weapon/TunaSweeperTopAttackRocket.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTopAttackRocketFlightTest, "TunaSweeper.Rocket.FlightPhases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTopAttackRocketFlightTest::RunTest(const FString& Parameters)
{
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto Spawn = [&]()
	{
		const FTransform Transform(FVector(0, 0, 100));
		auto* Rocket = World->SpawnActorDeferred<ATunaSweeperTopAttackRocket>(ATunaSweeperTopAttackRocket::StaticClass(), Transform);
		Rocket->TargetLocation = FVector(1000, 400, 0);
		Rocket->Damage = 0;
		Rocket->FinishSpawning(Transform);
		Rocket->DispatchBeginPlay();
		return Rocket;
	};
	auto* Rocket = Spawn();
	Rocket->Tick(0.4f);
	TestTrue(TEXT("Vertical ascent"), Rocket->GetActorLocation().Equals(FVector(0, 0, 1000), 0.1));
	TestFalse(TEXT("No marker before ascent finishes"), Rocket->GuidanceDecal->IsVisible());
	Rocket->Tick(0.4f);
	TestEqual(TEXT("Guidance begins"), Rocket->GetFlightPhase(), ETopAttackRocketPhase::Guidance);
	TestTrue(TEXT("Ground projection appears after ascent"), Rocket->GuidanceDecal->IsVisible());
	Rocket->Tick(0.8f);
	TestEqual(TEXT("Dive begins"), Rocket->GetFlightPhase(), ETopAttackRocketPhase::Dive);
	const FVector DiveStart = Rocket->GetActorLocation();
	Rocket->Tick(0.2f);
	TestTrue(TEXT("Dive stays vertical"), FVector::Dist2D(DiveStart, Rocket->GetActorLocation()) < 0.1f);
	TestTrue(TEXT("Dive descends"), Rocket->GetActorLocation().Z < DiveStart.Z);
	Rocket->Tick(1.f);
	TestEqual(TEXT("Impact completes flight"), Rocket->GetFlightPhase(), ETopAttackRocketPhase::Detonated);
	TestFalse(TEXT("Marker removed at impact"), Rocket->GuidanceDecal->IsVisible());
	TestTrue(TEXT("Fallback reaches aimed point"), Rocket->GetActorLocation().Equals(FVector(1000, 400, 0), 0.1));
	auto* Hitch = Spawn();
	Hitch->Tick(10.f);
	TestEqual(TEXT("Long frame crosses all phases"), Hitch->GetFlightPhase(), ETopAttackRocketPhase::Detonated);
	TestTrue(TEXT("Long frame preserves impact"), Hitch->GetActorLocation().Equals(Rocket->GetActorLocation(), 0.1));
	auto* Ceiling = World->SpawnActor<AActor>();
	auto* Box = NewObject<UBoxComponent>(Ceiling);
	Ceiling->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(100, 100, 5));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	Ceiling->SetActorLocation(FVector(0, 0, 500));
	auto* Blocked = Spawn();
	Blocked->Tick(3.f);
	TestEqual(TEXT("Swept ascent hits a thin ceiling during a hitch"), Blocked->GetFlightPhase(), ETopAttackRocketPhase::Detonated);
	TestTrue(TEXT("Rocket stops below the ceiling"), Blocked->GetActorLocation().Z < 500.f);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTopAttackRocketAssetTest, "TunaSweeper.Rocket.SavedBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTopAttackRocketAssetTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<ATunaSweeperTopAttackRocket>(nullptr, TEXT("/Game/Weapons/Rocket/BP_TopAttackRocket.BP_TopAttackRocket_C"));
	if (!TestNotNull(TEXT("Saved replaceable rocket BP"), Class)) return false;
	const auto* Defaults = Class->GetDefaultObject<ATunaSweeperTopAttackRocket>();
	TestNotNull(TEXT("Authored rocket mesh"), Defaults->BodyMesh.Get());
	TestNotNull(TEXT("Blue exhaust material"), Defaults->ExhaustMaterial.Get());
	TestNotNull(TEXT("Blue ground projection"), Defaults->GuidanceMaterial.Get());
	TestNotNull(TEXT("Exhaust shape"), Defaults->ExhaustShape.Get());
	return true;
}
#endif
