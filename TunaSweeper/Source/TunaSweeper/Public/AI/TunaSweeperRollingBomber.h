#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "TunaSweeperRollingBomber.generated.h"

class ATunaSweeperProjectile;
class ATunaSweeperTopDownCharacter;
class UDamageType;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ETunaSweeperRollingBomberMode : uint8
{
	ProjectileAttack,
	FoldingLegs,
	Rolling,
	RecoveringLegs,
	SelfDestructed
};

UENUM(BlueprintType)
enum class ETunaSweeperRollingBomberFoot : uint8
{
	Left,
	Right
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperRollingBomberFootIKState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	FVector EffectorWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	FVector JointTargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	FVector PlannedFootWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK")
	bool bIsStepping = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Rolling Bomber|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StepAlpha = 0.0f;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRollingBomber : public ATunaSweeperEnemyCharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperRollingBomber();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Rolling Bomber|Spawner")
	void LaunchFromSpawner(const FVector& LaunchVelocity);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber")
	ETunaSweeperRollingBomberMode GetRollingBomberMode() const { return CurrentMode; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|IK")
	bool IsLegIKEnabled() const { return bLegIKEnabled; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|IK")
	float GetLegFoldAlpha() const { return LegFoldAlpha; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|IK")
	FTunaSweeperRollingBomberFootIKState GetFootIKState(ETunaSweeperRollingBomberFoot Foot) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Eye")
	bool IsEyeChargeWarningActive() const { return bEyeChargeWarningActive; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Eye")
	FLinearColor GetEyeEmissiveColor() const { return CurrentEyeColor; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Eye")
	float GetEyeEmissiveStrength() const { return CurrentEyeEmissiveStrength; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Roll")
	FVector GetLockedRollDirection() const { return LockedRollDirection; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Roll")
	float GetRollDistanceTraveled() const { return RollDistanceTraveled; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Roll")
	float GetBodyRollDegrees() const { return BodyRollDegrees; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LeftFootIKTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RightFootIKTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LeftKneeIKTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RightKneeIKTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EyeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> EyeLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileAttackDurationSeconds = 4.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float ProjectileFireIntervalSeconds = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TargetTrackingRange = 1900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileAttackRange = 1050.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileApproachStartRange = 1050.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileApproachStopRange = 820.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileModeWalkSpeed = 210.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Legs", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float LegFoldDurationSeconds = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Legs", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float LegUnfoldDurationSeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollSpeed = 1150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float RollDurationSeconds = 1.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollMaxDistance = 950.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollContactRadius = 42.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionRadius = 240.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionDamage = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct")
	TSubclassOf<UDamageType> ExplosionDamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	FVector LeftFootHomeLocalOffset = FVector(-8.0f, -30.0f, -50.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	FVector RightFootHomeLocalOffset = FVector(-8.0f, 30.0f, -50.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootStepTriggerDistance = 46.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FootStepDurationSeconds = 0.16f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootStepHeight = 22.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootMoveLeadDistance = 28.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float KneeForwardOffset = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float KneeSideOffset = 16.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float KneeHeightOffset = 44.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootGroundTraceUp = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootGroundTraceDown = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootGroundClearance = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Visual")
	bool bUseBodyRollVisualRotation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Visual", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BodyVisualRadiusCm = 42.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	TSoftObjectPtr<UMaterialInterface> EyeMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	FVector EyeLocalOffset = FVector(43.0f, 0.0f, 18.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	FVector EyeLocalScale = FVector(0.11f, 0.24f, 0.24f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	FLinearColor NormalEyeColor = FLinearColor(0.05f, 0.45f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	FLinearColor ChargeWarningEyeColor = FLinearColor(1.0f, 0.02f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NormalEyeEmissiveStrength = 1.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ChargeWarningEyeEmissiveStrength = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NormalEyeLightIntensity = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ChargeWarningEyeLightIntensity = 1400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float NormalEyeLightRadius = 95.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ChargeWarningEyeLightRadius = 280.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EyeWarningInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnerLaunchControlGraceSeconds = 0.8f;

private:
	struct FFootRuntime
	{
		FVector EffectorWorldLocation = FVector::ZeroVector;
		FVector JointTargetWorldLocation = FVector::ZeroVector;
		FVector PlannedFootWorldLocation = FVector::ZeroVector;
		FVector StepStartWorldLocation = FVector::ZeroVector;
		float StepElapsedSeconds = 0.0f;
		float StepAlpha = 0.0f;
		bool bIsStepping = false;
		bool bInitialized = false;
	};

	void ApplyRollingBomberVisualDefaults();
	void EnterProjectileAttackMode();
	void EnterFoldingLegsMode();
	void EnterRollingMode(ATunaSweeperTopDownCharacter* TargetCharacter);
	void EnterRecoveringLegsMode();
	void SelfDestruct();

	void UpdateProjectileAttackMode(float DeltaSeconds, ATunaSweeperTopDownCharacter* TargetCharacter);
	void UpdateProjectileModeMovement(float DistanceToTarget, const FVector& DirectionToTarget);
	void UpdateFoldingLegsMode(float DeltaSeconds, ATunaSweeperTopDownCharacter* TargetCharacter);
	void UpdateRollingMode(float DeltaSeconds, ATunaSweeperTopDownCharacter* TargetCharacter);
	void UpdateRecoveringLegsMode(float DeltaSeconds);
	void UpdateLegIK(float DeltaSeconds, const FVector& PlanarMoveDirection);
	void UpdateSpawnerLaunchState(float DeltaSeconds);

	ATunaSweeperTopDownCharacter* ResolvePlayerTarget() const;
	bool FireRollingBomberProjectileAt(AActor* TargetActor);
	FVector ResolveRollDirection(ATunaSweeperTopDownCharacter* TargetCharacter) const;
	bool TrySelfDestructFromRollingContact(ATunaSweeperTopDownCharacter* TargetCharacter);
	bool IsActorInRollContact(const AActor* Actor) const;
	void ApplyExplosionDamage();
	void ApplyBodyRollVisualRotation(float DeltaDistance);
	void ResetBodyRollVisualRotation();
	void ApplyEyeVisualDefaults();
	void SetEyeChargeWarningActive(bool bActive, bool bInstant);
	void UpdateEyeVisualState(float DeltaSeconds);
	void ApplyEyeMaterialState(const FLinearColor& EyeColor, float EmissiveStrength);
	float CalculateEyeChargeAlpha(float EmissiveStrength) const;

	void InitializeLegIKTargets();
	void InitializeFootRuntime(FFootRuntime& FootRuntime, const FVector& FootHomeLocalOffset);
	void BeginFootStep(FFootRuntime& FootRuntime, const FVector& PlannedFootWorldLocation);
	void AdvanceFootStep(FFootRuntime& FootRuntime, float DeltaSeconds);
	FVector CalculatePlannedFootLocation(const FVector& FootHomeLocalOffset, const FVector& PlanarMoveDirection) const;
	FVector CalculateJointTargetLocation(const FVector& FootWorldLocation, float SideSign) const;
	FVector ResolveGroundedFootLocation(const FVector& DesiredWorldLocation) const;
	void UpdateFootSceneComponents();
	void UpdateFoldedLegSceneComponents();
	FTunaSweeperRollingBomberFootIKState BuildFootIKState(const FFootRuntime& FootRuntime) const;

	FFootRuntime LeftFootRuntime;
	FFootRuntime RightFootRuntime;
	ETunaSweeperRollingBomberMode CurrentMode = ETunaSweeperRollingBomberMode::ProjectileAttack;
	ETunaSweeperRollingBomberFoot NextStepFoot = ETunaSweeperRollingBomberFoot::Left;
	FVector LockedRollDirection = FVector::ForwardVector;
	FVector LastActorLocation = FVector::ZeroVector;
	FRotator BodyVisualBaseRelativeRotation = FRotator::ZeroRotator;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EyeDynamicMaterial;

	FLinearColor CurrentEyeColor = FLinearColor(0.05f, 0.45f, 1.0f, 1.0f);
	float ProjectileModeElapsedSeconds = 0.0f;
	float ProjectileFireElapsedSeconds = 0.0f;
	float ModeElapsedSeconds = 0.0f;
	float RollDistanceTraveled = 0.0f;
	float BodyRollDegrees = 0.0f;
	float LegFoldAlpha = 0.0f;
	float CurrentEyeEmissiveStrength = 1.6f;
	float SpawnerLaunchControlRemainingSeconds = 0.0f;
	bool bLegIKEnabled = true;
	bool bProjectileModeClosingDistance = false;
	bool bEyeChargeWarningActive = false;
	bool bHasSelfDestructed = false;
	bool bSpawnerLaunchActive = false;
};
