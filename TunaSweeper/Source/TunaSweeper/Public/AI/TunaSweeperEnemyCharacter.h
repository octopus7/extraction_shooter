#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TunaSweeperEnemyCharacter.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class ATunaSweeperProjectile;
class ATunaSweeperLootContainerActor;

UENUM(BlueprintType)
enum class ETunaSweeperEnemyAttackMode : uint8
{
	Projectile UMETA(DisplayName = "Projectile"),
	Melee UMETA(DisplayName = "Melee")
};

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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat")
	bool FireProjectileAt(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat")
	bool AttackTarget(AActor* TargetActor);

	void ConfigureSpawnData(
		const TSoftObjectPtr<UMaterialInterface>& InBodyMaterial,
		int32 InDropContainerDefinitionId,
		int32 InDropContentsId,
		float InMaxHealth);

	void ConfigureAttackData(
		ETunaSweeperEnemyAttackMode InAttackMode,
		float InAttackDamage,
		float InAttackRange,
		float InApproachStartRange,
		float InApproachStopRange,
		float InTrackingRange,
		float InAttackCooldownSeconds);

	ETunaSweeperEnemyAttackMode GetAttackMode() const { return AttackMode; }
	float GetMeleeAttackRange() const { return MeleeAttackRange; }
	float GetMeleeApproachStartRange() const { return MeleeApproachStartRange; }
	float GetMeleeApproachStopRange() const { return MeleeApproachStopRange; }
	float GetMeleeTrackingRange() const { return MeleeTrackingRange; }
	float GetMeleeAttackCooldownSeconds() const { return MeleeAttackCooldownSeconds; }

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
	ETunaSweeperEnemyAttackMode AttackMode = ETunaSweeperEnemyAttackMode::Projectile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MeleeAttackRange = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeApproachStartRange = 130.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeApproachStopRange = 95.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MeleeTrackingRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float MeleeAttackCooldownSeconds = 1.25f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 DropContainerDefinitionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 DropContentsId = INDEX_NONE;

private:
	void ApplyVisualMaterials();
	void HandleDeath(AActor* DamageCauser);
	bool ApplyMeleeDamageTo(AActor* TargetActor);
	bool SpawnDeathLootContainer(AActor* DamageCauser);
	FVector ResolveLootDropLocation(AActor* IgnoredActor) const;

	float CurrentHealth = 30.0f;
	bool bIsDead = false;
};
