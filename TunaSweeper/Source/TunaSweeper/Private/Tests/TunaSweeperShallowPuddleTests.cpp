#if WITH_DEV_AUTOMATION_TESTS

#include "Environment/TunaSweeperShallowPuddleActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperShallowPuddleFootprintTest,
	"TunaSweeper.Environment.ShallowPuddle.Footprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperShallowPuddleFootprintTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ATunaSweeperShallowPuddleActor* Puddle = World->SpawnActor<ATunaSweeperShallowPuddleActor>();
	Puddle->SetActorLocation(FVector(1000, 2000, 30));
	Puddle->HalfExtentCm = FVector2D(200, 100);
	Puddle->MaxWaterDepthCm = 10;
	Puddle->OutlineIrregularity = 0;
	Puddle->RefreshPuddle();
	TestTrue(TEXT("Ground beneath center is wet"), Puddle->ContainsGroundPoint(FVector(1000, 2000, 25)));
	TestFalse(TEXT("Bounding rectangle corner is dry"), Puddle->ContainsGroundPoint(FVector(1150, 2075, 25)));
	TestFalse(TEXT("Wet decal outside water is not a water footstep"), Puddle->ContainsGroundPoint(FVector(1170, 2000, 25)));
	TestFalse(TEXT("Upper floor is dry"), Puddle->ContainsGroundPoint(FVector(1000, 2000, 34)));
	TestFalse(TEXT("Lower floor outside authored depth is dry"), Puddle->ContainsGroundPoint(FVector(1000, 2000, 19)));
	TestTrue(TEXT("Inside long axis"), Puddle->ContainsGroundPoint(FVector(1140, 2000, 25)));
	Puddle->SetActorRotation(FRotator(25, 90, 15));
	Puddle->SetActorScale3D(FVector(-2, 1, 3));
	Puddle->RefreshPuddle();
	TestTrue(TEXT("Yaw and negative XY scale affect water footprint"), Puddle->ContainsGroundPoint(FVector(1000, 2250, 25)));
	TestFalse(TEXT("Rotated short axis stays narrow"), Puddle->ContainsGroundPoint(FVector(1100, 2000, 25)));
	TestTrue(TEXT("Pitch and roll never tilt water"), Puddle->GetSurface()->GetUpVector().Equals(FVector::UpVector, 0.001));
	TestEqual(TEXT("Water never blocks movement"), Puddle->GetSurface()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	Puddle->HalfExtentCm = FVector2D::ZeroVector;
	Puddle->RefreshPuddle();
	TestFalse(TEXT("Degenerate authored size does not create a giant footprint"), Puddle->ContainsGroundPoint(FVector(1005, 2005, 25)));
	World->DestroyWorld(false);
	World->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperShallowPuddleSelectionTest,
	"TunaSweeper.Environment.ShallowPuddle.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperShallowPuddleSelectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ATunaSweeperShallowPuddleActor* Lower = World->SpawnActor<ATunaSweeperShallowPuddleActor>();
	ATunaSweeperShallowPuddleActor* Upper = World->SpawnActor<ATunaSweeperShallowPuddleActor>();
	Lower->SetActorLocation(FVector(0, 0, 3));
	Upper->SetActorLocation(FVector(0, 0, 5));
	Lower->WaterMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Environment/ShallowPuddle/Materials/M_ShallowPuddle_Water.M_ShallowPuddle_Water"));
	TestNotNull(TEXT("Saved water material is available"), Lower->WaterMaterial.Get());
	Lower->RefreshPuddle();
	Upper->WaterMaterial = nullptr;
	Upper->RefreshPuddle();
	TestEqual(TEXT("Material-less water cannot override visible ground"),
		ATunaSweeperShallowPuddleActor::FindPuddleAtGroundPoint(World, FVector::ZeroVector), Lower);
	Upper->WaterMaterial = Lower->WaterMaterial;
	Upper->RefreshPuddle();
	TestEqual(TEXT("Overlapping puddles resolve once to highest valid surface"),
		ATunaSweeperShallowPuddleActor::FindPuddleAtGroundPoint(World, FVector::ZeroVector), Upper);
	Upper->SetActorHiddenInGame(true);
	TestEqual(TEXT("Hidden puddle does not emit water footsteps"),
		ATunaSweeperShallowPuddleActor::FindPuddleAtGroundPoint(World, FVector::ZeroVector), Lower);
	TestNull(TEXT("Dry ground has no water override"),
		ATunaSweeperShallowPuddleActor::FindPuddleAtGroundPoint(World, FVector(10000, 0, 0)));
	TestNull(TEXT("Missing world is safe"),
		ATunaSweeperShallowPuddleActor::FindPuddleAtGroundPoint(nullptr, FVector::ZeroVector));
	World->DestroyWorld(false);
	World->RemoveFromRoot();
	return true;
}

#endif
