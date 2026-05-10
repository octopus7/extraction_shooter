#include "Subsystem/TunaSweeperInteractionSubsystem.h"

#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "Kismet/GameplayStatics.h"
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

void UTunaSweeperInteractionSubsystem::RegisterInteractable(ATunaSweeperInteractableActor* Interactable)
{
	if (IsValid(Interactable))
	{
		RegisteredInteractables.Add(Interactable);
	}
}

void UTunaSweeperInteractionSubsystem::UnregisterInteractable(ATunaSweeperInteractableActor* Interactable)
{
	RegisteredInteractables.Remove(Interactable);

	if (FocusedInteractable.Get() == Interactable)
	{
		FocusedInteractable.Reset();
	}
}

ATunaSweeperInteractableActor* UTunaSweeperInteractionSubsystem::GetFocusedInteractable() const
{
	return FocusedInteractable.Get();
}

bool UTunaSweeperInteractionSubsystem::TryInteract(APawn* InstigatorPawn)
{
	RefreshFocusedInteractable();
	return RequestInteraction(FocusedInteractable.Get(), InstigatorPawn);
}

bool UTunaSweeperInteractionSubsystem::RequestInteraction(ATunaSweeperInteractableActor* Interactable, APawn* InstigatorPawn)
{
	if (!IsValid(Interactable) || !IsValid(InstigatorPawn) || !Interactable->IsWithinInteractionDistance(InstigatorPawn))
	{
		return false;
	}

	const FString DebugTypeName = GetInteractionDebugTypeName(Interactable);
	const FString DisplayName = Interactable->GetInteractionDisplayName().ToString();
	const FString DebugMessage = FString::Printf(TEXT("[Interaction] %s: %s"), *DebugTypeName, *DisplayName);

	if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::Open)
	{
		return OpenTempOpenLootWidget(InstigatorPawn);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White, DebugMessage);
	}

	return true;
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

	ATunaSweeperInteractableActor* ClosestInteractable = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (auto InteractableIt = RegisteredInteractables.CreateIterator(); InteractableIt; ++InteractableIt)
	{
		ATunaSweeperInteractableActor* Interactable = InteractableIt->Get();
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

FString UTunaSweeperInteractionSubsystem::GetInteractionDebugTypeName(const ATunaSweeperInteractableActor* Interactable) const
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
	default:
		return TEXT("Unknown");
	}
}
