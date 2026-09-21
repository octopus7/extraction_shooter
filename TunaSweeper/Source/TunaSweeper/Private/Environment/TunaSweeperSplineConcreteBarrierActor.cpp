#include "Environment/TunaSweeperSplineConcreteBarrierActor.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperSplineConcreteBarrierActor::ATunaSweeperSplineConcreteBarrierActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bRunConstructionScriptOnDrag = true;

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
	for (USplineMeshComponent* MeshComponent : SplineMeshes)
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
		USplineMeshComponent* MeshComponent = NewObject<USplineMeshComponent>(
			this, *FString::Printf(TEXT("BarrierSegment_%d"), SegmentIndex++), RF_Transactional);
		MeshComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->SetStaticMesh(BarrierMesh);
		MeshComponent->SetForwardAxis(ESplineMeshAxis::X, false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetupAttachment(Spline);
		AddInstanceComponent(MeshComponent);
		MeshComponent->RegisterComponent();

		FVector StartLocation, StartTangent, EndLocation, EndTangent;
		StartLocation = Spline->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		StartTangent = Spline->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local);
		EndLocation = Spline->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		EndTangent = Spline->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local);
		MeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);
		SplineMeshes.Add(MeshComponent);
	}
}
