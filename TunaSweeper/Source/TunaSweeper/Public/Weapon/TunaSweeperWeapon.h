#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperWeapon.generated.h"

class ATunaSweeperProjectile;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UWorld;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWeapon : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperWeapon();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void Fire(
		const FVector& AimDirection,
		APawn* InstigatorPawn,
		FName ProjectileHitEffectId = NAME_None,
		FName WeaponTypeTag = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void ConfigureGunVisual();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void ConfigureMeleeVisual();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Weapon")
	void SetWeaponMeshOverride(
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		FVector RelativeLocation,
		FRotator RelativeRotation,
		FVector RelativeScale);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSoftClassPtr<ATunaSweeperProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireCooldown = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shotgun", meta = (ClampMin = "1", UIMin = "1"))
	int32 ShotgunProjectileCount = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shotgun", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "360.0", UIMax = "120.0"))
	float ShotgunSpreadAngleDegrees = 36.0f;

	ATunaSweeperProjectile* SpawnProjectile(
		UWorld& World,
		TSubclassOf<ATunaSweeperProjectile> ProjectileClassToSpawn,
		const FVector& ShotDirection,
		APawn* InstigatorPawn,
		FName ProjectileHitEffectId);

private:
	float LastFireTimeSeconds = -1000.0f;
};
