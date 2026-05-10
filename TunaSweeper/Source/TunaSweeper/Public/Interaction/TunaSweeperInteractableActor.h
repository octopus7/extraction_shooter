#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperInteractableActor.generated.h"

class UTunaSweeperInteractionMarkerWidget;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class ETunaSweeperInteractionType : uint8
{
	Dialogue UMETA(DisplayName = "Dialogue"),
	Pickup UMETA(DisplayName = "Pickup"),
	Open UMETA(DisplayName = "Open")
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperInteractableActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	ETunaSweeperInteractionType GetInteractionType() const { return InteractionType; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	FText GetInteractionDisplayName() const { return InteractionDisplayName; }

	void ConfigureInteractionDefaults(
		ETunaSweeperInteractionType InInteractionType,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> MarkerWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	ETunaSweeperInteractionType InteractionType = ETunaSweeperInteractionType::Dialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionDisplayName = FText::FromString(TEXT("Interact"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker")
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MarkerVisibleDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MarkerFadeInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MarkerScaleInterpSpeed = 10.0f;

private:
	void EnsureMarkerWidgetClass();
	void UpdateMarker(float DeltaSeconds);
	void ApplyMarkerState();

	float MarkerAlpha = 0.0f;
	float MarkerRingScale = 3.0f;
};
