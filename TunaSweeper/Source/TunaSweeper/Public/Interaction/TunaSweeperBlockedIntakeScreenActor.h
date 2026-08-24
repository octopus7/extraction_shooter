#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "TunaSweeperBlockedIntakeScreenActor.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
class UTunaSweeperGameInstance;
class UTunaSweeperInteractableComponent;

/**
 * Level-placed world-progress actor for an intake screen that swaps its mesh in place.
 * The completed state is stored in the regular WorldProgressStates save collection.
 */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperBlockedIntakeScreenActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperBlockedIntakeScreenActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool IsScreenCleared() const { return bScreenCleared; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool CanClearScreen() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	bool ClearScreen(bool bSaveImmediately = true);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	/** Stable and unique save key. Do not change it after shipping a level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FName ProgressObjectId = TEXT("demo.water_intake.blocked_screen");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FName ProgressInfoId = TEXT("world_progress.blocked_intake_screen");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	TSoftObjectPtr<UStaticMesh> BlockedScreenMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	TSoftObjectPtr<UStaticMesh> ClearedScreenMesh;

	/** Editor-only placement preview. Runtime always follows the saved state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	bool bPreviewClearedStateInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionDisplayName = FText::FromString(TEXT("이물질 제거"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName InteractionDisplayNameStringKey = TEXT("ui.interaction.clear_debris");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName ObjectiveEventId = TEXT("demo.water_intake.screen_clear");

	/** Optional tool/item ownership requirement. Disabled until the crowbar item is defined. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Requirement")
	bool bRequiresItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Requirement", meta = (EditCondition = "bRequiresItem"))
	int32 RequiredItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Requirement", meta = (ClampMin = "1", UIMin = "1", EditCondition = "bRequiresItem"))
	int32 RequiredItemQuantity = 1;

	/** Leave false for reusable tools such as a crowbar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Requirement", meta = (EditCondition = "bRequiresItem"))
	bool bConsumeRequiredItem = false;

private:
	void ApplySavedState();
	void ApplyVisualState();
	void RefreshPresentation();
	FText ResolveInteractionDisplayName() const;
	UTexture2D* LoadRequiredItemIconTexture() const;
	FName GetEffectiveProgressObjectId() const;
	FTunaSweeperWorldProgressSaveData GetOrCreateProgressState() const;
	UTunaSweeperGameInstance* GetTunaGameInstance() const;

	UPROPERTY(Transient)
	bool bScreenCleared = false;
};
