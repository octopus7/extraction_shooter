#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperInteractionSubsystem.generated.h"

class ATunaSweeperLootContainerActor;
class ATunaSweeperMemoActor;
class ATunaSweeperDoorActor;
class ATunaSweeperPersistentDoorActor;
class AActor;

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
	bool MoveFocusedInteractionSelection(int32 SelectionDelta, APawn* InstigatorPawn);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool TryInteract(APawn* InstigatorPawn);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool RequestInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);

	bool CanOfferInteraction(const UTunaSweeperInteractableComponent* Interactable) const;
	bool ShouldDisplayMarkerForInteractable(const UTunaSweeperInteractableComponent* Interactable) const;
	bool IsFocusedInteractionGroupMarker(const UTunaSweeperInteractableComponent* Interactable) const;
	void GetMarkerInteractionOptions(
		const UTunaSweeperInteractableComponent* MarkerInteractable,
		TArray<FText>& OutDisplayNames,
		int32& OutFocusedIndex) const;

private:
	void RefreshFocusedInteractable();
	void GatherCandidateInteractablesForOwner(
		const AActor* Owner,
		const APawn* PlayerPawn,
		bool bRequireInteractionDistance,
		TArray<UTunaSweeperInteractableComponent*>& OutInteractables) const;
	UTunaSweeperInteractableComponent* ResolveMarkerInteractableForOwner(const AActor* Owner) const;
	int32 FindInteractableIndex(
		const TArray<UTunaSweeperInteractableComponent*>& Interactables,
		const UTunaSweeperInteractableComponent* Interactable) const;
	bool HandlePickupItemInteraction(UTunaSweeperInteractableComponent* Interactable);
	bool HandleItemSpawnInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleLootContainerOpenInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleLootContainerSpawnInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleLevelTravelInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleQuestInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleMoleDialogueInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleSelfDestructInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleWorldProgressInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandlePersistentDoorInteraction(UTunaSweeperInteractableComponent* Interactable);
	bool HandleDoorOpenInteraction(UTunaSweeperInteractableComponent* Interactable);
	bool HandleWarpPointInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleMemoInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleHousingManagementInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleStorageOpenInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleShopOpenInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleWorkbenchOpenInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleWorkbenchCraftInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleWorkbenchDismantleInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleWorkbenchBlueprintRegisterInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandlePiggyBankInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandlePiggyBankDepositInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandlePiggyBankWithdrawInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);
	bool HandleDifficultyAdjustmentInteraction(UTunaSweeperInteractableComponent* Interactable, APawn* InstigatorPawn);

	TSet<TWeakObjectPtr<UTunaSweeperInteractableComponent>> RegisteredInteractables;
	TWeakObjectPtr<UTunaSweeperInteractableComponent> FocusedInteractable;
	TWeakObjectPtr<AActor> FocusedInteractionOwner;
	int32 FocusedInteractionIndex = 0;
};
