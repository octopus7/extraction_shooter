#include "LoopRailStation.h"
#include "LoopRailTrack.h"
#include "Components/SceneComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ALoopRailStation::ALoopRailStation()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Static);
    StopMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("LocomotiveStop")); StopMarker->SetupAttachment(Root);
    Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform")); Platform->SetupAttachment(Root);
    AccessRamp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccessRamp")); AccessRamp->SetupAttachment(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto* Mesh : {Platform.Get(), AccessRamp.Get()})
    { Mesh->SetStaticMesh(Cube.Object); Mesh->SetMobility(EComponentMobility::Static); Mesh->SetCollisionProfileName(TEXT("BlockAll")); }
}
void ALoopRailStation::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const float Side = bPlatformOnRight ? 1.f : -1.f;
    const float Width = FMath::Max(100.f, PlatformWidth), Length = FMath::Max(200.f, PlatformLength);
    Platform->SetRelativeLocation(FVector(450-Length*.5, Side*(115+Width*.5), 30));
    Platform->SetRelativeScale3D(FVector(Length/100, Width/100, .6));
    AccessRamp->SetRelativeLocation(FVector(250, Side*(115+Width+150), 10));
    AccessRamp->SetRelativeRotation(FRotator(0,0,Side*11.31f));
    AccessRamp->SetRelativeScale3D(FVector(2, 3.06, .4));
    for (auto* Mesh : {Platform.Get(), AccessRamp.Get()})
    { Mesh->SetVisibility(bShowPlatform); Mesh->SetCollisionEnabled(bShowPlatform ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision); }
}
double ALoopRailStation::GetStopDistance() const { return IsValid(Track) ? Track->FindDistance(GetActorLocation()) : 0; }
void ALoopRailStation::AlignToTrack()
{
    if (IsValid(Track)) { SetActorTransform(Track->Sample(GetStopDistance())); OnConstruction(GetActorTransform()); }
}
