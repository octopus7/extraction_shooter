#include "Interaction/TunaSweeperDeliveryDestinationActor.h"

#include "Components/SceneComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperDeliveryDestinationActor::ATunaSweeperDeliveryDestinationActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Interactable = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	Interactable->SetupAttachment(RootComponent);
	Interactable->SetRelativeLocation(FVector(0, 0, 80));
	Interactable->SetInteractionTypeDisplayNameAndStringKey(
		ETunaSweeperInteractionType::None, FText::GetEmpty(), TEXT("ui.interaction.deliver_parcel"));
}

void ATunaSweeperDeliveryDestinationActor::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshInteraction);
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.AddUObject(this, &ThisClass::RefreshInteraction);
	}
	RefreshInteraction();
}

void ATunaSweeperDeliveryDestinationActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnInventoryStateChanged.RemoveAll(this);
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>()) Quests->OnQuestProgressChanged.RemoveAll(this);
	}
	Super::EndPlay(Reason);
}

bool ATunaSweeperDeliveryDestinationActor::CanDeliver() const
{
	const auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
	const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	return Quests && Quests->CanSubmitItemsForObjective(QuestId, ObjectiveId, DestinationId);
}

bool ATunaSweeperDeliveryDestinationActor::Deliver(APawn* Player)
{
	if (!Player || Player->GetWorld() != GetWorld() || !Player->IsPlayerControlled() || IsHidden() ||
		!CanDeliver() || !Interactable->IsWithinInteractionDistance(Player) ||
		FVector::DistSquared(Interactable->GetInteractionLocation(), Player->GetActorLocation()) >
			FMath::Square(Interactable->GetInteractionDistance())) return false;
	auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
	return Game->GetSubsystem<UTunaSweeperQuestSubsystem>()->TrySubmitItemsForObjective(QuestId, ObjectiveId, DestinationId);
}

void ATunaSweeperDeliveryDestinationActor::RefreshInteraction()
{
	Interactable->SetInteractionTypeDisplayNameAndStringKey(
		CanDeliver() ? ETunaSweeperInteractionType::WorldProgress : ETunaSweeperInteractionType::None,
		FText::GetEmpty(), TEXT("ui.interaction.deliver_parcel"));
}
