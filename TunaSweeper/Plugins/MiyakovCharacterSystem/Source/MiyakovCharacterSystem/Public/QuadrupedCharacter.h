// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "QuadrupedCharacter.generated.h"

class UQuadrupedComponent;

/**
 * Base character class for quadruped (4-legged) creatures.
 * Uses procedural IK animation via QuadrupedComponent.
 * Designed for dog-like companions.
 */
UCLASS()
class MIYAKOVCHARACTERSYSTEM_API AQuadrupedCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AQuadrupedCharacter();

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quadruped")
	UQuadrupedComponent* QuadrupedComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RunSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Body", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float BodyLength = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|Body", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float BodyWidth = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|IK")
	FName FrontLeftBone = TEXT("foot_fl");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|IK")
	FName FrontRightBone = TEXT("foot_fr");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|IK")
	FName BackLeftBone = TEXT("foot_bl");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quadruped|IK")
	FName BackRightBone = TEXT("foot_br");

	UFUNCTION(BlueprintPure, Category = "Quadruped|IK")
	FVector GetLegIKTarget(int32 LegIndex) const;

	UFUNCTION(BlueprintPure, Category = "Quadruped|IK")
	void GetAllLegIKTargets(FVector& OutFrontLeft, FVector& OutFrontRight, FVector& OutBackLeft, FVector& OutBackRight) const;

	UFUNCTION(BlueprintPure, Category = "Quadruped")
	bool IsMoving() const;

protected:
	UFUNCTION(BlueprintCallable, Category = "Quadruped")
	void ReinitializeLegs();
};
