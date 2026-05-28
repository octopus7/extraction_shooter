#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TunaSweeperEnemyCharacter.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UNiagaraSystem;
class ATunaSweeperProjectile;
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

	void ConfigureSpawnData(
		const TSoftObjectPtr<UMaterialInterface>& InBodyMaterial,
		FName InEnemyId,
		int32 InDropContainerDefinitionId,
		int32 InDropContentsId,
		float InMaxHealth,
		int32 InExperienceValue);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ForwardMarkerMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<ATunaSweeperProjectile> ProjectileClass;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MovementSpeed = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Randomization")
	FVector2D MovementSpeedRandomOffset = FVector2D(-35.0f, 45.0f);

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

private:
	void ApplyVoxelVisualMeshes();
	void ApplyVisualMaterials();
	void HandleDeath(AController* KillerController, AActor* DamageCauser);
	bool ApplyMeleeDamageTo(AActor* TargetActor);
	void ApplyMeleeKnockbackTo(AActor* TargetActor, const FVector& AttackDirection) const;
	void SpawnMeleeSwingEffect(const FVector& AttackDirection);
	void SpawnMeleeImpactBurst(const FVector& HitLocation, const FVector& BurstDirection);
	bool SpawnDeathLootContainer(AActor* DamageCauser);
	FVector ResolveLootDropLocation(AActor* IgnoredActor) const;

	float CurrentHealth = 30.0f;
	bool bIsDead = false;
};
