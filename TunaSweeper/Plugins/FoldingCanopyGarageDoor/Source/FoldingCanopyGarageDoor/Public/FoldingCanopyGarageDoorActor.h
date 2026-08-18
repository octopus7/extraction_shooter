#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoldingCanopyGarageDoorActor.generated.h"

class APawn;
class UBoxComponent;
class UMaterialInterface;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EFoldingCanopyGarageDoorState : uint8
{
	Closed UMETA(DisplayName = "Closed"),
	Opening UMETA(DisplayName = "Opening"),
	Open UMETA(DisplayName = "Open"),
	Closing UMETA(DisplayName = "Closing")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFoldingCanopyGarageDoorEvent);

/**
 * A reusable mechanical garage door. It intentionally has no dependency on a project's
 * interaction or travel framework: proximity opening is a visual-only presentation feature.
 */
UCLASS(Blueprintable, BlueprintType)
class FOLDINGCANOPYGARAGEDOOR_API AFoldingCanopyGarageDoor : public AActor
{
	GENERATED_BODY()

public:
	AFoldingCanopyGarageDoor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Garage Door")
	void OpenDoor();

	UFUNCTION(BlueprintCallable, Category = "Garage Door")
	void CloseDoor();

	UFUNCTION(BlueprintCallable, Category = "Garage Door")
	void ToggleDoor();

	UFUNCTION(BlueprintCallable, Category = "Garage Door")
	void SetOpenAlpha(float NewOpenAlpha);

	UFUNCTION(BlueprintPure, Category = "Garage Door")
	float GetOpenAlpha() const { return OpenAlpha; }

	UFUNCTION(BlueprintPure, Category = "Garage Door")
	EFoldingCanopyGarageDoorState GetDoorState() const { return DoorState; }

	UFUNCTION(BlueprintPure, Category = "Garage Door")
	bool IsFullyOpen() const;

	UFUNCTION(BlueprintPure, Category = "Garage Door")
	bool IsFullyClosed() const;

	UFUNCTION(BlueprintCallable, Category = "Garage Door")
	void SetDoorEnabled(bool bNewDoorEnabled);

	/** Applies a single project art kit to all visual parts. Individual panel slots remain editable in a Blueprint child. */
	void ConfigureVisualDefaults(
		UStaticMesh* InFrameTopMesh,
		UStaticMesh* InFrameLeftMesh,
		UStaticMesh* InFrameRightMesh,
		UStaticMesh* InCanopyRailLeftMesh,
		UStaticMesh* InCanopyRailRightMesh,
		UStaticMesh* InTemporaryWallLeftMesh,
		UStaticMesh* InTemporaryWallRightMesh,
		UStaticMesh* InTemporaryRoofMesh,
		UStaticMesh* InUpperPanelMesh,
		UStaticMesh* InLowerPanelMesh,
		UStaticMesh* InLedBarMesh,
		UMaterialInterface* InMetalMaterial,
		UMaterialInterface* InLedMaterial);

	UPROPERTY(BlueprintAssignable, Category = "Garage Door|Events")
	FFoldingCanopyGarageDoorEvent OnDoorOpeningStarted;

	UPROPERTY(BlueprintAssignable, Category = "Garage Door|Events")
	FFoldingCanopyGarageDoorEvent OnDoorFullyOpened;

	UPROPERTY(BlueprintAssignable, Category = "Garage Door|Events")
	FFoldingCanopyGarageDoorEvent OnDoorClosingStarted;

	UPROPERTY(BlueprintAssignable, Category = "Garage Door|Events")
	FFoldingCanopyGarageDoorEvent OnDoorFullyClosed;

protected:
	UFUNCTION()
	void HandleAutoOpenTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleAutoOpenTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> FrameTopComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> FrameLeftComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> FrameRightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> LedBarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> CanopyRailLeftComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> CanopyRailRightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> TemporaryWallLeftComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> TemporaryWallRightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> TemporaryRoofComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<USceneComponent> DoorCarrier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<USceneComponent> Hinge01;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<USceneComponent> Hinge02;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<USceneComponent> Hinge03;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<USceneComponent> Hinge04;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> UpperPanel01;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> UpperPanel02;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> UpperPanel03;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> UpperPanel04;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<USceneComponent> LowerPanelRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UStaticMeshComponent> LowerPanelComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UBoxComponent> AutoOpenTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garage Door|Components")
	TObjectPtr<UBoxComponent> PassageBlocker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> FrameTopMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> FrameLeftMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> FrameRightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> CanopyRailLeftMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> CanopyRailRightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> TemporaryWallLeftMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> TemporaryWallRightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> TemporaryRoofMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> LedBarMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes", meta = (EditFixedSize))
	TArray<TObjectPtr<UStaticMesh>> UpperPanelMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Meshes")
	TObjectPtr<UStaticMesh> LowerPanelMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Materials")
	TObjectPtr<UMaterialInterface> MetalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Materials")
	TObjectPtr<UMaterialInterface> LedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float DoorWidth = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float DoorThickness = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float SharedUpperPanelHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (EditFixedSize))
	TArray<bool> bOverrideUpperPanelHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (EditFixedSize, ClampMin = "1.0"))
	TArray<float> UpperPanelHeightOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float LowerPanelHeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "0.0"))
	float LowerPanelEmbedDepth = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "0.0"))
	float InterlockClearance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float FrameSideWidth = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float FrameTopHeight = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float FrameDepth = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float LedBarWidth = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float LedBarHeight = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry", meta = (ClampMin = "1.0"))
	float LedBarThickness = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Geometry")
	bool bAutoFitMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Motion", meta = (ClampMin = "0.01"))
	float OpenDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Motion", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float GroundDropEndAlpha = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Motion")
	float CarrierFinalRollDegrees = 90.0f;

	/** Fraction of one panel exposed beyond the preceding panel in the open rail stack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Motion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CanopyPanelRevealRatio = 0.15f;

	/** Vertical spacing between panel layers after they stack on the exterior rail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Motion", meta = (ClampMin = "0.0"))
	float CanopyPanelStackVerticalStep = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Proximity")
	bool bAutoOpenOnPlayerProximity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Proximity")
	bool bPlayerControlledPawnsOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Proximity", meta = (ClampMin = "0.0"))
	float AutoOpenDepth = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Proximity", meta = (ClampMin = "0.0"))
	float AutoOpenSidePadding = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Proximity", meta = (ClampMin = "0.0"))
	float AutoOpenHeightPadding = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Proximity", meta = (ClampMin = "0.0"))
	float AutoCloseDelaySeconds = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Collision", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PassageBlockerOpenAlpha = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|State")
	bool bStartsOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|State")
	bool bDoorEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PreviewOpenAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Preview")
	bool bPreviewInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Preview")
	bool bDrawDebugHinges = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garage Door|Preview")
	bool bDrawDebugInterlock = false;

private:
	void EnforceIndependentPanelAttachments();
	void NormalizeFixedSizeArrays();
	void ApplyMeshes();
	void ApplyClosedLayout();
	void ApplyDoorPose(float PoseAlpha);
	void ApplyCanopyRailPose(float TopPanelRollDegrees);
	void ApplyMeshDimensions(UStaticMeshComponent* Component, const FVector& TargetDimensions) const;
	void UpdateCollisionState(float PoseAlpha);
	void DrawDebugLayout() const;
	float GetEffectiveUpperPanelHeight(int32 PanelIndex) const;
	float GetUpperTotalHeight() const;
	float GetCanopyPanelRevealOffset(int32 PanelIndex) const;
	float GetCanopyRailLength() const;
	float GetCanopyRailHeight() const;
	bool IsEligibleAutoOpenPawn(const AActor* Actor) const;
	bool HasAutoOpenPawns();
	void HandleDelayedAutoClose();
	void SetDoorState(EFoldingCanopyGarageDoorState NewState);

	float OpenAlpha = 0.0f;
	EFoldingCanopyGarageDoorState DoorState = EFoldingCanopyGarageDoorState::Closed;
	TSet<TWeakObjectPtr<APawn>> AutoOpenPawns;
	FTimerHandle AutoCloseTimerHandle;
};
