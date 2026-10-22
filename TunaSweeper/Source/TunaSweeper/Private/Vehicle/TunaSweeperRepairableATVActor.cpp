#include "Vehicle/TunaSweeperRepairableATVActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicle/TunaSweeperATVActor.h"

ATunaSweeperRepairableATVActor::ATunaSweeperRepairableATVActor()
{
    // Keep the shared repair transaction and interaction, but present an immovable vehicle shell.
    FacilityMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FacilityMesh->SetCanEverAffectNavigation(false);
    VehicleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(RootComponent);
    VehicleMesh->SetCollisionProfileName(TEXT("BlockAll"));
    VehicleMesh->SetSimulatePhysics(false);
    VehicleMesh->SetEnableGravity(false);
    VehicleMesh->SetCanEverAffectNavigation(true);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/Meshes/Props/ATV/SKM_ATV.SKM_ATV"));
    if (Mesh.Succeeded()) VehicleMesh->SetSkeletalMesh(Mesh.Object);
    static ConstructorHelpers::FObjectFinder<UPhysicsAsset> Physics(TEXT("/Game/Meshes/Props/ATV/PA_ATV.PA_ATV"));
    if (Physics.Succeeded()) VehicleMesh->SetPhysicsAsset(Physics.Object);

    RequiredItemId = 6016;
    RequiredItemQuantity = 1;
    bRequiresItem = true;
    bConsumeRequiredItem = true;
    Interactable->SetRelativeLocation(FVector(0.0, 0.0, 140.0));
    FaultSparks->SetRelativeLocation(FVector(-25.0, 0.0, 45.0));
}

void ATunaSweeperRepairableATVActor::BeginPlay()
{
    // The base loads persisted repair state. Its later inventory/quest callbacks only refresh scenery.
    Super::BeginPlay();
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() || !IsRepaired() || RequiredRewardQuestId <= 0 || !RepairedVehicleClass)
        return;
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
    if (!Quests || Quests->GetQuestState(RequiredRewardQuestId) != ETunaSweeperQuestState::RewardCompleted)
        return;

    // Only BeginPlay may replace the shell: claiming the reward while this map is open cannot spawn it.
    FActorSpawnParameters Spawn;
    Spawn.OverrideLevel = GetLevel();
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Spawn.CustomPreSpawnInitalization = [this](AActor* Actor)
    {
        CastChecked<ATunaSweeperATVActor>(Actor)->FuelSessionId = ProgressObjectId;
    };
    if (World->SpawnActor<ATunaSweeperATVActor>(RepairedVehicleClass, GetActorTransform(), Spawn))
        Destroy();
}
