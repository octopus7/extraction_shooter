// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "QuadrupedComponent.generated.h"

class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EQuadrupedFootPhase : uint8
{
	Planted,
	Swinging
};

/**
 * Data for a single leg in the quadruped system.
 */
USTRUCT(BlueprintType)
struct FQuadrupedLegData
{
	GENERATED_BODY()

	// Offset from body center in local space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leg")
	FVector DefaultOffset = FVector::ZeroVector;

	// Current foot world position
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	FVector CurrentPosition = FVector::ZeroVector;

	// Continuously evaluated preview for the next valid footfall.
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	FVector TargetPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	FVector CurrentSurfaceNormal = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	FVector TargetSurfaceNormal = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	bool bHasValidGroundTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	float PlacementScore = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	EQuadrupedFootPhase Phase = EQuadrupedFootPhase::Planted;

	// Latched positions for a stable swing trajectory.
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	FVector StepStartPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	FVector StepEndPosition = FVector::ZeroVector;

	FVector StepEndSurfaceNormal = FVector::UpVector;

	// Step interpolation progress (0 to 1)
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	float StepProgress = 0.0f;

	// Is this leg currently stepping?
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	bool bIsMoving = false;

	// Gait group (0 or 1) - diagonal pairs step together
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leg")
	int32 GaitGroup = 0;

	// Runtime-only contact bookkeeping for moving support components.
	TWeakObjectPtr<UPrimitiveComponent> SupportComponent;
	FVector SupportRelativePosition = FVector::ZeroVector;
	TWeakObjectPtr<UPrimitiveComponent> TargetSupportComponent;
	FVector TargetSupportRelativePosition = FVector::ZeroVector;
	TWeakObjectPtr<UPrimitiveComponent> StepEndSupportComponent;
	FVector StepEndSupportRelativePosition = FVector::ZeroVector;
};

/**
 * Component that handles procedural quadruped leg animation.
 * Calculates IK target positions for 4 legs using ground raycasts
 * and diagonal gait pattern.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIYAKOVCHARACTERSYSTEM_API UQuadrupedComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuadrupedComponent();

protected:
	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	// Refresh leg positions in editor (called by owner's PostEditMove)
	void RefreshLegsInEditor();
#endif

	// 0: Front Left, 1: Front Right, 2: Back Left, 3: Back Right
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Legs")
	TArray<FQuadrupedLegData> Legs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Step", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float StepThreshold = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Step", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StepHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Step", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float StepDuration = 0.15f;

	/** Predict the body motion this far ahead when evaluating the next footfall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Prediction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LookAheadSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Prediction", meta = (ClampMin = "0.0"))
	float MaxPredictedYawSpeed = 480.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Step", meta = (ClampMin = "1.0"))
	float MaxStepDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Step", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GroupStepThresholdScale = 0.5f;

	/** False gives a stable four-beat walk suitable for armed enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Step")
	bool bMoveGaitGroupTogether = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float GroundCheckDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground")
	float GroundCheckStartOffset = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground", meta = (ClampMin = "0.0"))
	float GroundProbeRadius = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinGroundNormalZ = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground", meta = (ClampMin = "1.0"))
	float MaxLegReach = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Debug")
	bool bDrawDebug = false;

	UFUNCTION(BlueprintPure, Category = "Quadruped")
	FVector GetFootPosition(int32 LegIndex) const;

	UFUNCTION(BlueprintPure, Category = "Quadruped")
	FVector GetNextFootPosition(int32 LegIndex) const;

	UFUNCTION(BlueprintPure, Category = "Quadruped")
	bool HasValidNextFootPosition(int32 LegIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Quadruped")
	void InitializeDefaultLegs(float BodyLength = 60.0f, float BodyWidth = 30.0f);

private:
	void UpdateMotionPrediction(float DeltaTime);
	void UpdatePlantedSupportPositions();
	void UpdateLegTargets();
	void ResetLegPositionsToTargets();
	void ProcessGaitCycle(float DeltaTime);
	void InterpolateLegPositions(float DeltaTime);
	void StartStep(FQuadrupedLegData& Leg);
	void FinishStep(FQuadrupedLegData& Leg);
	bool IsGaitGroupStepping(int32 GroupIndex) const;
	bool TraceGround(const FVector& WorldLocation, FHitResult& OutHit) const;
	bool IsCandidateReachable(const FQuadrupedLegData& Leg, const FVector& CandidatePosition) const;
	float CalculatePlacementError(const FQuadrupedLegData& Leg) const;
	FVector CalculateIdealFootPosition(int32 LegIndex) const;

	FTransform PredictedOwnerTransform = FTransform::Identity;
	float PreviousOwnerYaw = 0.0f;
	bool bHasPreviousOwnerYaw = false;
};
