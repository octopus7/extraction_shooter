#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperProjectile.generated.h"

class UProjectileMovementComponent;
class UPrimitiveComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperProjectile : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperProjectile();

	void SetDamageAmount(float InDamageAmount) { DamageAmount = FMath::Max(0.0f, InDamageAmount); }
	float GetDamageAmount() const { return DamageAmount; }
	void SetHitEffectId(FName InHitEffectId) { HitEffectId = InHitEffectId; }
	FName GetHitEffectId() const { return HitEffectId; }
	void SetAimIntent(
		AActor* InAimIntentActor,
		UPrimitiveComponent* InAimIntentComponent,
		const FVector& InAimIntentWorldPoint,
		bool bInHasAimIntentWorldPoint);
	bool IsAimIntentFor(const AActor* Actor) const { return Actor && AimIntentActor.Get() == Actor; }
	UPrimitiveComponent* GetAimIntentComponent() const { return AimIntentComponent.Get(); }
	const FVector& GetAimIntentWorldPoint() const { return AimIntentWorldPoint; }
	bool HasAimIntentWorldPoint() const { return bHasAimIntentWorldPoint; }
	void IgnoreActor(AActor* ActorToIgnore);
	void ApplyVisualMaterial(
		UMaterialInterface* Material,
		const FLinearColor& BaseColor,
		float EmissiveStrength);
	void ApplyTrailVisual(
		UMaterialInterface* Material,
		const FLinearColor& TrailColor,
		float EmissiveStrength,
		float TrailLengthCm,
		float TrailRadiusCm,
		float Opacity,
		float EndFade);
	void SetSpeedMultiplier(float InSpeedMultiplier);
	void SetCameraHitReactionScale(float InCameraHitReactionScale);
	float GetCameraHitReactionScale() const { return CameraHitReactionScale; }

protected:
	virtual void BeginPlay() override;

	void ApplyProjectileCollisionDefaults();
	void SpawnHitEffect(const FHitResult& Hit, AActor* OtherActor, UPrimitiveComponent* OtherComp) const;
	FVector ResolveHitEffectLocation(const FHitResult& Hit, AActor* OtherActor) const;

	UFUNCTION()
	void HandleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProceduralMeshComponent> TrailMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicVisualMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicTrailMaterial;

	UPROPERTY(Transient)
	TObjectPtr<AActor> AimIntentActor;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> AimIntentComponent;

	FVector AimIntentWorldPoint = FVector::ZeroVector;
	bool bHasAimIntentWorldPoint = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float LifeSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageAmount = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Camera", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CameraHitReactionScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Hit Effect")
	FName HitEffectId = NAME_None;
};
