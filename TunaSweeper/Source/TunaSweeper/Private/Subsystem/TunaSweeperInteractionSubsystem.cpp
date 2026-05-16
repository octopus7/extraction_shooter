#include "Subsystem/TunaSweeperInteractionSubsystem.h"

#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Character/TunaSweeperQuestNpcActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
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
#include "UI/TunaSweeperTempOpenLootWidget.h"

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

	const FString DebugTypeName = GetInteractionDebugTypeName(Interactable);
	const FString DisplayName = Interactable->GetInteractionDisplayName().ToString();
	const FString DebugMessage = FString::Printf(TEXT("[Interaction] %s: %s"), *DebugTypeName, *DisplayName);

	switch (Interactable->GetInteractionType())
	{
	case ETunaSweeperInteractionType::Open:
		return OpenTempOpenLootWidget(InstigatorPawn);
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
		break;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White, DebugMessage);
	}

	return true;
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

bool UTunaSweeperInteractionSubsystem::OpenTempOpenLootWidget(APawn* InstigatorPawn)
{
	if (!IsValid(InstigatorPawn))
	{
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(InstigatorPawn->GetController());
	if (!PlayerController)
	{
		return false;
	}

	if (ActiveTempOpenLootWidget.IsValid() && ActiveTempOpenLootWidget->IsInViewport())
	{
		return true;
	}

	static const FSoftClassPath TempOpenLootWidgetClassPath(TEXT("/Game/UI/WBP_TempOpenLootTileView.WBP_TempOpenLootTileView_C"));
	TSubclassOf<UTunaSweeperTempOpenLootWidget> WidgetClass = Cast<UClass>(TempOpenLootWidgetClassPath.TryLoad());
	if (!WidgetClass)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("[Interaction] Missing WBP_TempOpenLootTileView."));
		}
		return false;
	}

	UTunaSweeperTempOpenLootWidget* Widget = CreateWidget<UTunaSweeperTempOpenLootWidget>(PlayerController, WidgetClass);
	if (!Widget)
	{
		return false;
	}

	Widget->AddToViewport(10);
	ActiveTempOpenLootWidget = Widget;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;

	return true;
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

FString UTunaSweeperInteractionSubsystem::GetInteractionDebugTypeName(const UTunaSweeperInteractableComponent* Interactable) const
{
	if (!Interactable)
	{
		return TEXT("None");
	}

	switch (Interactable->GetInteractionType())
	{
	case ETunaSweeperInteractionType::Dialogue:
		return TEXT("Dialogue");
	case ETunaSweeperInteractionType::Pickup:
		return TEXT("Pickup");
	case ETunaSweeperInteractionType::Open:
		return TEXT("Open");
	case ETunaSweeperInteractionType::ItemPickup:
		return TEXT("ItemPickup");
	case ETunaSweeperInteractionType::ItemSpawn:
		return TEXT("ItemSpawn");
	case ETunaSweeperInteractionType::LootContainerOpen:
		return TEXT("LootContainerOpen");
	case ETunaSweeperInteractionType::LootContainerSpawn:
		return TEXT("LootContainerSpawn");
	case ETunaSweeperInteractionType::LevelTravel:
		return TEXT("LevelTravel");
	case ETunaSweeperInteractionType::Quest:
		return TEXT("Quest");
	case ETunaSweeperInteractionType::SelfDestruct:
		return TEXT("SelfDestruct");
	default:
		return TEXT("Unknown");
	}
}
