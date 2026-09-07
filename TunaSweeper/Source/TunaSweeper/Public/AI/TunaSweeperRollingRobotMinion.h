#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "TunaSweeperRollingRobotMinion.generated.h"

class UPrimitiveComponent;

UENUM(BlueprintType)
enum class ETunaSweeperRollingRobotPhase : uint8
{
	Rolling,
	Unfolding,
	Walking,
	Dead
};

/** Damageable from spawn; rolls out as a ball, unfolds, then uses ordinary enemy melee AI. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRollingRobotMinion : public ATunaSweeperEnemyCharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperRollingRobotMinion();

	/** Direction is planar and normalized internally. Safe before FinishSpawning or during play. */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Rolling Robot")
	void InitializeRoll(const FVector& Direction, AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Robot")
	ETunaSweeperRollingRobotPhase GetDeploymentPhase() const { return DeploymentPhase; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Robot")
	bool IsDeployingFromRoll() const;

	/** Radius used by rollout collision and deferred-spawn placement, including Blueprint tuning. */
	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Robot")
	float GetRollRadius() const { return FMath::Max(10.0f, BallRadius); }

	virtual bool IsStandardCombatSuppressed() const override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnDeathPresentationStarted() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RobotBodyPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RobotShell;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RobotEye;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftLeg;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightLeg;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftFoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightFoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rolling Robot|Deployment", meta = (ClampMin = "1.0", Units = "cm/s"))
	float RollSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rolling Robot|Deployment", meta = (ClampMin = "0.05", Units = "s"))
	float RollDurationSeconds = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rolling Robot|Deployment", meta = (ClampMin = "0.0", Units = "cm/s"))
	float LaunchUpwardSpeed = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rolling Robot|Deployment", meta = (ClampMin = "0.05", Units = "s"))
	float UnfoldDurationSeconds = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rolling Robot|Deployment", meta = (ClampMin = "10.0", Units = "cm"))
	float BallRadius = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rolling Robot|Deployment", meta = (ClampMin = "10.0", Units = "cm"))
	float StandingHalfHeight = 66.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Robot|Visual")
	TSoftObjectPtr<UMaterialInterface> ShellMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Robot|Visual")
	TSoftObjectPtr<UMaterialInterface> LegMaterial;

private:
	void StartRollMovement();
	bool TryBeginUnfolding();
	void FinishUnfolding();
	void UpdateRobotPresentation(float UnfoldAlpha, float DeltaSeconds);
	void ApplyRobotMaterials();

	UFUNCTION()
	void HandleCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(Transient)
	ETunaSweeperRollingRobotPhase DeploymentPhase = ETunaSweeperRollingRobotPhase::Rolling;

	TWeakObjectPtr<AActor> DeploymentTarget;
	FVector RollDirection = FVector::ForwardVector;
	FVector LastPresentationLocation = FVector::ZeroVector;
	float DeploymentElapsedSeconds = 0.0f;
	float BodyRollDegrees = 0.0f;
	float WalkCycleRadians = 0.0f;
	float SavedWalkSpeed = 300.0f;
	float SavedGroundFriction = 8.0f;
	float SavedBrakingDeceleration = 2048.0f;
	float SavedFootstepLoudness = 0.3f;
	bool bRollWasInitialized = false;
	bool bMovementSettingsSaved = false;
	bool bRollBlocked = false;
};
