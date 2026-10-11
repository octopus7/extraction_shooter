#include "Interaction/TunaSweeperMemoAnchor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperMemoSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperMemoAnchor::ATunaSweeperMemoAnchor()
{
    PrimaryActorTick.bCanEverTick = false;
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    SetRootComponent(VisualMesh);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(
        TEXT("/Game/Meshes/Props/MemoStorageDevice/SM_MemoStorageDevice_LowPoly.SM_MemoStorageDevice_LowPoly"));
    if (Mesh.Succeeded()) VisualMesh->SetStaticMesh(Mesh.Object);

    InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
    InteractableComponent->SetupAttachment(VisualMesh);
    InteractableComponent->SetRelativeLocation(FVector(0.0, 0.0, 20.0));
    InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
        ETunaSweeperInteractionType::Memo, FText::GetEmpty(), TEXT("ui.interaction.memo"));
}

void ATunaSweeperMemoAnchor::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        // Finish loading the selected save before subscribing to its completion event.
        Game->IsMemoAcquired(MemoId);
        BoundGameInstance = Game;
        MemoStateChangedHandle = Game->OnMemoStateChanged.AddUObject(this, &ThisClass::RefreshMemoState);
    }
    RefreshMemoState();
}

void ATunaSweeperMemoAnchor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (auto* Game = BoundGameInstance.Get()) Game->OnMemoStateChanged.Remove(MemoStateChangedHandle);
    BoundGameInstance.Reset();
    Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperMemoAnchor::CanCollectMemo() const
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game || MemoId <= 0 || Game->IsMemoAcquired(MemoId)) return false;
    auto* Memos = Game->GetSubsystem<UTunaSweeperMemoSubsystem>();
    FTunaSweeperMemoDefinition Definition;
    return Memos && Memos->TryGetMemoDefinition(MemoId, Definition);
}

bool ATunaSweeperMemoAnchor::CollectMemo()
{
    if (!CanCollectMemo()) return false;
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game->MarkMemoAcquired(MemoId, true)) return false;
    RefreshMemoState();
    return true;
}

void ATunaSweeperMemoAnchor::RefreshMemoState()
{
    const bool bAvailable = CanCollectMemo();
    // Retain the placed instance so loading another save in this world can restore it.
    SetActorHiddenInGame(!bAvailable);
    SetActorEnableCollision(bAvailable);
    InteractableComponent->SetVisibility(bAvailable, true);
}
