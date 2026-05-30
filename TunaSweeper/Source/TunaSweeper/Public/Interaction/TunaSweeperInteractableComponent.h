#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TunaSweeperInteractableComponent.generated.h"

class UTunaSweeperInteractionMarkerWidget;
class UWidgetComponent;
class UTexture2D;
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
	SelfDestruct = 9 UMETA(DisplayName = "Self Destruct"),
	WorldProgress = 10 UMETA(DisplayName = "World Progress"),
	PersistentDoor = 11 UMETA(DisplayName = "Persistent Door"),
	WarpPoint = 12 UMETA(DisplayName = "Warp Point"),
	Memo = 13 UMETA(DisplayName = "Memo"),
	DoorOpen = 14 UMETA(DisplayName = "Door Open"),
	HousingManagement = 15 UMETA(DisplayName = "Housing Management"),
	StorageOpen = 16 UMETA(DisplayName = "Storage Open"),
	ShopOpen = 17 UMETA(DisplayName = "Shop Open"),
	WorkbenchOpen = 18 UMETA(DisplayName = "Workbench Open"),
	WorkbenchCraft = 19 UMETA(DisplayName = "Workbench Craft"),
	WorkbenchDismantle = 20 UMETA(DisplayName = "Workbench Dismantle"),
	WorkbenchBlueprintRegister = 21 UMETA(DisplayName = "Workbench Blueprint Register"),
	PiggyBank = 22 UMETA(DisplayName = "Piggy Bank")
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
	FText GetInteractionDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	FName GetObjectiveEventId() const { return ObjectiveEventId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	int32 GetInteractionOrder() const { return InteractionOrder; }

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
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		FName InInteractionDisplayNameStringKey = NAME_None);

	void SetInteractionTypeAndDisplayName(
		ETunaSweeperInteractionType InInteractionType,
		const FText& InInteractionDisplayName);

	void SetInteractionTypeDisplayNameAndStringKey(
		ETunaSweeperInteractionType InInteractionType,
		const FText& InInteractionDisplayName,
		FName InInteractionDisplayNameStringKey);

	void SetInteractionDisplayNameStringKey(FName InInteractionDisplayNameStringKey);

	void SetInteractionOrder(int32 InInteractionOrder);

	void SetInteractionRequirementPreview(
		UTexture2D* InIconTexture,
		int32 InRequiredQuantity,
		bool bInShowRequirement);

	void SetMarkerCompleted(bool bInMarkerCompleted);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	ETunaSweeperInteractionType InteractionType = ETunaSweeperInteractionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionDisplayName = FText::FromString(TEXT("Interact"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName InteractionDisplayNameStringKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName ObjectiveEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	int32 InteractionOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float InteractionDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker")
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Marker", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MarkerVisibleDistance = 400.0f;

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
	FText ResolveInteractionDisplayName() const;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> MarkerWidgetComponent;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> RequirementIconTexture;

	int32 RequirementQuantity = 0;
	bool bShowRequirementPreview = false;
	bool bMarkerCompleted = false;

	float MarkerAlpha = 0.0f;
	float MarkerRingScale = 3.0f;
	float LabelAlpha = 0.0f;
	bool bRegisteredWithInteractionSubsystem = false;
};
