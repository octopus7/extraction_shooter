#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperLedRobotCharacterActor.generated.h"

class UMaterialInterface;
class UCapsuleComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTunaSweeperLedExpressionComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLedRobotCharacterActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLedRobotCharacterActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Robot")
	void ConfigureRobotDefaults(
		FName InRobotId,
		const FString& InExpressionPresetFilePath,
		FName InInitialExpressionName,
		FLinearColor InLedColor,
		FLinearColor InOffColor,
		float InLedPitch,
		float InLedRadius,
		TSoftObjectPtr<UMaterialInterface> InBodyMaterial,
		bool bOverrideLedColor = true,
		bool bOverrideOffColor = true,
		bool bOverrideLedPitch = true,
		bool bOverrideLedRadius = true);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Robot")
	bool SetExpressionByName(FName ExpressionName);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Robot")
	void ConfigureExpressionDemo(bool bEnabled, float InIntervalSeconds);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Robot")
	void SetExpressionDemoModeEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Robot")
	bool IsExpressionDemoModeEnabled() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Robot")
	FName GetRobotId() const { return RobotId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Robot")
	UTunaSweeperLedExpressionComponent* GetExpressionComponent() const { return ExpressionComponent; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void RefreshRobotVisuals();
	void RefreshExpressionDemoSettings();
	void UpdatePlayerLookAt(float DeltaSeconds);
	bool TryGetPlayerLookYaw(float& OutYaw, float& OutDistance2D) const;
	float ResolveNonMechanicalYawOffset(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> BodyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperLedExpressionComponent> ExpressionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	FName RobotId = TEXT("BunkerRobot");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	FName InitialExpressionName = TEXT("Smile");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UStaticMesh> BodyMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	FVector BodyRelativeLocation = FVector(0.0f, 0.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	FVector BodyScale = FVector(0.82f, 1.70f, 1.80f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Collision", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float BodyCollisionRadius = 86.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Collision", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float BodyCollisionHalfHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Face", meta = (AllowPrivateAccess = "true"))
	FString ExpressionPresetFilePath = TEXT("Data/LedExpressionPresets.txt");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Face", meta = (AllowPrivateAccess = "true"))
	bool bExpressionDemoMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Face", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", UIMin = "0.1"))
	float ExpressionDemoIntervalSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true"))
	bool bLookAtNearbyPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtStartDistance = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtStopDistance = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtInterpolationSpeed = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtReturnInterpolationSpeed = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtMinReactionDelay = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtMaxReactionDelay = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtTargetRefreshInterval = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtYawOffsetDegrees = 3.0f;

	FRotator IdleActorRotation = FRotator::ZeroRotator;
	float PendingLookAtYaw = 0.0f;
	float LookAtReactionElapsed = 0.0f;
	float LookAtReactionDelay = 0.0f;
	float LookAtRefreshElapsed = 0.0f;
	float LookAtYawOffset = 0.0f;
	float LookAtYawOffsetTarget = 0.0f;
	float LookAtYawOffsetRefreshElapsed = 0.0f;
	bool bIsLookingAtPlayer = false;
	bool bLookAtReactionPending = false;
};
