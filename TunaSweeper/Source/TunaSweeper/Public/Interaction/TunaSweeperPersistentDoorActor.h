#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "TunaSweeperPersistentDoorActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTunaSweeperGameInstance;
class UTunaSweeperInteractableComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPersistentDoorActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperPersistentDoorActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Persistent Door")
	void ConfigurePersistentDoorDefaults(
		FName InDoorObjectId,
		FName InDoorInfoId,
		const FText& InDisplayName,
		const FText& InInteractionDisplayName,
		const FVector& InBlockingBoxExtent,
		const FVector& InDoorMeshRelativeLocation,
		const FVector& InDoorMeshRelativeScale,
		const FRotator& InClosedDoorRelativeRotation,
		const FRotator& InOpenDoorRelativeRotation);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Persistent Door")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Persistent Door")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Persistent Door")
	bool OpenDoor(bool bSaveImmediately = true);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FName DoorObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FName DoorInfoId = FName(TEXT("door.persistent_open"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FText DisplayName = FText::FromString(TEXT("\uBB38"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FText InteractionDisplayName = FText::FromString(TEXT("\uC5F4\uAE30"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FVector BlockingBoxExtent = FVector(60.0f, 18.0f, 130.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FVector DoorMeshRelativeLocation = FVector(0.0f, 0.0f, 130.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FVector DoorMeshRelativeScale = FVector(1.2f, 0.12f, 2.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FRotator ClosedDoorRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FRotator OpenDoorRelativeRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistent Door")
	FVector InteractableRelativeLocation = FVector(0.0f, 0.0f, 145.0f);

private:
	void ApplyCollisionDefaults();
	void ApplyDoorState();
	void RefreshPresentation();
	void ApplySavedState();
	FName GetEffectiveDoorObjectId() const;
	FTunaSweeperWorldProgressSaveData GetOrCreateDoorState() const;
	UTunaSweeperGameInstance* GetTunaGameInstance() const;

	UPROPERTY(Transient)
	bool bOpen = false;
};
