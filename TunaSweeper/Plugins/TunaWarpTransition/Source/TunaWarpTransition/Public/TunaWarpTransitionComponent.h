#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TunaWarpTransitionComponent.generated.h"

class AActor;
class UCameraComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWarpTransitionStartedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWarpTransitionMidpointEvent, bool, bTeleportSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWarpTransitionFinishedEvent, bool, bTeleportSucceeded);

/** Designer-facing timing, distortion, rim, and optional physical-light settings. */
USTRUCT(BlueprintType)
struct TUNAWARPTRANSITION_API FWarpTransitionStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.01", UIMin = "0.01", UIMax = "1.0"))
	float CloseDuration = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.01", UIMin = "0.01", UIMax = "1.5"))
	float OpenDuration = 0.34f;

	/** Screen-space center in ViewportUV coordinates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Distortion")
	FVector2D ScreenCenter = FVector2D(0.5, 0.5);

	/** Radius measured relative to viewport height after aspect correction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Distortion", meta = (ClampMin = "0.05", ClampMax = "2.0", UIMin = "0.05", UIMax = "1.0"))
	float OpenRadius = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Distortion", meta = (ClampMin = "0.005", ClampMax = "1.0", UIMin = "0.01", UIMax = "0.25"))
	float CollapsedRadius = 0.055f;

	/** Smaller values produce stronger inward pinch and outward stretching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Distortion", meta = (ClampMin = "0.08", ClampMax = "1.0", UIMin = "0.08", UIMax = "1.0"))
	float MinimumRadialScale = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Distortion", meta = (ClampMin = "0.0", ClampMax = "0.25", UIMin = "0.0", UIMax = "0.12"))
	float StreakLength = 0.055f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Distortion")
	FLinearColor CoverColor = FLinearColor(0.008f, 0.025f, 0.045f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim")
	FLinearColor RimColor = FLinearColor(0.24f, 0.88f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0"))
	float RimIntensity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "8.0"))
	float RimPower = 2.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "4.0"))
	float EdgeStrength = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "8.0"))
	float NormalEdgeScale = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0"))
	float DepthEdgeScale = 320.0f;

	/** Persistent fraction of the rim while the brighter world-space wave crosses the scene. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float GlobalRimFraction = 0.34f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10000.0"))
	float WaveEndRadiusCm = 2800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "2000.0"))
	float WaveHalfWidthCm = 430.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Rim", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "2000.0"))
	float WaveSoftnessCm = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Light")
	bool bEnableArrivalPointLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Light", meta = (EditCondition = "bEnableArrivalPointLight", ClampMin = "0.0", UIMin = "0.0", UIMax = "200000.0"))
	float PointLightIntensityLumens = 70000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Light", meta = (EditCondition = "bEnableArrivalPointLight", ClampMin = "1.0", UIMin = "1.0", UIMax = "5000.0"))
	float PointLightRadiusCm = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Light", meta = (EditCondition = "bEnableArrivalPointLight"))
	FLinearColor PointLightColor = FLinearColor(0.18f, 0.72f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrival Light", meta = (EditCondition = "bEnableArrivalPointLight"))
	FVector PointLightRelativeLocation = FVector(0.0f, 0.0f, 120.0f);
};

/**
 * Reusable player-camera warp effect. Add it to a pawn Blueprint for designer overrides, or let
 * gameplay create it on demand. The actual teleport occurs only after the closing image is covered.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = Rendering, meta = (BlueprintSpawnableComponent, DisplayName = "Tuna Warp Transition"))
class TUNAWARPTRANSITION_API UTunaWarpTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaWarpTransitionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Starts a complete close / teleport / arrival-open sequence. */
	UFUNCTION(BlueprintCallable, Category = "Tuna Warp Transition")
	bool PlayWarpTransition(AActor* ActorToTeleport, const FTransform& TargetTransform, bool bUpdatePawnControlRotation = true);

	/** C++ form used by gameplay that needs a reliable midpoint result callback. */
	bool PlayWarpTransitionNative(
		AActor* ActorToTeleport,
		const FTransform& TargetTransform,
		bool bUpdatePawnControlRotation,
		TFunction<void(bool)> MidpointCallback);

	UFUNCTION(BlueprintPure, Category = "Tuna Warp Transition")
	bool IsTransitionActive() const { return bTransitionActive; }

	/** Immediately restores the camera and input state without teleporting. */
	UFUNCTION(BlueprintCallable, Category = "Tuna Warp Transition")
	void CancelWarpTransition();

	UPROPERTY(BlueprintAssignable, Category = "Tuna Warp Transition")
	FWarpTransitionStartedEvent OnTransitionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Tuna Warp Transition")
	FWarpTransitionMidpointEvent OnTransitionMidpoint;

	UPROPERTY(BlueprintAssignable, Category = "Tuna Warp Transition")
	FWarpTransitionFinishedEvent OnTransitionFinished;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuna Warp Transition")
	FWarpTransitionStyle Style;

	/** Public material instances may be duplicated or replaced without editing plugin code. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuna Warp Transition|Materials")
	TSoftObjectPtr<UMaterialInterface> WarpMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuna Warp Transition|Materials")
	TSoftObjectPtr<UMaterialInterface> ArrivalRimMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuna Warp Transition|Input")
	bool bLockMovementInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuna Warp Transition|Input")
	bool bLockLookInput = false;

private:
	enum class ETransitionPhase : uint8
	{
		Idle,
		Closing,
		Opening
	};

	bool InitializeEffectMaterials();
	UCameraComponent* FindTargetCamera() const;
	void AttachBlendables();
	void DetachBlendables();
	void UpdateClosing(float Alpha);
	void UpdateOpening(float Alpha);
	void ExecuteMidpointTeleport();
	bool ExecuteImmediateTeleport(AActor* ActorToTeleport, const FTransform& TargetTransform, bool bUpdatePawnControlRotation) const;
	void FinishTransition();
	void ResetMaterialParameters();
	void ApplyInputLock();
	void ReleaseInputLock();
	void EnsureArrivalPointLight();
	void SetArrivalPointLight(float NormalizedIntensity);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WarpMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ArrivalRimMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> TargetCamera;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> ArrivalPointLight;

	TWeakObjectPtr<AActor> PendingTeleportActor;
	FTransform PendingTargetTransform = FTransform::Identity;
	TFunction<void(bool)> PendingMidpointCallback;
	ETransitionPhase Phase = ETransitionPhase::Idle;
	float PhaseElapsed = 0.0f;
	bool bTransitionActive = false;
	bool bTeleportSucceeded = false;
	bool bPendingControlRotationUpdate = false;
	bool bAppliedMoveInputLock = false;
	bool bAppliedLookInputLock = false;
};
