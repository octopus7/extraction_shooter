#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"
#include "TunaSweeperWeapon.generated.h"

class ATunaSweeperProjectile;
class ATunaSweeperShellCasing;
class UTunaSweeperWeaponCombatComponent;
class UTunaSweeperWeaponPresentationDataAsset;
class UMaterialInterface;
class UPointLightComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UTunaSweeperLaserSightComponent;
class UWorld;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWeapon : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperWeapon();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	bool Fire(
		const FVector& AimDirection,
		APawn* InstigatorPawn,
		FName ProjectileHitEffectId = NAME_None,
		FName WeaponTypeTag = NAME_None);

	bool FireWithAimIntent(
		const FVector& AimDirection,
		APawn* InstigatorPawn,
		FName ImpactProfileId,
		FName ProjectileHitEffectId,
		FName WeaponTypeTag,
		float ProjectileDamageMultiplier = 1.0f,
		int32 ProjectileDamageBonus = 0,
		float SpreadHalfAngleDegrees = 0.0f,
		const FVector& AimWorldPoint = FVector::ZeroVector,
		bool bHasAimWorldPoint = false,
		AActor* AimIntentActor = nullptr,
		UPrimitiveComponent* AimIntentComponent = nullptr,
		FVector AimIntentWorldPoint = FVector::ZeroVector,
		bool bHasAimIntentWorldPoint = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void ConfigureGunVisual();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void ConfigureMeleeVisual();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	UTunaSweeperWeaponCombatComponent* GetCombatComponent() const { return CombatComponent; }

	void ConfigureRuntimeSpreadRecoil(
		FName WeaponTypeTag,
		const FTunaSweeperWeaponSpreadRecoilDefinition& RecoilDefinition);

	void ClearRuntimeSpreadRecoil();
	void ResetRuntimeSpreadRecoil();
	float GetRuntimeSpreadHalfAngleDegrees() const;
	float GetRuntimeAimedSpreadHalfAngleDegrees() const;
	void AddRuntimeSpreadRecoilShot();
	bool StartReloadRuntime(float ReloadSeconds);
	void FinishReloadRuntime();
	void CancelReloadRuntime();
	bool IsReloadRuntimeActive() const;
	bool HasReloadRuntimeFinished() const;
	float GetReloadRuntimeProgress() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void SetLaserSightEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon")
	bool IsLaserSightEnabled() const;

	FVector GetMuzzleWorldLocation() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void UpdateLaserSightBeam(
		const FVector& AimDirection,
		const FVector& AimWorldPoint,
		bool bHasAimWorldPoint);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void SetWeaponMeshOverride(
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		FVector RelativeLocation,
		FRotator RelativeRotation,
		FVector RelativeScale);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Presentation")
	void SetWeaponPresentationDataAsset(TSoftObjectPtr<UTunaSweeperWeaponPresentationDataAsset> InWeaponPresentationDataAsset);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Weapon|Presentation")
	bool HasWeaponPresentationDataAsset() const { return !WeaponPresentationDataAsset.IsNull(); }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon|Presentation")
	bool TryPlayEmptyFirePresentation();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Presentation")
	TObjectPtr<UPointLightComponent> MuzzleFlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UTunaSweeperLaserSightComponent> LaserSightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UTunaSweeperWeaponCombatComponent> CombatComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSoftClassPtr<ATunaSweeperProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shell Casing")
	TSubclassOf<ATunaSweeperShellCasing> ShellCasingClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Presentation")
	TSoftObjectPtr<UTunaSweeperWeaponPresentationDataAsset> WeaponPresentationDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Presentation|Muzzle Flash Light")
	FLinearColor MuzzleFlashLightColor = FLinearColor(1.0f, 0.28f, 0.06f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Presentation|Muzzle Flash Light", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MuzzleFlashLightIntensity = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Presentation|Muzzle Flash Light", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MuzzleFlashLightAttenuationRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Presentation|Muzzle Flash Light", meta = (ClampMin = "0.01", UIMin = "0.01", ClampMax = "0.2", UIMax = "0.2"))
	float MuzzleFlashLightDuration = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireCooldown = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shotgun", meta = (ClampMin = "1", UIMin = "1"))
	int32 ShotgunProjectileCount = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shotgun", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "360.0", UIMax = "120.0"))
	float ShotgunSpreadAngleDegrees = 36.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Laser", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float LaserSightFallbackRange = 5000.0f;

	ATunaSweeperProjectile* SpawnProjectile(
		UWorld& World,
		TSubclassOf<ATunaSweeperProjectile> ProjectileClassToSpawn,
		const FVector& ShotDirection,
		APawn* InstigatorPawn,
		FName ImpactProfileId,
		FName ProjectileHitEffectId,
		float ProjectileDamageMultiplier,
		int32 ProjectileDamageBonus,
		AActor* AimIntentActor,
		UPrimitiveComponent* AimIntentComponent,
		const FVector& AimIntentWorldPoint,
		bool bHasAimIntentWorldPoint);

	void PlayFirePresentation();
	void PlayReloadPresentation(TSoftObjectPtr<class USoundBase> ReloadSound);
	void EjectShellCasing(UWorld& World, APawn* InstigatorPawn);
	void TriggerMuzzleFlashLight();
	void DeactivateMuzzleFlashLight();

private:
	bool TryGetWeaponSocketWorldTransform(FName SocketName, FTransform& OutTransform) const;
	FTransform GetMuzzleWorldTransform() const;
	FTransform GetLaserSightWorldTransform() const;

	float LastFireTimeSeconds = -1000.0f;
	float LastEmptyFirePresentationTimeSeconds = -1000.0f;
	float LastLaserSightDebugLogTimeSeconds = -1000.0f;
	FTimerHandle MuzzleFlashLightTimerHandle;
};
