#include "TunaRocketDemoRange.h"
#include "TunaGuidedRocket.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ATunaRocketDemoRange::ATunaRocketDemoRange()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}
void ATunaRocketDemoRange::BeginPlay()
{
    Super::BeginPlay();
    auto* Marker = GetWorld()->SpawnActor<AStaticMeshActor>();
    Marker->SetMobility(EComponentMobility::Movable);
    Marker->GetStaticMeshComponent()->SetStaticMesh(TargetMesh);
    Marker->GetStaticMeshComponent()->SetMaterial(0, TargetMaterial);
    Marker->SetActorScale3D(FVector(34, 34, 88));
    Marker->SetActorEnableCollision(false);
    Target = Marker;
    Target->SetActorLocation(GetActorLocation() + FVector(1800, 0, 0));
    Fire();
}
void ATunaRocketDemoRange::Fire()
{
    if (!RocketClass || !Target) return;
    for (int32 Index = 0; Index < 6; ++Index)
    {
        const FVector Start = GetActorLocation() + FVector(0, Index == 5 ? -900 : (Index - 2) * 30, 0);
        const FTransform Transform(FRotator(0, Index == 5 ? 0 : (Index - 2) * 3.f, 0), Start);
        auto* Rocket = GetWorld()->SpawnActorDeferred<ATunaGuidedRocket>(RocketClass, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Rocket) continue;
        Rocket->TargetActor = Index == 5 ? nullptr : Target.Get();
        Rocket->FinishSpawning(Transform);
    }
}
void ATunaRocketDemoRange::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Cycle += DeltaSeconds;
    if (Target) Target->SetActorLocation(GetActorLocation() + FVector(1800, FMath::Min(Cycle, 2.f) * 400, 0));
    if (Cycle >= 4.f)
    {
        Cycle = 0;
        if (Target) Target->SetActorLocation(GetActorLocation() + FVector(1800, 0, 0));
        Fire();
    }
}
void ATunaRocketDemoRange::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(Target)) Target->Destroy();
    Super::EndPlay(Reason);
}
