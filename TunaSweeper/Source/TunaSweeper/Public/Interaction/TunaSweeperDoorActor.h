#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperDoorActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperDoorActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperDoorActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Door")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Door")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Door")
	bool ToggleDoor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Door")
	void SetDoorOpen(bool bInOpen, bool bInstant = false);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> HingePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> DoorBodyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bStartsOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FText InteractionDisplayName = FText::FromString(TEXT("\uC0C1\uD638\uC791\uC6A9"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FRotator ClosedHingeRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FRotator OpenHingeRelativeRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RotationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float RotationSnapToleranceDegrees = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bEnableDoorBodyCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Body")
	FVector DoorBodyExtent = FVector(50.0f, 6.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Body")
	FVector DoorBodyRelativeLocation = FVector(50.0f, 0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Mesh")
	FVector DoorMeshRelativeLocation = FVector(50.0f, 0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Mesh")
	FVector DoorMeshRelativeScale = FVector(1.0f, 0.12f, 2.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Interaction")
	FVector InteractableRelativeLocation = FVector(50.0f, 0.0f, 130.0f);

private:
	void ApplyComponentDefaults();
	void ApplyDoorState(bool bInstant);
	FRotator GetTargetHingeRotation() const;

	UPROPERTY(Transient)
	bool bOpen = false;
};
