#include "Interaction/TunaSweeperBombObstacleActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Weapon/TunaSweeperThrowableDamageType.h"
#include "Engine/DamageEvents.h"
#include "UObject/ConstructorHelpers.h"
ATunaSweeperBombObstacleActor::ATunaSweeperBombObstacleActor()
{
    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObstacleMesh"));
    SetRootComponent(ObstacleMesh);
    ObstacleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlockingVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingVolume"));
    BlockingVolume->SetupAttachment(ObstacleMesh);
    BlockingVolume->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) ObstacleMesh->SetStaticMesh(Cube.Object);
    SetCanBeDamaged(true);
}
void ATunaSweeperBombObstacleActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateBlockingVolume();
}
void ATunaSweeperBombObstacleActor::UpdateBlockingVolume()
{
    // Explicit primitive collision also works with temporary meshes lacking cooked collision.
    ObstacleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FVector Min, Max;
    ObstacleMesh->GetLocalBounds(Min, Max);
    BlockingVolume->SetRelativeLocation((Min + Max) * 0.5);
    BlockingVolume->SetBoxExtent(((Max - Min) * 0.5).ComponentMax(FVector(1)));
}
void ATunaSweeperBombObstacleActor::BeginPlay()
{
    Super::BeginPlay();
    UpdateBlockingVolume();
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>()) Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::RefreshObstacle);
    RefreshObstacle();
}
void ATunaSweeperBombObstacleActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>()) Game->OnWorldProgressChanged.RemoveAll(this);
    Super::EndPlay(Reason);
}
void ATunaSweeperBombObstacleActor::RefreshObstacle()
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    FTunaSweeperWorldProgressSaveData State;
    const bool bOpen = Game && !ProgressObjectId.IsNone() && Game->TryGetWorldProgressState(ProgressObjectId, State) && State.State == ETunaSweeperWorldProgressState::Completed;
    SetActorHiddenInGame(bOpen);
    SetActorEnableCollision(!bOpen);
    SetCanBeDamaged(!bOpen);
}
float ATunaSweeperBombObstacleActor::TakeDamage(float Amount, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer)
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    const bool bBomb = Event.DamageTypeClass && (Event.DamageTypeClass->IsChildOf(UTunaSweeperGrenadeDamageType::StaticClass()) || Event.DamageTypeClass->IsChildOf(UTunaSweeperDynamiteDamageType::StaticClass()));
    if (!Game || ProgressObjectId.IsNone() || !CanBeDamaged() || !bBomb || !FMath::IsFinite(Amount) || Amount <= 0) return 0;
    Game->UpdateWorldProgressState(ProgressObjectId, TEXT("world_progress.region_passage"), ETunaSweeperWorldProgressState::Completed, 1, 1, true);
    RefreshObstacle();
    return Amount;
}
