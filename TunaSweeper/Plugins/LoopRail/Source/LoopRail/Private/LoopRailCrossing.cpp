#include "LoopRailCrossing.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "LoopRailMotion.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ALoopRailCrossing::ALoopRailCrossing()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (int32 I = 0; I < 2; ++I)
    {
        const float S = I == 0 ? 1 : -1;
        auto* Post = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Post%d"),I));
        Post->SetupAttachment(Root); Post->SetStaticMesh(Cube.Object);
        Post->SetRelativeLocation(FVector(-S*300,-S*220,55)); Post->SetRelativeScale3D(FVector(.25,.25,1.1));
        Post->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        auto* Hinge = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Hinge%d"),I));
        Hinge->SetupAttachment(Root); Hinge->SetRelativeLocation(FVector(-S*300,-S*220,110)); Hinges.Add(Hinge);
        auto* Bar = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Barrier%d"),I));
        Bar->SetupAttachment(Hinge); Bar->SetStaticMesh(Cube.Object);
        Bar->SetRelativeLocation(FVector(300,0,0)); Bar->SetRelativeScale3D(FVector(6,.12,.14));
        Bar->SetCollisionProfileName(TEXT("BlockAllDynamic")); Bar->SetCanEverAffectNavigation(false); Barriers.Add(Bar);
        auto* Lamp = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("WarningLamp%d"),I));
        Lamp->SetupAttachment(Root); Lamp->SetRelativeLocation(FVector(-S*300,-S*220,150));
        Lamp->SetLightColor(FLinearColor::Red); Lamp->SetIntensity(0); Lamp->SetAttenuationRadius(220); Lamp->SetCastShadows(false); Lamps.Add(Lamp);
    }
    Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("WarningAudio")); Audio->SetupAttachment(Root); Audio->bAutoActivate = false;
}
void ALoopRailCrossing::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); UpdatePresentation(); }
void ALoopRailCrossing::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<ALoopRailTrain> It(GetWorld()); It; ++It) AddTickPrerequisiteActor(*It);
    Audio->SetSound(WarningSound);
    Tick(0);
    // A level may start with a train already occupying the crossing.
    OpenAlpha = bWarning ? 0 : 1; UpdatePresentation();
}
void ALoopRailCrossing::AlignToTrack() { if (IsValid(Track)) SetActorTransform(Track->Sample(Track->FindDistance(GetActorLocation()))); }
void ALoopRailCrossing::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    bool bOccupied = false;
    if (IsValid(Track) && Track->GetLength() > 1)
    {
        const double CrossingDistance = Track->FindDistance(GetActorLocation());
        for (TActorIterator<ALoopRailTrain> It(GetWorld()); It; ++It)
        {
            if (It->Track != Track) continue;
            const double Approach = FMath::Max(WarningDistance, It->GetSpeed()*(BarrierSeconds+WarningLeadSeconds));
            if (LoopRailMotion::OccupiesCrossing(It->GetHeadDistance(), ALoopRailTrain::VehicleLength*.5,
                It->GetConsistLength(), CrossingDistance, Approach, RearClearance, Track->GetLength())) { bOccupied = true; break; }
        }
    }
    if (bOccupied != bWarning)
    {
        bWarning = bOccupied; OnWarningChanged.Broadcast(bWarning);
        if (bWarning && WarningSound) Audio->Play(); else Audio->Stop();
    }
    OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, bWarning ? 0.f : 1.f, DeltaSeconds, 1/FMath::Max(.1f,BarrierSeconds));
    UpdatePresentation();
}
void ALoopRailCrossing::UpdatePresentation()
{
    for (int32 I = 0; I < Hinges.Num(); ++I)
    {
        Hinges[I]->SetRelativeRotation(FRotator(OpenAlpha*90,I*180,0));
        const bool bLit = bWarning && (FMath::Fmod(GetWorld() ? GetWorld()->GetTimeSeconds() : 0, 1.f) < .5f) == (I == 0);
        Lamps[I]->SetIntensity(bLit ? 1500 : 0);
    }
}
