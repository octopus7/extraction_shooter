#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperExplosiveBarrelActor.generated.h"

class ATunaSweeperLocalExplosionEffectActor;
class UBoxComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperExplosiveBarrelActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperExplosiveBarrelActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	void ConfigureExplosiveBarrelDefaults(
		FName InBarrelId,
		float InMaxHealth,
		const TSoftObjectPtr<UStaticMesh>& InIntactMesh,
		const TSoftObjectPtr<UStaticMesh>& InDestroyedMesh,
		const TSoftObjectPtr<UNiagaraSystem>& InDestroyedLoopEffect,
		const TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>& InExplosionEffectActorClass,
		float InExplosionVisualRadiusCm,
		float InExplosionDurationSeconds);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Explosive Barrel")
	FName GetBarrelId() const { return BarrelId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Explosive Barrel")
	float GetHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Explosive Barrel")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Explosive Barrel")
	bool IsBarrelDestroyed() const { return bBarrelDestroyed; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BarrelMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> DestroyedLoopEffectComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	FName BarrelId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual")
	TSoftObjectPtr<UStaticMesh> IntactBarrelMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual")
	TSoftObjectPtr<UStaticMesh> DestroyedBarrelMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual")
	TSoftObjectPtr<UMaterialInterface> IntactMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual")
	TSoftObjectPtr<UMaterialInterface> DestroyedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Collision", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector IntactCollisionExtent = FVector(42.0f, 42.0f, 62.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Collision", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector DestroyedCollisionExtent = FVector(42.0f, 42.0f, 18.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion")
	TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor> ExplosionEffectActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion")
	FVector ExplosionEffectOffset = FVector(0.0f, 0.0f, 62.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ExplosionVisualRadiusCm = 210.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float ExplosionDurationSeconds = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Destroyed Loop")
	TSoftObjectPtr<UNiagaraSystem> DestroyedLoopEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Destroyed Loop")
	FVector DestroyedLoopEffectRelativeLocation = FVector(0.0f, 0.0f, 28.0f);

private:
	void ApplyCollisionDefaults();
	void ApplyVisualState();
	void RefreshDestroyedLoopEffect();
	void DestroyBarrel();
	void SpawnExplosionEffect();

	UPROPERTY(Transient)
	float CurrentHealth = 30.0f;

	UPROPERTY(Transient)
	bool bBarrelDestroyed = false;
};
