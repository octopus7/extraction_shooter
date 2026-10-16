#include "Interaction/TunaSweeperParcelContainerActor.h"

#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Templates/UnrealTemplate.h"

ATunaSweeperParcelContainerActor::ATunaSweeperParcelContainerActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
    Interactable = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
    Interactable->SetupAttachment(RootComponent);
    Interactable->SetRelativeLocation(FVector(0, 0, 65));
    Interactable->SetInteractionTypeDisplayNameAndStringKey(
        ETunaSweeperInteractionType::None, FText::GetEmpty(), TEXT("ui.interaction.pickup_parcel"));
    CreateDefaultSubobject<UTunaSweeperVisionSubjectComponent>(TEXT("VisionSubject"));
}

void ATunaSweeperParcelContainerActor::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->GetInventorySlots(); // Restore the selected save before evaluating quest presence.
        if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
            Quests->OnQuestProgressChanged.AddUObject(this, &ThisClass::RefreshPresence);
    }
    RefreshPresence();
}

void ATunaSweeperParcelContainerActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
        if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
            Quests->OnQuestProgressChanged.RemoveAll(this);
    Super::EndPlay(Reason);
}

int32 ATunaSweeperParcelContainerActor::ResolvePickupItemId() const
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
    if (!Quests || Game->IsCombatTestSession() || QuestId <= 0 || ObjectiveId.IsNone() ||
        Quests->GetQuestState(QuestId) != ETunaSweeperQuestState::Accepted) return 0;
    const auto* Definition = Quests->FindQuestDefinition(QuestId);
    if (!Definition) return 0;
    const auto* Objective = Definition->Objectives.FindByPredicate(
        [this](const auto& Value) { return Value.ObjectiveId == ObjectiveId; });
    if (!Objective || Objective->Type != ETunaSweeperObjectiveType::ItemSubmitted ||
        Objective->RequiredCount != 1 || Objective->TargetProviderId.IsNone()) return 0;
    TArray<FTunaSweeperObjectiveProgressView> Progress;
    Quests->GetQuestObjectiveProgress(QuestId, Progress);
    if (Progress.ContainsByPredicate([this](const auto& Value)
        { return Value.ObjectiveId == ObjectiveId && Value.bCompleted; })) return 0;
    auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
    FTunaSweeperItemDefinition Item;
    return Items && Items->TryGetItemDefinition(Objective->ItemId, Item) && Item.IsVolatile() ? Objective->ItemId : 0;
}

bool ATunaSweeperParcelContainerActor::CanPickUp() const
{
    return !bCollected && !bCollecting && ResolvePickupItemId() > 0;
}

bool ATunaSweeperParcelContainerActor::PickUp(APawn* Player)
{
    if (!Player || Player->GetWorld() != GetWorld() || !Player->IsPlayerControlled() || IsHidden() ||
        !CanPickUp() || !Interactable->IsWithinInteractionDistance(Player) ||
        FVector::DistSquared(Interactable->GetInteractionLocation(), Player->GetActorLocation()) >
            FMath::Square(Interactable->GetInteractionDistance())) return false;
    const int32 ItemId = ResolvePickupItemId();
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    TGuardValue<bool> Guard(bCollecting, true);
    // Inventory callbacks must not collect twice or observe an available box after granting.
    bCollected = true;
    RefreshPresence();
    if (!Game->AddItemToFirstAvailableInventorySlot(ItemId, 1))
    {
        bCollected = false;
        RefreshPresence();
        return false;
    }
    Game->SaveGameState();
    return true;
}

void ATunaSweeperParcelContainerActor::RefreshPresence()
{
    const bool bAvailable = !bCollected && ResolvePickupItemId() > 0;
    SetActorHiddenInGame(!bAvailable);
    SetActorEnableCollision(bAvailable);
    Interactable->SetInteractionTypeDisplayNameAndStringKey(
        bAvailable ? ETunaSweeperInteractionType::WorldProgress : ETunaSweeperInteractionType::None,
        FText::GetEmpty(), TEXT("ui.interaction.pickup_parcel"));
}
