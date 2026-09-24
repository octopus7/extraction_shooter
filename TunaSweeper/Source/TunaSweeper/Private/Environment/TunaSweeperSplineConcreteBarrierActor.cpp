#include "Environment/TunaSweeperSplineConcreteBarrierActor.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperSplineConcreteBarrierActor::ATunaSweeperSplineConcreteBarrierActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);
	Spline->SetClosedLoop(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(
		TEXT("/Game/Meshes/Props/Underconstrution/Cute_Concrete_Barrier/StaticMeshes/SM_Concrete_Barrier.SM_Concrete_Barrier"));
	if (MeshFinder.Succeeded())
	{
		BarrierMesh = MeshFinder.Object;
	}
}

void ATunaSweeperSplineConcreteBarrierActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bSnapToLandscape)
	{
		SnapSplinePointsToLandscape();
	}
	RebuildSplineMeshes();
}

void ATunaSweeperSplineConcreteBarrierActor::SnapSplinePointsToLandscape()
{
	UWorld* World = GetWorld();
	if (!World || !Spline)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SplineConcreteBarrierLandscapeSnap), false, this);
	const int32 PointCount = Spline->GetNumberOfSplinePoints();
	for (int32 PointIndex = 0; PointIndex < PointCount; ++PointIndex)
	{
		const FVector WorldPoint = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
		const FVector TraceStart = WorldPoint + FVector(0.0f, 0.0f, LandscapeTraceDistance * 0.5f);
		const FVector TraceEnd = WorldPoint - FVector(0.0f, 0.0f, LandscapeTraceDistance * 0.5f);

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, LandscapeTraceChannel, QueryParams))
		{
			FVector LocalPoint = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::Local);
			LocalPoint.Z += Hit.Location.Z - WorldPoint.Z;
			Spline->SetLocationAtSplinePoint(PointIndex, LocalPoint, ESplineCoordinateSpace::Local, false);
		}
	}
}

void ATunaSweeperSplineConcreteBarrierActor::RebuildSplineMeshes()
{
	for (UStaticMeshComponent* MeshComponent : SplineMeshes)
	{
		if (IsValid(MeshComponent))
		{
			MeshComponent->DestroyComponent();
		}
	}
	SplineMeshes.Reset();

	if (!Spline || !BarrierMesh)
	{
		return;
	}

	const float SplineLength = Spline->GetSplineLength();
	const float MeshLength = FMath::Max(1.0f, BarrierMesh->GetBounds().BoxExtent.X * 2.0f);
	int32 SegmentIndex = 0;
	for (float StartDistance = 0.0f; StartDistance < SplineLength - KINDA_SMALL_NUMBER; StartDistance += MeshLength)
	{
		const float EndDistance = FMath::Min(StartDistance + MeshLength, SplineLength);
		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(
			this, *FString::Printf(TEXT("BarrierSegment_%d"), SegmentIndex++), RF_Transactional);
		MeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		// Static children cannot attach to a movable spline in a placed level actor.
		MeshComponent->SetMobility(Spline->Mobility);
		MeshComponent->SetStaticMesh(BarrierMesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetupAttachment(Spline);
		AddInstanceComponent(MeshComponent);
		MeshComponent->RegisterComponent();

		const float CenterDistance = (StartDistance + EndDistance) * 0.5f;
		const FVector CenterLocation = Spline->GetLocationAtDistanceAlongSpline(CenterDistance, ESplineCoordinateSpace::Local);
		const FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(CenterDistance, ESplineCoordinateSpace::Local);
		MeshComponent->SetRelativeLocation(CenterLocation);
		MeshComponent->SetRelativeRotation(Tangent.Rotation());
		SplineMeshes.Add(MeshComponent);
	}
}
