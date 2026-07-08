// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "QuadrupedComponent.generated.h"

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

	// Target foot world position
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	FVector TargetPosition = FVector::ZeroVector;

	// Step interpolation progress (0 to 1)
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	float StepProgress = 0.0f;

	// Is this leg currently stepping?
	UPROPERTY(BlueprintReadOnly, Category = "Leg")
	bool bIsMoving = false;

	// Gait group (0 or 1) - diagonal pairs step together
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leg")
	int32 GaitGroup = 0;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float GroundCheckDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Ground")
	float GroundCheckStartOffset = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Debug")
	bool bDrawDebug = false;

	UFUNCTION(BlueprintPure, Category = "Quadruped")
	FVector GetFootPosition(int32 LegIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Quadruped")
	void InitializeDefaultLegs(float BodyLength = 60.0f, float BodyWidth = 30.0f);

private:
	void UpdateLegTargets();
	void ProcessGaitCycle(float DeltaTime);
	void InterpolateLegPositions(float DeltaTime);
	bool IsGaitGroupStepping(int32 GroupIndex) const;
	bool TraceGround(const FVector& WorldLocation, FVector& OutGroundPosition) const;
	FVector CalculateIdealFootPosition(int32 LegIndex) const;

	int32 CurrentGaitGroup = 0;
	float GaitTimer = 0.0f;
};
