#include "Subsystem/TunaSweeperInteractionSubsystem.h"

#include "Engine/Engine.h"
#include "Character/TunaSweeperQuestNpcActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "Interaction/TunaSweeperSelfDestructInteractableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Stats/Stats.h"

void UTunaSweeperInteractionSubsystem::Tick(float DeltaTime)
{
	RefreshFocusedInteractable();
}

TStatId UTunaSweeperInteractionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperInteractionSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperInteractionSubsystem::IsTickable() const
{
	return GetWorld() && GetWorld()->IsGameWorld();
}

void UTunaSweeperInteractionSubsystem::RegisterInteractable(UTunaSweeperInteractableComponent* Interactable)
{
	if (IsValid(Interactable))
	{
		RegisteredInteractables.Add(Interactable);
	}
}

void UTunaSweeperInteractionSubsystem::UnregisterInteractable(UTunaSweeperInteractableComponent* Interactable)
{
	RegisteredInteractables.Remove(Interactable);

	if (FocusedInteractable.Get() == Interactable)
	{
		FocusedInteractable.Reset();
	}
}

UTunaSweeperInteractableComponent* UTunaSweeperInteractionSubsystem::GetFocusedInteractable() const
{
	return FocusedInteractable.Get();
}

bool UTunaSweeperInteractionSubsystem::TryInteract(APawn* InstigatorPawn)
{
	RefreshFocusedInteractable();
	return RequestInteraction(FocusedInteractable.Get(), InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::RequestInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn)
{
	if (!IsValid(Interactable) || !IsValid(InstigatorPawn) || !Interactable->IsWithinInteractionDistance(InstigatorPawn))
	{
		return false;
	}

	switch (Interactable->GetInteractionType())
	{
	case ETunaSweeperInteractionType::ItemPickup:
		return HandlePickupItemInteraction(Interactable);
	case ETunaSweeperInteractionType::ItemSpawn:
		return HandleItemSpawnInteraction(Interactable, InstigatorPawn);
	case ETunaSweeperInteractionType::LootContainerOpen:
		return HandleLootContainerOpenInteraction(Interactable, InstigatorPawn);
	case ETunaSweeperInteractionType::LootContainerSpawn:
		return HandleLootContainerSpawnInteraction(Interactable, InstigatorPawn);
	case ETunaSweeperInteractionType::LevelTravel:
		return HandleLevelTravelInteraction(Interactable, InstigatorPawn);
	case ETunaSweeperInteractionType::Quest:
		return HandleQuestInteraction(Interactable, InstigatorPawn);
	case ETunaSweeperInteractionType::SelfDestruct:
		return HandleSelfDestructInteraction(Interactable, InstigatorPawn);
	default:
		return false;
	}
}

bool UTunaSweeperInteractionSubsystem::HandlePickupItemInteraction(UTunaSweeperInteractableComponent* Interactable)
{
	ATunaSweeperPickupItemActor* PickupItemActor = Interactable
		? Cast<ATunaSweeperPickupItemActor>(Interactable->GetOwner())
		: nullptr;
	if (!PickupItemActor)
	{
		return false;
	}

	const FString ItemName = PickupItemActor->GetItemDisplayName().ToString();
	UTunaSweeperGameInstance* TunaGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	if (!TunaGameInstance || !TunaGameInstance->AddItemToFirstAvailableInventorySlot(PickupItemActor->GetItemId(), 1))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Red,
				FString::Printf(TEXT("[Interaction] Inventory full: %s"), *ItemName));
		}
		return false;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Green,
			FString::Printf(TEXT("[Interaction] 획득: %s"), *ItemName));
	}

	if (PickupItemActor->ShouldDestroyOnPickup())
	{
		PickupItemActor->Destroy();
	}

	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleItemSpawnInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperItemSpawnInteractableActor* ItemSpawnActor = Interactable
		? Cast<ATunaSweeperItemSpawnInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return ItemSpawnActor && ItemSpawnActor->SpawnRandomPickupItem(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleLootContainerOpenInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperLootContainerActor* LootContainerActor = Interactable
		? Cast<ATunaSweeperLootContainerActor>(Interactable->GetOwner())
		: nullptr;
	if (!LootContainerActor || !InstigatorPawn)
	{
		return false;
	}

	FTunaSweeperLootContainerInstance ContainerInstance;
	if (!LootContainerActor->BuildContainerInstance(ContainerInstance))
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	TunaPlayerController->OpenLootContainerPanel(ContainerInstance);
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleLootContainerSpawnInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperLootContainerSpawnInteractableActor* SpawnActor = Interactable
		? Cast<ATunaSweeperLootContainerSpawnInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return SpawnActor && SpawnActor->SpawnRandomLootContainer(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleLevelTravelInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperLevelTravelInteractableActor* LevelTravelActor = Interactable
		? Cast<ATunaSweeperLevelTravelInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return LevelTravelActor && LevelTravelActor->TravelToTargetLevel(InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::HandleQuestInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperQuestNpcActor* QuestNpcActor = Interactable
		? Cast<ATunaSweeperQuestNpcActor>(Interactable->GetOwner())
		: nullptr;
	if (!QuestNpcActor || !InstigatorPawn)
	{
		return false;
	}

	ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController());
	if (!TunaPlayerController)
	{
		return false;
	}

	TunaPlayerController->OpenQuestPanel(QuestNpcActor->GetQuestId());
	return true;
}

bool UTunaSweeperInteractionSubsystem::HandleSelfDestructInteraction(
	UTunaSweeperInteractableComponent* Interactable,
	APawn* InstigatorPawn)
{
	ATunaSweeperSelfDestructInteractableActor* SelfDestructActor = Interactable
		? Cast<ATunaSweeperSelfDestructInteractableActor>(Interactable->GetOwner())
		: nullptr;
	return SelfDestructActor && SelfDestructActor->StartSelfDestruct(InstigatorPawn);
}

void UTunaSweeperInteractionSubsystem::RefreshFocusedInteractable()
{
	UWorld* World = GetWorld();
	const APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!PlayerPawn)
	{
		FocusedInteractable.Reset();
		return;
	}

	UTunaSweeperInteractableComponent* ClosestInteractable = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (auto InteractableIt = RegisteredInteractables.CreateIterator(); InteractableIt; ++InteractableIt)
	{
		UTunaSweeperInteractableComponent* Interactable = InteractableIt->Get();
		if (!IsValid(Interactable))
		{
			InteractableIt.RemoveCurrent();
			continue;
		}

		if (!Interactable->IsWithinInteractionDistance(PlayerPawn))
		{
			continue;
		}

		const float DistanceSquared = Interactable->GetSquaredDistance2DTo(PlayerPawn);
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestInteractable = Interactable;
		}
	}

	FocusedInteractable = ClosestInteractable;
}
