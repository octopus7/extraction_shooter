#include "LoopRailTrack.h"
#include "LoopRailMotion.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALoopRailTrack::ALoopRailTrack()
{
    Spline = CreateDefaultSubobject<USplineComponent>(TEXT("RailPath"));
    SetRootComponent(Spline);
    Spline->SetMobility(EComponentMobility::Static);
    Spline->ClearSplinePoints(false);
    // Counter-clockwise rounded rectangle, generous enough for four 9m vehicles.
    for (const FVector& P : { FVector(-5000,-2500,0), FVector(5000,-2500,0), FVector(6500,0,0),
                             FVector(5000,2500,0), FVector(-5000,2500,0), FVector(-6500,0,0) })
        Spline->AddSplinePoint(P, ESplineCoordinateSpace::Local, false);
    Spline->SetClosedLoop(true);
    Rails = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rails"));
    Sleepers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Sleepers"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto* Mesh : {Rails.Get(), Sleepers.Get()})
    {
        Mesh->SetupAttachment(Spline);
        Mesh->SetMobility(EComponentMobility::Static);
        Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCanEverAffectNavigation(false);
    }
}

void ALoopRailTrack::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); RebuildTrack(); }
double ALoopRailTrack::GetLength() const { return Spline->GetSplineLength(); }
bool ALoopRailTrack::IsUsableLoop() const
{
    return Spline->IsClosedLoop() && Spline->GetNumberOfSplinePoints() >= 3 && GetLength() > 1
        && GetActorScale3D().Equals(FVector::OneVector, .001);
}
double ALoopRailTrack::FindDistance(const FVector& Location) const
{ return Spline->GetDistanceAlongSplineAtLocation(Location, ESplineCoordinateSpace::World); }
FTransform ALoopRailTrack::Sample(double Distance) const
{
    FTransform Result = Spline->GetTransformAtDistanceAlongSpline(LoopRailMotion::Wrap(Distance, GetLength()), ESplineCoordinateSpace::World, false);
    Result.SetScale3D(FVector::OneVector);
    return Result;
}
void ALoopRailTrack::RebuildTrack()
{
    Rails->ClearInstances(); Sleepers->ClearInstances();
    const double Length = GetLength();
    if (Length < 1) return;
    const int32 Segments = FMath::Clamp(FMath::CeilToInt(Length / FMath::Max(25.f, RailSegmentLength)), 1, 20000);
    for (int32 I = 0; I < Segments; ++I)
    {
        const double A = I * Length / Segments, B = (I + 1) * Length / Segments;
        for (float Side : {-1.f, 1.f})
        {
            const FTransform TA = Spline->GetTransformAtDistanceAlongSpline(A, ESplineCoordinateSpace::Local, false);
            const FTransform TB = Spline->GetTransformAtDistanceAlongSpline(B, ESplineCoordinateSpace::Local, false);
            const FVector PA = TA.TransformPosition(FVector(0, Side * Gauge * .5, -3));
            const FVector PB = TB.TransformPosition(FVector(0, Side * Gauge * .5, -3));
            Rails->AddInstance(FTransform((PB-PA).Rotation(), (PA+PB)*.5, FVector((PB-PA).Size()/100 + .01, .06, .06)));
        }
    }
    const int32 Count = FMath::Clamp(FMath::CeilToInt(Length / FMath::Max(25.f, SleeperSpacing)), 1, 20000);
    for (int32 I = 0; I < Count; ++I)
    {
        FTransform T = Spline->GetTransformAtDistanceAlongSpline(I * Length / Count, ESplineCoordinateSpace::Local, false);
        T.AddToTranslation(T.GetRotation().RotateVector(FVector(0,0,-9)));
        T.SetScale3D(FVector(.18, (Gauge+60)/100, .12));
        Sleepers->AddInstance(T);
    }
}
