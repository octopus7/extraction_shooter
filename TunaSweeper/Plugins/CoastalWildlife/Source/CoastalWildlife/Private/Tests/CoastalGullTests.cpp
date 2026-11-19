#if WITH_DEV_AUTOMATION_TESTS

#include "Environment/CoastalGull.h"
#include "Environment/CoastalGullProximitySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalGullMotionTest, "TunaSweeper.Environment.CoastalGull.MotionAndProximity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoastalGullMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Game world"), World)) return false;
	ACoastalGull* Gull = World->SpawnActor<ACoastalGull>();
	auto* Scheduler = World->GetSubsystem<UCoastalGullProximitySubsystem>();
	if (!TestNotNull(TEXT("Gull"), Gull) || !TestNotNull(TEXT("World scheduler"), Scheduler))
	{
		World->DestroyWorld(false);
		return false;
	}
	Gull->RegisterAllActorTickFunctions(true, false);
	Gull->InitializeRoute();
	Gull->CycleTime = 0.f;
	Gull->ApplyMotion();
	Scheduler->RegisterGull(Gull);
	const FVector Origin = Gull->GetActorLocation();
	auto CheckDistance = [&](double Distance)
	{
		const TArray<FVector> Players{ Gull->GetActorLocation() + FVector(Distance, 0, 0) };
		Scheduler->ApplyPlayerLocations(Players);
	};
	CheckDistance(10000.);
	TestTrue(TEXT("100m activates even with no view or render"), Gull->IsActorTickEnabled());
	Gull->Tick(0.2f);
	TestTrue(TEXT("Walk advances on dry shore axis"), Gull->GetActorLocation().Y > Origin.Y);
	TestTrue(TEXT("Feet swing in opposition"), Gull->LegL->GetRelativeRotation().Pitch * Gull->LegR->GetRelativeRotation().Pitch < 0);
	TestTrue(TEXT("Walking wings folded"), FMath::Abs(Gull->WingL->GetRelativeRotation().Yaw) > 65);
	Gull->Tick(5.8f);
	TestTrue(TEXT("Takeoff gains height"), Gull->GetActorLocation().Z > Origin.Z + 5);
	const FTransform InFlight = Gull->GetActorTransform();
	const FTransform WingPose = Gull->WingL->GetRelativeTransform();
	CheckDistance(10001.);
	TestFalse(TEXT("Beyond 100m disables individual ticking"), Gull->IsActorTickEnabled());
	CheckDistance(10000.);
	TestTrue(TEXT("Reentry preserves world and wing pose"), InFlight.Equals(Gull->GetActorTransform()) && WingPose.Equals(Gull->WingL->GetRelativeTransform()));
	Gull->Tick(3.f);
	TestTrue(TEXT("Flight extends over water"), Gull->GetActorLocation().X > Origin.X + 350);
	const float FlapA = Gull->WingL->GetRelativeRotation().Roll;
	Gull->Tick(0.1f);
	TestTrue(TEXT("Flapping changes wing roll"), FMath::Abs(FlapA - Gull->WingL->GetRelativeRotation().Roll) > 0.1);
	Gull->Tick(1.9f);
	const FRotator GlidePose = Gull->WingL->GetRelativeRotation();
	Gull->Tick(0.15f);
	TestTrue(TEXT("Glide holds wing pose"), GlidePose.Equals(Gull->WingL->GetRelativeRotation(), 0.1));
	Gull->Tick(4.85f);
	TestTrue(TEXT("Approach descends with feet extended"), Gull->GetActorLocation().Z < Origin.Z + 60 && FMath::Abs(Gull->LegL->GetRelativeRotation().Pitch) < 30);
	Gull->Tick(1.f);
	TestTrue(TEXT("Landing returns to dry starting point"), Origin.Equals(Gull->GetActorLocation(), 0.1));
	Scheduler->UnregisterGull(Gull);
	World->DestroyWorld(false);
	return true;
}

#endif
