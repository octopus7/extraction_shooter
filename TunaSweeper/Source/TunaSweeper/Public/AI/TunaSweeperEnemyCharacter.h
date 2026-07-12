#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperEnemyCharacter.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UTunaSweeperVisionSubjectComponent;
class UTunaSweeperEnemySensorDebugComponent;
class UWidgetComponent;
class UMaterialInterface;
class UNiagaraSystem;
class ATunaSweeperProjectile;
class ATunaSweeperWeapon;
class ATunaSweeperLootContainerActor;
class ATunaSweeperMeleeImpactBurstActor;
class ATunaSweeperMeleeSwingTrailActor;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperEnemyCharacter();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	virtual FVector ResolveProjectileHitEffectLocation(const FHitResult& Hit) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat")
	bool FireProjectileAt(AActor* TargetActor);

	bool AttackTarget(AActor* TargetActor);
	bool UsesMeleeAttack() const;
	float GetMeleeAttackRange() const;
	float GetMeleeApproachStartRange() const;
	float GetMeleeApproachStopRange() const;
	float GetMeleeTrackingRange() const;
	float GetMeleeAttackCooldownSeconds() const;
	bool TryApplyBleedTo(AActor* TargetActor) const;
	void SetAlertIndicatorVisible(bool bVisible);

	void ConfigureSpawnData(
		const TSoftObjectPtr<UMaterialInterface>& InBodyMaterial,
		FName InEnemyId,
		int32 InDropContainerDefinitionId,
		int32 InDropContentsId,
		float InMaxHealth,
		int32 InExperienceValue,
		int32 InBleedingChanceBonus = 0,
		float InBleedingDurationBonusSeconds = 0.0f,
		int32 InWeaponItemId = INDEX_NONE,
		int32 InAmmoItemId = INDEX_NONE,
		int32 InReserveAmmoCount = INDEX_NONE,
		float InLootLoadedAmmoDeductionRatio = 0.35f,
		int32 InLootLoadedAmmoFlatDeduction = 0);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ForwardMarkerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AlertIndicatorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EnemyWeaponAttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> EnemyReloadWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVisionSubjectComponent> VisionSubjectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperEnemySensorDebugComponent> SensorDebugComponent;

public:
	UTunaSweeperEnemySensorDebugComponent* GetSensorDebugComponent() const { return SensorDebugComponent; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<ATunaSweeperProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon")
	TSubclassOf<ATunaSweeperWeapon> EnemyWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FVector ProjectileSpawnOffset = FVector(60.0f, 0.0f, 55.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ProjectileDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FName ProjectileHitEffectId = FName(TEXT("hit.red_burst"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (DisplayName = "Melee Impact Niagara Effect"))
	TSoftObjectPtr<UNiagaraSystem> MeleeSwingEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSoftClassPtr<ATunaSweeperMeleeImpactBurstActor> MeleeImpactBurstActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSoftClassPtr<ATunaSweeperMeleeSwingTrailActor> MeleeSwingTrailActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debuffs", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 BleedingChanceBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Debuffs", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BleedingDurationBonusSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MovementSpeed = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Randomization")
	FVector2D MovementSpeedRandomOffset = FVector2D(-35.0f, 45.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootstepNoiseLoudness = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootstepNoiseMaxRange = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float FootstepNoiseIntervalSeconds = 0.42f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FootstepNoiseMinSpeed = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep")
	FName FootstepNoiseTag = FName(TEXT("noise.enemy_footstep"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise|Footstep")
	FVector FootstepNoiseSourceOffset = FVector(0.0f, 0.0f, 42.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UMaterialInterface> ForwardMarkerMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName EnemyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 DropContainerDefinitionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 DropContentsId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Experience", meta = (ClampMin = "0", UIMin = "0"))
	int32 ExperienceValue = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	int32 EnemyWeaponItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	int32 EnemyAmmoItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
	int32 EnemyReserveAmmoCount = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LootLoadedAmmoDeductionRatio = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Weapon", meta = (ClampMin = "0", UIMin = "0"))
	int32 LootLoadedAmmoFlatDeduction = 0;

private:
	void ApplyVoxelVisualMeshes();
	void ApplyVisualMaterials();
	void UpdateAlertIndicatorFacing();
	void InitializeEnemyWeaponRuntime();
	bool EnsureEnemyWeaponActor();
	bool StartEnemyReload();
	void CompleteEnemyReloadIfReady();
	void UpdateEnemyReloadWidget();
	bool TryCreateEnemyWeaponLootInstance(
		class UTunaSweeperGameInstance* TunaGameInstance,
		class UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		FGuid& OutWeaponUid) const;
	bool TryBuildDeathLootRuntimeItemUids(
		FTunaSweeperLootContainerInstance& OutContainerInstance,
		TArray<FGuid>& OutRuntimeItemUids) const;
	int32 ResolveLootLoadedAmmoCount(int32& OutSourceLoadedAmmoCount, int32& OutDeductedLoadedAmmoCount) const;
	void HandleDeath(AController* KillerController, AActor* DamageCauser);
	bool ApplyMeleeDamageTo(AActor* TargetActor);
	void ApplyMeleeKnockbackTo(AActor* TargetActor, const FVector& AttackDirection) const;
	void SpawnMeleeSwingEffect(const FVector& AttackDirection);
	void SpawnMeleeImpactBurst(const FVector& HitLocation, const FVector& BurstDirection);
	bool SpawnDeathLootContainer(AActor* DamageCauser);
	FVector ResolveLootDropLocation(AActor* IgnoredActor) const;
	void TickFootstepNoise(float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperWeapon> EnemyWeapon;

	float CurrentHealth = 30.0f;
	FName EnemyWeaponTypeTag = NAME_None;
	FName EnemyImpactProfileId = NAME_None;
	FName EnemyProjectileHitEffectId = NAME_None;
	float EnemyProjectileDamageMultiplier = 1.0f;
	float EnemyReloadSeconds = 1.8f;
	int32 EnemyProjectileDamageBonus = 0;
	int32 EnemyMagazineCapacity = 0;
	int32 EnemyLoadedAmmoCount = 0;
	int32 PendingEnemyReloadAmmoCount = 0;
	bool bIsDead = false;
	bool bEnemyWeaponRuntimeInitialized = false;
	float FootstepNoiseElapsedSeconds = 0.0f;
};
