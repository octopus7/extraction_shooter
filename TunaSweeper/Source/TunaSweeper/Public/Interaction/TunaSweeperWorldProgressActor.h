#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "TunaSweeperWorldProgressActor.generated.h"

class ATunaSweeperWorldProgressCompletedActor;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTunaSweeperGameInstance;
class UTunaSweeperInteractableComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWorldProgressActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperWorldProgressActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	void ConfigureWorldProgressDefaults(
		FName InProgressObjectId,
		FName InProgressInfoId,
		const FText& InDisplayName,
		const FText& InInteractionDisplayName,
		int32 InRequiredItemId,
		int32 InRequiredQuantity,
		int32 InInitialProgressQuantity,
		const FText& InRequiredItemDisplayName,
		const FVector& InBlockingBoxExtent,
		TSoftClassPtr<AActor> InCompletedReplacementActorClass);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	FText GetRequiredItemDisplayName() const { return RequiredItemDisplayName; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	int32 GetRequiredItemId() const { return RequiredItemId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	int32 GetRequiredQuantity() const { return RequiredQuantity; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	int32 GetProgressQuantity() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	int32 GetOwnedRequiredItemCount() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool IsRepairReady() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool IsCompleted() const { return bCompleted; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	int32 UseAvailableRequiredItems(bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	bool Repair(bool bSaveImmediately = true);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FName ProgressObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FName ProgressInfoId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FText DisplayName = FText::FromString(TEXT("\uBD80\uC11C\uC9C4 \uB2E4\uB9AC"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FText InteractionDisplayName = FText::FromString(TEXT("\uC218\uB9AC\uD558\uAE30"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	int32 RequiredItemId = 6002;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	int32 RequiredQuantity = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	int32 InitialProgressQuantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FText RequiredItemDisplayName = FText::FromString(TEXT("\uBAA9\uC7AC"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FVector BlockingBoxExtent = FVector(260.0f, 55.0f, 140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	TSoftClassPtr<AActor> CompletedReplacementActorClass;

private:
	void ApplyBridgeVisualMesh();
	void ApplyCollisionDefaults();
	void RefreshPresentation();
	void ApplySavedState();
	void CompleteAndReplace(bool bSaveImmediately);
	FName GetEffectiveProgressObjectId() const;
	FTunaSweeperWorldProgressSaveData GetOrCreateProgressState() const;
	UTunaSweeperGameInstance* GetTunaGameInstance() const;

	UPROPERTY(Transient)
	bool bCompleted = false;
};
