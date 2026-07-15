#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperBreakableAppleCrateActor.h"
#include "TunaSweeperBreakableTomatoActor.generated.h"

class UNiagaraSystem;
class UPhysicalMaterial;
class UMaterialInterface;
class UTunaSweeperBreakableTomatoComponent;

/** A one-shot Chaos tomato. The hidden Geometry Collection is activated only when its health is depleted. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperBreakableTomatoActor : public ATunaSweeperBreakableAppleCrateActor
{
	GENERATED_BODY()

public:
	ATunaSweeperBreakableTomatoActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Breakable Tomato")
	void ConfigureBreakableTomatoDefaults(
		FName InTomatoId,
		float InMaxHealth,
		const TSoftObjectPtr<UStaticMesh>& InTomatoMesh,
		const TSoftObjectPtr<UGeometryCollection>& InTomatoGeometryCollection,
		const TSoftObjectPtr<UNiagaraSystem>& InStickySplatterSystem,
		const TSoftObjectPtr<UMaterialInterface>& InStickyGooDecalMaterial,
		const TSoftObjectPtr<UPhysicalMaterial>& InPhysicalMaterial,
		const FVector& InCollisionExtent,
		const FVector& InCollisionCenterOffset);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperBreakableTomatoComponent> BreakableTomatoComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Effect")
	TSoftObjectPtr<UNiagaraSystem> StickySplatterSystem;

	/** Tomato sprite burst rendered with the dedicated non-smoke particle material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Effect")
	bool bUseNiagaraStickySplatter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Effect")
	TSoftObjectPtr<UMaterialInterface> StickyGooMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Effect")
	TSoftObjectPtr<UMaterialInterface> StickyGooDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Collision")
	TSoftObjectPtr<UPhysicalMaterial> TomatoPhysicalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Cleanup", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DestroyedDebrisLifetime = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ActivationRadiusCm = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopDistanceCm = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HopMoveSpeedCmPerSecond = 135.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HopHeightCm = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Goo", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector2D GooRadiusRangeCm = FVector2D(5.0f, 13.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Goo", meta = (ClampMin = "0", UIMin = "0"))
	int32 MinGooSplatCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Goo", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxGooSplatCount = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Break", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TomatoImpactDirectionalImpulse = 1800.0f;

private:
	void ApplyTomatoCollisionMaterial();
	void SpawnStickySplatter(const FDamageEvent& DamageEvent);
	void SpawnStickyGooSplats(const FVector& ImpactLocation);
	FVector ResolveImpactLocation(const FDamageEvent& DamageEvent) const;
	FVector ResolveImpactDirection(const FDamageEvent& DamageEvent) const;
	FVector ResolveImpactNormal(const FDamageEvent& DamageEvent) const;
	void StartMovementSegment();
	void StartRestSegment();
	void UpdateHopMovement(float DeltaSeconds);
	void EnsureTomatoBreakSoundVariants();

	bool bMovingTowardCharacter = false;
	float SegmentRemainingSeconds = 0.0f;
	float HopElapsedSeconds = 0.0f;
	float InitialActorZ = 0.0f;
};
