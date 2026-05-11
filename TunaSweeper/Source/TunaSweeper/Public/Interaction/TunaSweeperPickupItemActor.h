#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperPickupItemActor.generated.h"

class UTunaSweeperInteractableComponent;
class UTunaSweeperPickupItemIconWidget;
class USceneComponent;
class UTexture2D;
class UWidgetComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPickupItemActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperPickupItemActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Pickup Item")
	void SetItemId(int32 InItemId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Pickup Item")
	int32 GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Pickup Item")
	FText GetItemDisplayName() const { return CachedItemDisplayName; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Pickup Item")
	bool ShouldDestroyOnPickup() const { return bDestroyOnPickup; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	void ConfigurePickupItemDefaults(
		int32 InItemId,
		TSoftClassPtr<UTunaSweeperPickupItemIconWidget> InFloorIconWidgetClass);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> FloorIconWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Item")
	int32 ItemId = 1001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Item")
	ETunaSweeperItemTextLanguage DisplayLanguage = ETunaSweeperItemTextLanguage::Korean;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Item")
	bool bDestroyOnPickup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Item|Icon")
	TSoftClassPtr<UTunaSweeperPickupItemIconWidget> FloorIconWidgetClass;

private:
	void EnsureFloorIconWidgetClass();
	void RefreshItemPresentation();
	UTexture2D* LoadIconTexture(const FTunaSweeperItemDefinition& ItemDefinition) const;
	FText BuildFallbackItemName() const;

	UPROPERTY(Transient)
	FText CachedItemDisplayName;
};
