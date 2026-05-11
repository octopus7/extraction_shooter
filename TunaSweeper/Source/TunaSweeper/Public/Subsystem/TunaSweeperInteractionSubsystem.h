#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperInteractionSubsystem.generated.h"

class UTunaSweeperTempOpenLootWidget;

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

private:
	void RefreshFocusedInteractable();
	FString GetInteractionDebugTypeName(const UTunaSweeperInteractableComponent* Interactable) const;
	bool OpenTempOpenLootWidget(APawn* InstigatorPawn);

	TSet<TWeakObjectPtr<UTunaSweeperInteractableComponent>> RegisteredInteractables;
	TWeakObjectPtr<UTunaSweeperInteractableComponent> FocusedInteractable;
	TWeakObjectPtr<UTunaSweeperTempOpenLootWidget> ActiveTempOpenLootWidget;
};
