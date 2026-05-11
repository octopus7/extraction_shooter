#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "TunaSweeperInteractableActor.generated.h"

class UTunaSweeperInteractionMarkerWidget;
class UTunaSweeperInteractableComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperInteractableActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	ETunaSweeperInteractionType GetInteractionType() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	FText GetInteractionDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	float GetInteractionDistance() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	bool IsWithinInteractionDistance(const AActor* OtherActor) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	float GetSquaredDistance2DTo(const AActor* OtherActor) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool RequestInteraction(APawn* InstigatorPawn);

	void ConfigureInteractionDefaults(
		ETunaSweeperInteractionType InInteractionType,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;
};
