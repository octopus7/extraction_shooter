#if WITH_DEV_AUTOMATION_TESTS

#include "ForestScatterGeometry.h"
#include "ForestScatterRegion.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "PCGComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterBoundaryTest, "TunaSweeper.ForestScatter.ConcaveBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterBoundaryTest::RunTest(const FString& Parameters)
{
	USplineComponent* Spline = NewObject<USplineComponent>();
	const TArray<FVector> Points = {
		{0, 0, 50}, {400, 0, 0}, {400, 100, 0}, {100, 100, 70}, {100, 400, 0}, {0, 400, 0}
	};
	Spline->SetSplinePoints(Points, ESplineCoordinateSpace::Local, false);
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		Spline->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}
	Spline->SetClosedLoop(true);

	FForestScatterBoundary Boundary;
	TestTrue(TEXT("Accept a closed outline"), Boundary.Build(*Spline));
	TestTrue(TEXT("Inside the narrow upper arm"), Boundary.Contains({50, 300}));
	TestTrue(TEXT("Inside the narrow right arm"), Boundary.Contains({300, 50}));
	TestFalse(TEXT("Empty notch inside the outline's bounding box"), Boundary.Contains({300, 300}));
	TestFalse(TEXT("Beyond the outline"), Boundary.Contains({-30, 100}));

	Spline->SetClosedLoop(false);
	TestFalse(TEXT("An unfinished outline provides no planting surface"), Boundary.Build(*Spline));
	TestFalse(TEXT("Rebuilding an unfinished outline clears the previous surface"), Boundary.Contains({50, 300}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterBuildingContactTest, "TunaSweeper.ForestScatter.GroundedBuildingFootprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterBuildingContactTest::RunTest(const FString& Parameters)
{
	const FBox LongBuilding(FVector(-200, -20, 0), FVector(200, 20, 150));
	const FTransform Placement(FRotator(0, 45, 0), FVector(0, 0, 5));
	FForestBuildingFootprints Buildings;
	TestFalse(TEXT("An empty building list permits planting"), Buildings.TouchesGround({0, 0, 0}, 10));
	Buildings.AddBox(LongBuilding, Placement);
	TestTrue(TEXT("Foundation within the 10 cm ground tolerance"), Buildings.TouchesGround({70, 70, 0}, 10));
	TestTrue(TEXT("Foundation set into the terrain"), Buildings.TouchesGround({70, 70, 30}, 10));
	TestFalse(TEXT("Empty corner inside the rotated world-aligned bounds"), Buildings.TouchesGround({100, -100, 0}, 10));
	TestFalse(TEXT("Vegetation may grow underneath an elevated building"), Buildings.TouchesGround({0, 0, -70}, 10));
	TestFalse(TEXT("Completely buried geometry leaves the surface available"), Buildings.TouchesGround({0, 0, 200}, 10));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterBuildingSelectionTest, "TunaSweeper.ForestScatter.BuildingFootprintSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterBuildingSelectionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).ShouldSimulatePhysics(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	AStaticMeshActor* Building = World->SpawnActor<AStaticMeshActor>();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Building"), Building) || !TestNotNull(TEXT("Foundation mesh"), Cube))
	{
		return false;
	}
	UStaticMeshComponent* Foundation = Building->GetStaticMeshComponent();
	Foundation->SetMobility(EComponentMobility::Movable);
	Foundation->SetStaticMesh(Cube);
	Foundation->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FForestBuildingFootprints Ordinary;
	Ordinary.AddBuilding(*Building);
	TestTrue(TEXT("A selected building contributes its colliding mesh footprint"), Ordinary.TouchesGround({0, 0, 0}, 10));
	Foundation->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FForestBuildingFootprints Decorative;
	Decorative.AddBuilding(*Building);
	TestFalse(TEXT("A decorative mesh has no automatic footprint"), Decorative.TouchesGround({0, 0, 0}, 10));
	Foundation->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	UBoxComponent* Marker = NewObject<UBoxComponent>(Building);
	Marker->SetupAttachment(Foundation);
	Marker->SetBoxExtent(FVector(30, 30, 5));
	Marker->SetRelativeLocation(FVector(200, 0, 0));
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->ComponentTags.Add(TEXT("ForestScatterFootprint"));
	Building->AddInstanceComponent(Marker);
	Marker->RegisterComponent();
	FForestBuildingFootprints Marked;
	Marked.AddBuilding(*Building);
	TestTrue(TEXT("A marked noncolliding foundation box is honored"), Marked.TouchesGround({200, 0, 0}, 10));
	TestFalse(TEXT("The explicit footprint replaces the mesh bounds"), Marked.TouchesGround({0, 0, 0}, 10));

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!TestNotNull(TEXT("Foundation with a curved underside"), Sphere))
	{
		return false;
	}
	Marker->ComponentTags.Reset();
	Foundation->SetStaticMesh(Sphere);
	Foundation->SetWorldLocation({0, 0, 47});
	FForestBuildingFootprints Curved;
	Curved.AddBuilding(*Building);
	TestTrue(TEXT("Ground touches the center of the curved foundation"), Curved.TouchesGround({0, 0, 0}, 5));
	TestFalse(TEXT("The elevated side is clear inside the same mesh bounds"), Curved.TouchesGround({43, 0, 0}, 5));

	Foundation->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UInstancedStaticMeshComponent* Repeated = NewObject<UInstancedStaticMeshComponent>(Building);
	Repeated->SetupAttachment(Foundation);
	Repeated->SetMobility(EComponentMobility::Movable);
	Repeated->SetStaticMesh(Sphere);
	Repeated->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Building->AddInstanceComponent(Repeated);
	Repeated->RegisterComponent();
	Repeated->AddInstance(FTransform(FVector(120, 0, 47)), /*bWorldSpace=*/true);
	Repeated->AddInstance(FTransform(FVector(320, 0, 100)), /*bWorldSpace=*/true);
	FForestBuildingFootprints Instanced;
	Instanced.AddBuilding(*Building);
	TestTrue(TEXT("The grounded instance touches its own support"), Instanced.TouchesGround({120, 0, 0}, 5));
	TestFalse(TEXT("The grounded instance also has an elevated side"), Instanced.TouchesGround({163, 0, 0}, 5));
	TestFalse(TEXT("The second instance stands above the ground"), Instanced.TouchesGround({320, 0, 0}, 5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterDensityTest, "TunaSweeper.ForestScatter.DensityAndSeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterDensityTest::RunTest(const FString& Parameters)
{
	const FBox2D Patch(FVector2D(0, 0), FVector2D(1000, 1000));
	FForestScatterGrid Grid;
	TestFalse(TEXT("Zero density omits the layer"), Grid.Build(Patch, 0));
	if (!TestTrue(TEXT("Build a 100 square metre patch"), Grid.Build(Patch, 4)))
	{
		return false;
	}
	TestEqual(TEXT("Four instances per 100 square metres"), Grid.Num(), 4);
	const FForestScatterSample First = Grid.Sample(0, 315);
	TestTrue(TEXT("Generated point is inside its cell"), FBox2D(FVector2D(0, 0), FVector2D(500, 500)).IsInsideOrOn(First.XY));
	TestTrue(TEXT("Regeneration keeps the sample"), Grid.Sample(0, 315).XY.Equals(First.XY));
	TestFalse(TEXT("A different seed changes its location"), Grid.Sample(0, 316).XY.Equals(First.XY));
	float MinScale = 1.0f, MaxScale = 1.0f;
	for (int32 Seed = 0; Seed < 64; ++Seed)
	{
		const FForestScatterSample A = Grid.Sample(0, Seed);
		const FForestScatterSample B = Grid.Sample(0, Seed);
		TestEqual(TEXT("Regeneration keeps the plant's scale"), A.Scale, B.Scale);
		TestEqual(TEXT("Regeneration keeps its heading"), A.Yaw, B.Yaw);
		TestTrue(TEXT("Heading can turn around the entire vertical axis"), A.Yaw >= 0.0f && A.Yaw < 360.0f);
		MinScale = FMath::Min(MinScale, A.Scale);
		MaxScale = FMath::Max(MaxScale, A.Scale);
	}
	TestTrue(TEXT("Uniform scale varies within five percent"), MinScale >= 0.95f && MaxScale <= 1.05f &&
		MinScale < 0.98f && MaxScale > 1.02f);
	Grid.Build(FBox2D(FVector2D(-500, -500), FVector2D(1000, 1000)), 4);
	TestTrue(TEXT("Enlarging the outline preserves plants in overlapping cells"), Grid.Sample(4, 315).XY.Equals(First.XY));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterFlowerPatchTest, "TunaSweeper.ForestScatter.FlowerColorPatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterFlowerPatchTest::RunTest(const FString&)
{
	TSet<int32> Seen;
	int32 MatchingNeighbors = 0, ChangedSeed = 0;
	for (int32 X = -10; X <= 10; ++X)
	{
		for (int32 Y = -10; Y <= 10; ++Y)
		{
			const FVector2D Position(351.0 * X, 397.0 * Y);
			const int32 Color = ForestScatter::FlowerMaterialIndex(Position, 600.0, 173, 4);
			TestEqual(TEXT("The same location retains its petal color"), Color, ForestScatter::FlowerMaterialIndex(Position, 600.0, 173, 4));
			TestTrue(TEXT("The index addresses a palette entry"), Color >= 0 && Color < 4);
			Seen.Add(Color);
			MatchingNeighbors += Color == ForestScatter::FlowerMaterialIndex(Position + FVector2D(25, 15), 600.0, 173, 4);
			ChangedSeed += Color != ForestScatter::FlowerMaterialIndex(Position, 600.0, 941, 4);
		}
	}
	TestEqual(TEXT("All four colors occur across the region"), Seen.Num(), 4);
	TestTrue(TEXT("Flowers within 30 cm usually share a patch"), MatchingNeighbors > 390);
	TestTrue(TEXT("Changing the region seed changes the patch layout"), ChangedSeed > 190);
	TestEqual(TEXT("A single color covers the whole region"), ForestScatter::FlowerMaterialIndex({-745, 250}, 600.0, 93, 1), 0);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterGameWorldTest, "TunaSweeper.ForestScatter.Editing.PreserveBakedGameWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterGameWorldTest::RunTest(const FString&)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Game world"), World))
	{
		return false;
	}
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	AForestScatterRegion* Region = World->SpawnActor<AForestScatterRegion>();
	if (!TestNotNull(TEXT("Forest region"), Region) || !TestNotNull(TEXT("Game-world PCG subsystem"), Region->PCGComponent->GetSubsystem())
		|| !TestNotNull(TEXT("The region has its authoring graph"), Region->PCGComponent->GetGraph()))
	{
		return false;
	}
	Region->RegenerateForest();
	TestFalse(TEXT("The authoring command preserves the baked forest in a game world"), Region->PCGComponent->IsGenerating());
	return true;
}

#endif
