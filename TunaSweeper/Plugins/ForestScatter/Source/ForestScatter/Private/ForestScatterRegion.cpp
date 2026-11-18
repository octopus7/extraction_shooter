#include "ForestScatterRegion.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "PCGComponent.h"
#include "PCGGraph.h"

AForestScatterRegion::AForestScatterRegion()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);

	Boundary = CreateDefaultSubobject<USplineComponent>(TEXT("Boundary"));
	Boundary->ComponentTags.Add(TEXT("Boundary"));
	Boundary->SetupAttachment(SceneRoot);
	Boundary->SetMobility(EComponentMobility::Static);
	Boundary->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Boundary->SetCanEverAffectNavigation(false);
	const TArray<FVector> Corners = {{-1000, -1000, 0}, {1000, -1000, 0}, {1000, 1000, 0}, {-1000, 1000, 0}};
	Boundary->SetSplinePoints(Corners, ESplineCoordinateSpace::Local, false);
	for (int32 Index = 0; Index < Corners.Num(); ++Index)
	{
		Boundary->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}
	Boundary->SetClosedLoop(true);

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
	PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
	DefaultForestGraph = FSoftObjectPath(TEXT("/Game/Environment/ForestScatter/PCG_ForestScatter.PCG_ForestScatter"));

	Shrubs.Mesh = FSoftObjectPath(TEXT("/Game/Environment/ForestScatter/Meshes/SM_ForestShrub.SM_ForestShrub"));
	Shrubs.InstancesPer100SquareMeters = 4.0f;
	Flowers.Mesh = FSoftObjectPath(TEXT("/Game/Environment/ForestScatter/Meshes/SM_ForestFlower.SM_ForestFlower"));
	Flowers.InstancesPer100SquareMeters = 18.0f;
	Grass.Mesh = FSoftObjectPath(TEXT("/Game/Environment/ForestScatter/Meshes/SM_ForestLawnGrass.SM_ForestLawnGrass"));
	Grass.InstancesPer100SquareMeters = 35.0f;
	Gravel.Mesh = FSoftObjectPath(TEXT("/Game/Environment/ForestScatter/Meshes/SM_ForestGravel.SM_ForestGravel"));
	Gravel.InstancesPer100SquareMeters = 14.0f;
	Boulders.Mesh = FSoftObjectPath(TEXT("/Game/Environment/ForestScatter/Meshes/SM_ForestBoulder.SM_ForestBoulder"));
	Boulders.InstancesPer100SquareMeters = 0.22f;
	for (const TCHAR* Name : {TEXT("MI_ForestPetalWhite"), TEXT("MI_ForestPetalYellow"), TEXT("MI_ForestPetalRose"), TEXT("MI_ForestPetalLilac")})
	{
		FlowerPetalMaterials.Emplace(FSoftObjectPath(FString::Printf(TEXT("/Game/Environment/ForestScatter/Materials/%s.%s"), Name, Name)));
	}
}

TStaticArray<const FForestScatterLayer*, 6> AForestScatterRegion::GetGroundCoverLayers() const
{
	// Slot four previously generated hydrangeas. Gravel and boulders still use offsets five and six.
	return {&Shrubs, &Flowers, &Grass, nullptr, &Gravel, &Boulders};
}

void AForestScatterRegion::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	AssignForestGraph();
}

void AForestScatterRegion::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!Boundary->IsClosedLoop())
	{
		Boundary->SetClosedLoop(true);
	}
}

void AForestScatterRegion::AssignForestGraph()
{
	if (PCGComponent && !PCGComponent->GetGraph() && !IsTemplate())
	{
		if (UPCGGraph* Graph = DefaultForestGraph.LoadSynchronous())
		{
			PCGComponent->SetGraphLocal(Graph);
		}
	}
}

void AForestScatterRegion::RegenerateForest()
{
#if WITH_EDITOR
	if (!QueuedRegeneration.IsValid() && TryRegenerateInEditor(0.0f))
	{
		QueuedRegeneration = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &AForestScatterRegion::TryRegenerateInEditor));
	}
#endif
}

#if WITH_EDITOR
bool AForestScatterRegion::TryRegenerateInEditor(float)
{
	const UWorld* World = GetWorld();
	if (!World || World->WorldType != EWorldType::Editor || World->bIsTearingDown || IsActorBeingDestroyed() || !PCGComponent)
	{
		return false;
	}
	// A native PCG generation drops new requests while busy. Recheck on the next editor frame, after
	// PCG's completion/cancellation callbacks and cleanup have finished, and coalesce repeated clicks.
	if (PCGComponent->IsGenerating() || PCGComponent->IsCleaningUp())
	{
		return true;
	}
	QueuedRegeneration.Reset();
	AssignForestGraph();
	if (PCGComponent->GetGraph())
	{
		PCGComponent->GenerateLocal(/*bForce=*/true);
	}
	return false;
}
#endif
