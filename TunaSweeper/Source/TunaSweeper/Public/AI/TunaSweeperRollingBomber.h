#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "TunaSweeperRollingBomber.generated.h"

class ATunaSweeperProjectile;
class ATunaSweeperLocalExplosionEffectActor;
class ATunaSweeperTopDownCharacter;
class UDamageType;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPhysicalMaterial;
class UPointLightComponent;
class UCapsuleComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ETunaSweeperRollingBomberMode : uint8
{
	SpawnPhysics,
	StandingUpFromSpawn,
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
	virtual FVector ResolveProjectileHitEffectLocation(const FHitResult& Hit) const override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Rolling Bomber|Spawner")
	void LaunchFromSpawner(const FVector& LaunchVelocity);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber")
	ETunaSweeperRollingBomberMode GetRollingBomberMode() const { return CurrentMode; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Spawn")
	bool IsSpawnPhysicsActive() const { return CurrentMode == ETunaSweeperRollingBomberMode::SpawnPhysics; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Spawn")
	bool IsStandingUpFromSpawn() const { return CurrentMode == ETunaSweeperRollingBomberMode::StandingUpFromSpawn; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Rolling Bomber|Spawn")
	float GetSpawnStandUpAlpha() const { return SpawnStandUpAlpha; }

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
	TObjectPtr<USceneComponent> BodyVisualPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RollChargeCylinderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftUpperLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftLowerLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftFootMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightUpperLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightLowerLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightFootMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EyeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> EyeLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> ProjectileHurtbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPhysicalMaterial> SpawnBouncePhysicalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileAttackDurationSeconds = 4.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float ProjectileFireIntervalSeconds = 2.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileInitialFireDelayMinSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileInitialFireDelayMaxSeconds = 1.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileFireIntervalJitterSeconds = 0.45f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileOrbitPreferredRange = 620.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileOrbitMinimumRange = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileOrbitStrafeWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileOrbitApproachWeight = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileOrbitCloseApproachWeight = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileOrbitRetreatWeight = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileDamageCap = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float RollingBomberProjectileScale = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollingBomberProjectileSpeedMultiplier = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileSpawnClearance = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile")
	TSoftObjectPtr<UMaterialInterface> RollingBomberProjectileMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile")
	TSoftObjectPtr<UMaterialInterface> RollingBomberProjectileTrailMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile")
	FLinearColor RollingBomberProjectileColor = FLinearColor(1.0f, 0.34f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollingBomberProjectileEmissiveStrength = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile")
	FLinearColor RollingBomberProjectileTrailColor = FLinearColor(0.08f, 0.55f, 1.0f, 0.5f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollingBomberProjectileTrailEmissiveStrength = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float RollingBomberProjectileTrailLengthCm = 65.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float RollingBomberProjectileTrailRadiusCm = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float RollingBomberProjectileTrailOpacity = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float RollingBomberProjectileTrailEndFade = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollingBomberProjectileCameraHitReactionScale = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Hitbox", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileHurtboxRadiusCm = 22.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Hitbox", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileHurtboxHalfHeightCm = 88.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Hitbox")
	FVector ProjectileHurtboxLocalOffset = FVector(0.0f, 0.0f, 38.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Legs", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float LegFoldDurationSeconds = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Legs", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float LegUnfoldDurationSeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollSpeed = 1725.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float RollDurationSeconds = 1.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollMaxDistance = 950.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollContactRadius = 21.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollChargeSpinStartDegreesPerSecond = 540.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollChargeSpinEndDegreesPerSecond = 2160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollLaunchVisualHopHeightCm = 14.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float RollLaunchVisualHopDurationSeconds = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll")
	TSoftObjectPtr<UStaticMesh> RollChargeCylinderMeshAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll")
	TSoftObjectPtr<UMaterialInterface> RollChargeCylinderMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll")
	FVector RollChargeCylinderLocalOffset = FVector(0.0f, 0.0f, 6.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll")
	FVector RollChargeCylinderLocalScale = FVector(0.92f, 0.72f, 0.72f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollChargeCylinderMinOpacity = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollChargeCylinderMaxOpacity = 0.82f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Roll", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RollChargeCylinderPulseScale = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionRadius = 240.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct")
	TSubclassOf<UDamageType> ExplosionDamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct")
	TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor> SelfDestructExplosionEffectActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float SelfDestructExplosionVisualRadiusMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SelfDestructExplosionDurationSeconds = 0.72f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	FVector LeftFootHomeLocalOffset = FVector(-4.0f, -15.0f, -25.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	FVector RightFootHomeLocalOffset = FVector(-4.0f, 15.0f, -25.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	FVector LeftHipLocalOffset = FVector(-2.0f, -11.0f, -9.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	FVector RightHipLocalOffset = FVector(-2.0f, 11.0f, -9.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootStepTriggerDistance = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FootStepDurationSeconds = 0.16f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootStepHeight = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootMoveLeadDistance = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float KneeForwardOffset = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float KneeSideOffset = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float KneeHeightOffset = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float UpperLegLengthCm = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float LowerLegLengthCm = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LegReachSlackCm = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootGroundTraceUp = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootGroundTraceDown = 140.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|IK")
	float FootGroundClearance = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Leg Mesh", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float UpperLegThicknessCm = 6.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Leg Mesh", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float LowerLegThicknessCm = 5.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Leg Mesh", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float FootVisualLengthCm = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Leg Mesh", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float FootVisualWidthCm = 5.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Leg Mesh", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float FootVisualHeightCm = 2.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Leg Mesh")
	TSoftObjectPtr<UMaterialInterface> LegMetalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Visual")
	bool bUseBodyRollVisualRotation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Visual", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BodyVisualRadiusCm = 21.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Visual", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BodyRollWobbleDegrees = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	TSoftObjectPtr<UMaterialInterface> EyeMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	FVector EyeLocalOffset = FVector(21.5f, 0.0f, 9.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye")
	FVector EyeLocalScale = FVector(0.055f, 0.12f, 0.12f);

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
	float NormalEyeLightRadius = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ChargeWarningEyeLightRadius = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Eye", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EyeWarningInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnerLaunchControlGraceSeconds = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnerPhysicsMinimumSeconds = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SpawnStandUpDurationSeconds = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnPhysicsSettleSpeed = 85.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SpawnBounceRestitution = 0.72f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnBounceFriction = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rolling Bomber|Spawner", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpawnPhysicsAngularVelocityDegrees = 620.0f;

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
	void ApplyRollChargeCylinderVisualDefaults();
	void EnterSpawnPhysicsMode(const FVector& LaunchVelocity);
	void EnterStandingUpFromSpawnMode();
	void EnterProjectileAttackMode();
	void EnterFoldingLegsMode(ATunaSweeperTopDownCharacter* TargetCharacter);
	void EnterRollingMode(ATunaSweeperTopDownCharacter* TargetCharacter);
	void EnterRecoveringLegsMode();
	void SelfDestruct();

	void UpdateSpawnPhysicsMode(float DeltaSeconds);
	void UpdateStandingUpFromSpawnMode(float DeltaSeconds);
	void UpdateProjectileAttackMode(float DeltaSeconds, ATunaSweeperTopDownCharacter* TargetCharacter);
	void UpdateProjectileModeMovement(float DistanceToTarget, const FVector& DirectionToTarget);
	void UpdateFoldingLegsMode(float DeltaSeconds, ATunaSweeperTopDownCharacter* TargetCharacter);
	void UpdateRollingMode(float DeltaSeconds, ATunaSweeperTopDownCharacter* TargetCharacter);
	void UpdateRecoveringLegsMode(float DeltaSeconds);
	void UpdateLegIK(float DeltaSeconds, const FVector& PlanarMoveDirection);
	void FinishSpawnPhysicsSimulation();
	bool IsSpawnPhysicsGrounded() const;
	bool IsSpawnPhysicsSettled() const;

	ATunaSweeperTopDownCharacter* ResolvePlayerTarget() const;
	float ResolveProjectileFireIntervalSeconds() const;
	void ResetProjectileFireTimer(bool bUseInitialDelay);
	bool FireRollingBomberProjectileAt(AActor* TargetActor);
	FVector ResolveRollDirection(ATunaSweeperTopDownCharacter* TargetCharacter) const;
	bool TrySelfDestructFromRollingContact(ATunaSweeperTopDownCharacter* TargetCharacter);
	bool IsActorInRollContact(const AActor* Actor) const;
	void SpawnSelfDestructBurst();
	void ApplyExplosionDamage();
	void ApplyBodyRollVisualRotation(float DeltaDistance);
	void ApplyBodyRollVisualRotationDegrees(float DeltaDegrees);
	void ApplyRollLaunchVisualHop();
	void SetRollChargeCylinderEffectActive(bool bActive);
	void UpdateRollChargeCylinderEffect(float ChargeAlpha);
	void ResetBodyRollVisualRotation();
	void ApplyLegVisualMaterial();
	void ApplyEyeVisualDefaults();
	void SetEyeChargeWarningActive(bool bActive, bool bInstant);
	void UpdateEyeVisualState(float DeltaSeconds);
	void ApplyEyeMaterialState(const FLinearColor& EyeColor, float EmissiveStrength);
	float CalculateEyeChargeAlpha(float EmissiveStrength) const;

	void InitializeLegIKTargets();
	void InitializeFootRuntime(
		FFootRuntime& FootRuntime,
		const FVector& FootHomeLocalOffset,
		const FVector& HipLocalOffset);
	void BeginFootStep(FFootRuntime& FootRuntime, const FVector& PlannedFootWorldLocation);
	void AdvanceFootStep(FFootRuntime& FootRuntime, float DeltaSeconds);
	FVector CalculatePlannedFootLocation(const FVector& FootHomeLocalOffset, const FVector& PlanarMoveDirection) const;
	FVector ClampFootLocationToLegReach(const FVector& HipWorldLocation, const FVector& DesiredFootWorldLocation) const;
	FVector CalculateJointTargetLocation(
		const FVector& HipWorldLocation,
		const FVector& FootWorldLocation,
		float SideSign) const;
	FVector ResolveGroundedFootLocation(const FVector& DesiredWorldLocation) const;
	void UpdateFootSceneComponents();
	void UpdateFoldedLegSceneComponents();
	void UpdateVisibleLegMeshes();
	void UpdateVisibleLegMeshForFoot(
		UStaticMeshComponent* UpperLegMesh,
		UStaticMeshComponent* LowerLegMesh,
		UStaticMeshComponent* FootMesh,
		const FVector& HipWorldLocation,
		const FFootRuntime& FootRuntime) const;
	void PositionLegSegmentMesh(
		UStaticMeshComponent* SegmentMesh,
		const FVector& StartWorldLocation,
		const FVector& EndWorldLocation,
		float ThicknessCm,
		float TargetLengthCm) const;
	void PositionFootMesh(UStaticMeshComponent* FootMesh, const FVector& FootWorldLocation) const;
	FTunaSweeperRollingBomberFootIKState BuildFootIKState(const FFootRuntime& FootRuntime) const;

	FFootRuntime LeftFootRuntime;
	FFootRuntime RightFootRuntime;
	ETunaSweeperRollingBomberMode CurrentMode = ETunaSweeperRollingBomberMode::ProjectileAttack;
	ETunaSweeperRollingBomberFoot NextStepFoot = ETunaSweeperRollingBomberFoot::Left;
	FVector LockedRollDirection = FVector::ForwardVector;
	FVector LastActorLocation = FVector::ZeroVector;
	FVector BodyVisualPivotBaseRelativeLocation = FVector::ZeroVector;
	FRotator BodyVisualBaseRelativeRotation = FRotator::ZeroRotator;
	FRotator BodyVisualPivotBaseRelativeRotation = FRotator::ZeroRotator;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EyeDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RollChargeCylinderDynamicMaterial;

	FLinearColor CurrentEyeColor = FLinearColor(0.05f, 0.45f, 1.0f, 1.0f);
	float ProjectileModeElapsedSeconds = 0.0f;
	float ProjectileFireElapsedSeconds = 0.0f;
	float CurrentProjectileFireIntervalSeconds = 2.4f;
	float ModeElapsedSeconds = 0.0f;
	float RollDistanceTraveled = 0.0f;
	float BodyRollDegrees = 0.0f;
	float LegFoldAlpha = 0.0f;
	float SpawnStandUpAlpha = 0.0f;
	float CurrentEyeEmissiveStrength = 1.6f;
	float SpawnerLaunchControlRemainingSeconds = 0.0f;
	float SpawnPhysicsElapsedSeconds = 0.0f;
	float ProjectileOrbitDirectionSign = 1.0f;
	bool bLegIKEnabled = true;
	bool bProjectileModeClosingDistance = false;
	bool bEyeChargeWarningActive = false;
	bool bHasSelfDestructed = false;
	bool bSpawnPhysicsSimulationActive = false;
};
