#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperInteractionSubsystem.generated.h"

class ATunaSweeperInteractableActor;

UCLASS()
class TUNASWEEPER_API UTunaSweeperInteractionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	void RegisterInteractable(ATunaSweeperInteractableActor* Interactable);
	void UnregisterInteractable(ATunaSweeperInteractableActor* Interactable);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	ATunaSweeperInteractableActor* GetFocusedInteractable() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool TryInteract(APawn* InstigatorPawn);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool RequestInteraction(ATunaSweeperInteractableActor* Interactable, APawn* InstigatorPawn);

private:
	void RefreshFocusedInteractable();
	FString GetInteractionDebugTypeName(const ATunaSweeperInteractableActor* Interactable) const;

	TSet<TWeakObjectPtr<ATunaSweeperInteractableActor>> RegisteredInteractables;
	TWeakObjectPtr<ATunaSweeperInteractableActor> FocusedInteractable;
};
