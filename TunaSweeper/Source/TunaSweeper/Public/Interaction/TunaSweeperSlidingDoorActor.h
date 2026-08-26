#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperSlidingDoorActor.generated.h"

class APawn;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ETunaSweeperSlidingDoorState : uint8
{
	Closed UMETA(DisplayName = "Closed"),
	Opening UMETA(DisplayName = "Opening"),
	Open UMETA(DisplayName = "Open"),
	Closing UMETA(DisplayName = "Closing")
};

/**
 * Two-panel sci-fi sliding door that opens automatically for nearby players.
 * The actor faces local -Y; both panels slide outward along local X.
 */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperSlidingDoorActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSlidingDoorActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Sliding Door")
	void OpenDoor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Sliding Door")
	void CloseDoor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Sliding Door")
	void ToggleDoor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Sliding Door")
	void SetDoorOpen(bool bInOpen, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Sliding Door")
	float GetOpenAlpha() const { return OpenAlpha; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Sliding Door")
	ETunaSweeperSlidingDoorState GetDoorState() const { return DoorState; }

protected:
	UFUNCTION()
	void HandleProximityBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleProximityEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<USceneComponent> LeftPanelRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<USceneComponent> RightPanelRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<UStaticMeshComponent> LeftDoorMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<UStaticMeshComponent> RightDoorMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<UStaticMeshComponent> DoorFrameMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<UBoxComponent> LeftPanelCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<UBoxComponent> RightPanelCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sliding Door|Components")
	TObjectPtr<UBoxComponent> ProximityTrigger;

	/** Meshes can be replaced independently on a Blueprint child. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Meshes")
	TObjectPtr<UStaticMesh> LeftDoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Meshes")
	TObjectPtr<UStaticMesh> RightDoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Meshes")
	TObjectPtr<UStaticMesh> DoorFrameMesh;

	/** Fits replacement meshes to the configured doorway while keeping their geometry floor-aligned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Meshes")
	bool bAutoFitPanelMeshes = true;

	/** Total clear width of the closed doorway, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Geometry", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float DoorWidth = 132.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Geometry", meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
	float DoorHeight = 195.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Geometry", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float DoorThickness = 15.0f;

	/** Distance each panel travels outward from its closed location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Motion", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float PanelTravelDistance = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Motion", meta = (ClampMin = "0.01", UIMin = "0.01", Units = "s"))
	float OpenDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Motion", meta = (ClampMin = "0.01", UIMin = "0.01", Units = "s"))
	float CloseDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Motion")
	bool bUseSmoothStepMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Proximity")
	bool bAutoOpenOnPlayerProximity = true;

	/** Detection distance from the door plane on both sides. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Proximity", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float PlayerDetectionDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Proximity", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float DetectionSidePadding = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Proximity", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float DetectionHeightPadding = 40.0f;

	/** Time after the last player leaves before the door begins closing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Proximity", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float AutoCloseDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|Collision")
	bool bEnablePanelCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sliding Door|State")
	bool bStartsOpen = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Sliding Door|State")
	float OpenAlpha = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Sliding Door|State")
	ETunaSweeperSlidingDoorState DoorState = ETunaSweeperSlidingDoorState::Closed;

private:
	void ApplyConfiguration();
	void ApplyDoorPose();
	void ApplyMeshDimensions(UStaticMeshComponent* MeshComponent, const FVector& TargetDimensions) const;
	void ApplyFrameMeshScale(const FVector& TargetDoorDimensions) const;
	bool IsEligiblePlayer(const AActor* Actor) const;
	bool HasNearbyPlayers();
	void HandleDelayedAutoClose();
	void AddInitiallyOverlappingPlayers();

	TSet<TWeakObjectPtr<APawn>> NearbyPlayers;
	FTimerHandle AutoCloseTimerHandle;
};
