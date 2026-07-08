#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperThrowableGrenade.generated.h"

class UCameraShakeBase;
class UParticleSystem;
class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperThrowableGrenade : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperThrowableGrenade();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Throwable Grenade")
	void LaunchGrenade(const FVector& Direction, float Speed);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Throwable Grenade")
	void Explode();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Throwable Grenade|Effects")
	void SetExplosionEffects(
		UParticleSystem* InParticle,
		UNiagaraSystem* InNiagara,
		USoundBase* InSound,
		TSubclassOf<UCameraShakeBase> InCameraShake,
		float InCameraShakeRadius);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable Grenade")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable Grenade")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable Grenade")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Damage", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Damage = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Damage", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Timing", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FuseTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ThrowSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Effects")
	TObjectPtr<UParticleSystem> ExplosionParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Effects")
	TObjectPtr<UNiagaraSystem> ExplosionNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Effects")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Effects")
	TSubclassOf<UCameraShakeBase> ExplosionCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Effects", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CameraShakeRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable Grenade|Debug")
	bool bShowDebugExplosion = false;

private:
	FTimerHandle FuseTimerHandle;
	float FuseStartTime = 0.0f;
	bool bHasLanded = false;
	bool bHasExploded = false;
};
