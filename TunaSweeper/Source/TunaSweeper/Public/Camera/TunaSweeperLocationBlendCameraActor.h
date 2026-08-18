#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperLocationBlendCameraActor.generated.h"

class ACameraActor;
class APlayerController;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * A placeable location camera that blends from the controlled pawn's live camera
 * according to the pawn's distance from BlendOrigin.
 *
 * The actor owns the complete transition. It does not require the player pawn or
 * its camera components to be moved, and it yields to unrelated view targets such
 * as dialogue and cinematic cameras.
 */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLocationBlendCameraActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLocationBlendCameraActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual bool IsDefaultPreviewEnabled() const override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Location Camera")
	ACameraActor* GetTargetCameraActor() const { return TargetCameraActor; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Location Camera")
	void SetTargetCameraActor(ACameraActor* InTargetCameraActor);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Location Camera")
	USceneComponent* GetBlendOriginComponent() const { return BlendOrigin; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Location Camera")
	float GetBlendWeightAtLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Location Camera")
	float GetCurrentBlendWeight() const { return CurrentBlendWeight; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Location Camera")
	void SetBlendEnabled(bool bEnabled);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> CameraRigRoot;

	/** Independent world-space center used for player distance measurement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BlendOrigin;

	/**
	 * Independently placed level camera used as the destination POV. Pilot this
	 * CameraActor to adjust framing without moving this actor's distance zone.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<ACameraActor> TargetCameraActor;

	/** Outer radius at which blending begins. Must be larger than Blend Complete Distance. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Distance Blend", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float BlendStartDistance = 1600.0f;

	/** Inner radius at which the location camera has full weight. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Distance Blend", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float BlendCompleteDistance = 600.0f;

	/** Ignores height differences when measuring player distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Blend")
	bool bUse2DDistance = true;

	/** Applies a smoothstep curve to the normalized distance weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Blend")
	bool bUseSmoothStep = true;

	/** Higher priority wins when multiple location camera ranges overlap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activation")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activation")
	bool bBlendEnabled = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> BlendStartPreview;

	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> BlendCompletePreview;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> EditorSelectionHandle;
#endif

private:
	static ATunaSweeperLocationBlendCameraActor* FindPreferredCamera(
		const UWorld* World,
		const FVector& PlayerLocation);

	bool CanTakeViewTarget(const APlayerController& PlayerController) const;
	float GetDistanceToBlendOrigin(const FVector& WorldLocation) const;
	void RestorePawnViewTarget() const;

#if WITH_EDITOR
	void RefreshEditorVisualization();
	void UpdateEditorSelectionVisualization();
#endif

	UPROPERTY(Transient)
	float CurrentBlendWeight = 0.0f;
};
