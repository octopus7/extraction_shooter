#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TunaSweeperInteractableComponent.generated.h"

class UTunaSweeperInteractionMarkerWidget;
class UWidgetComponent;
class AActor;
class APawn;

UENUM(BlueprintType)
enum class ETunaSweeperInteractionType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	ItemPickup = 3 UMETA(DisplayName = "Item Pickup"),
	ItemSpawn = 4 UMETA(DisplayName = "Item Spawn"),
	LootContainerOpen = 5 UMETA(DisplayName = "Loot Container Open"),
	LootContainerSpawn = 6 UMETA(DisplayName = "Loot Container Spawn"),
	LevelTravel = 7 UMETA(DisplayName = "Level Travel"),
	Quest = 8 UMETA(DisplayName = "Quest"),
	SelfDestruct = 9 UMETA(DisplayName = "Self Destruct")
};

UCLASS(BlueprintType, Blueprintable, ClassGroup=(TunaSweeper), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperInteractableComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperInteractableComponent();

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	ETunaSweeperInteractionType GetInteractionType() const { return InteractionType; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	FText GetInteractionDisplayName() const { return InteractionDisplayName; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	float GetInteractionDistance() const { return InteractionDistance; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	AActor* GetInteractionOwner() const { return GetOwner(); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	FVector GetInteractionLocation() const;

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

	void SetInteractionTypeAndDisplayName(
		ETunaSweeperInteractionType InInteractionType,
		const FText& InInteractionDisplayName);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	ETunaSweeperInteractionType InteractionType = ETunaSweeperInteractionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionDisplayName = FText::FromString(TEXT("Interact"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float InteractionDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker")
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MarkerVisibleDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MarkerFadeInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MarkerScaleInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LabelFadeInterpSpeed = 10.0f;

private:
	void RegisterWithInteractionSubsystem();
	void UnregisterFromInteractionSubsystem();
	void EnsureMarkerWidgetComponent();
	void ApplyMarkerWidgetLayout();
	void EnsureMarkerWidgetClass();
	void UpdateMarker(float DeltaSeconds);
	void ApplyMarkerState();

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> MarkerWidgetComponent;

	float MarkerAlpha = 0.0f;
	float MarkerRingScale = 3.0f;
	float LabelAlpha = 0.0f;
	bool bRegisteredWithInteractionSubsystem = false;
};
