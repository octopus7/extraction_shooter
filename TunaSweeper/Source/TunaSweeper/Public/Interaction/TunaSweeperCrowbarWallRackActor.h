#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperCrowbarWallRackActor.generated.h"

class APawn;
class USceneComponent;
class UStaticMeshComponent;
class UTunaSweeperGameInstance;
class UTunaSweeperInteractableComponent;

/**
 * Demo-only bunker supply that offers a crowbar while the player does not own one
 * and the water-intake debris has not been cleared. The actor itself is not persisted.
 */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperCrowbarWallRackActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperCrowbarWallRackActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Appearance")
	UStaticMeshComponent* GetPedestalMeshComponent() const { return PedestalMesh; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Appearance")
	UStaticMeshComponent* GetCrowbarMeshComponent() const { return CrowbarMesh; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Crowbar Supply")
	bool IsCrowbarAvailable() const { return bCrowbarAvailable; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Crowbar Supply")
	int32 GetCrowbarItemId() const { return CrowbarItemId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Crowbar Supply")
	FName GetWaterIntakeProgressObjectId() const { return WaterIntakeProgressObjectId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Crowbar Supply")
	bool SupplyCrowbar(APawn* InstigatorPawn);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Wall mount or pedestal visual. Assign its mesh later in the Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PedestalMesh;

	/** Crowbar pickup visual. Assign its mesh later in the Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CrowbarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowbar Supply", meta = (ClampMin = "1"))
	int32 CrowbarItemId = 6003;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowbar Supply")
	FName WaterIntakeProgressObjectId = TEXT("demo.water_intake.blocked_screen");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowbar Supply")
	FText InteractionDisplayName = FText::FromString(TEXT("크로우바 획득"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowbar Supply")
	FName InteractionDisplayNameStringKey = TEXT("ui.interaction.take_crowbar");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crowbar Supply")
	FVector InteractionLocation = FVector(0.0f, 0.0f, 100.0f);

private:
	void RefreshAvailability();
	bool DoesPlayerOwnCrowbar() const;
	bool IsWaterIntakeDebrisCleared() const;
	UTunaSweeperGameInstance* GetTunaGameInstance() const;

	UPROPERTY(Transient)
	bool bCrowbarAvailable = false;
};
