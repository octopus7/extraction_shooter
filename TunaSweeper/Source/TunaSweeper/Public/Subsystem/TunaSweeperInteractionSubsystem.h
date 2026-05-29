#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperInteractionSubsystem.generated.h"

class ATunaSweeperLootContainerActor;
class ATunaSweeperMemoActor;
class ATunaSweeperDoorActor;
class ATunaSweeperPersistentDoorActor;

UCLASS()
class TUNASWEEPER_API UTunaSweeperInteractionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	void RegisterInteractable(UTunaSweeperInteractableComponent* Interactable);
	void UnregisterInteractable(UTunaSweeperInteractableComponent* Interactable);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetFocusedInteractable() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool TryInteract(APawn* InstigatorPawn);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool RequestInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);

	bool CanOfferInteraction(const UTunaSweeperInteractableComponent* Interactable) const;

private:
	void RefreshFocusedInteractable();
	bool HandlePickupItemInteraction(UTunaSweeperInteractableComponent* Interactable);
	bool HandleItemSpawnInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleLootContainerOpenInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleLootContainerSpawnInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleLevelTravelInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleQuestInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleSelfDestructInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleWorldProgressInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandlePersistentDoorInteraction(UTunaSweeperInteractableComponent* Interactable);
	bool HandleDoorOpenInteraction(UTunaSweeperInteractableComponent* Interactable);
	bool HandleWarpPointInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleMemoInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleHousingManagementInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleStorageOpenInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);

	TSet<TWeakObjectPtr<UTunaSweeperInteractableComponent>> RegisteredInteractables;
	TWeakObjectPtr<UTunaSweeperInteractableComponent> FocusedInteractable;
};
