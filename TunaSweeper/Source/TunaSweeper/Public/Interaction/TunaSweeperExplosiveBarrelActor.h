#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperExplosiveBarrelActor.generated.h"

class ATunaSweeperLocalExplosionEffectActor;
class UBoxComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** A mesh/material/smoke combination for one physical damage state. */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperExplosiveBarrelVisualState
{
	GENERATED_BODY()

	/** The mesh to use for this state. State 0 is intact and the final entry is the destroyed base. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Optional material override for mesh slot 0. Enabled only when bOverrideMaterial is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	TSoftObjectPtr<UMaterialInterface> Material;

	/** Leave false to use the material authored on the selected mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	bool bOverrideMaterial = false;

	/** Continuous smoke emitted while the barrel remains in this state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	TSoftObjectPtr<UNiagaraSystem> SmokeEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	FVector CollisionExtent = FVector(42.0f, 42.0f, 62.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	FVector SmokeRelativeLocation = FVector(0.0f, 0.0f, 54.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	FVector SmokeScale = FVector::OneVector;

	/** Multiplier for the persistent gas source. It is also used to reduce the source cleanly during a state transition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SmokeEmitterStrength = 1.0f;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperExplosiveBarrelActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperExplosiveBarrelActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
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

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Explosive Barrel")
	int32 GetDamageStage() const { return CurrentDamageStage; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Explosive Barrel")
	void SetDamageStageCount(int32 InDamageStageCount);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BarrelMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> DestroyedLoopEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> DamageSmokeEffectComponent;

	/** Default attached smoke/fire visual. It remains locked to the barrel even when world-space Niagara is disabled. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AttachedSmokeSprite;

	/** Editor-only ground decal that shows the damage radius when this actor is selected. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> DamageRadiusDecal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel")
	FName BarrelId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 30.0f;

	/** State 0 is intact, then weak/heavy damage states, with the final entry used after destruction. Add entries to support more damage stages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual", meta = (TitleProperty = "Mesh"))
	TArray<FTunaSweeperExplosiveBarrelVisualState> VisualStates;

	/** Number of non-lethal visible damage states. Defaults to weak and heavy damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Durability", meta = (ClampMin = "0", UIMin = "0"))
	int32 DamageStageCount = 2;

	/** Time spent in each damaged state before advancing automatically. Zero is invalid and is repaired to one second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Durability", meta = (ClampMin = "0.01", UIMin = "0.01", ForceUnits = "s"))
	float DamageStageAdvanceDelaySeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion")
	TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor> ExplosionEffectActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion")
	FVector ExplosionEffectOffset = FVector(0.0f, 0.0f, 62.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ExplosionVisualRadiusCm = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float ExplosionDurationSeconds = 0.72f;

	/** Damage applied at the centre of the explosion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionDamageStrong = 60.0f;

	/** Damage applied at the outer edge of the explosion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionDamageWeak = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Explosion", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionDamageInnerRadiusCm = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Editor Preview")
	TSoftObjectPtr<UMaterialInterface> DamageRadiusDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Destroyed Burning", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BurningDurationSeconds = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Destroyed Burning")
	FVector DestroyedLoopEffectRelativeLocation = FVector(0.0f, 0.0f, 14.0f);

	/** Use the state Smoke Effect on the root-attached Niagara component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual")
	bool bUseNiagaraStageSmoke = true;

	/** Time in seconds used to cross-fade the previous persistent smoke state into the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual", meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "s"))
	float StageSmokeCrossFadeSeconds = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive Barrel|Visual")
	TSoftObjectPtr<UMaterialInterface> AttachedSmokeMaterial;

private:
	void ApplyCollisionDefaults();
	void ApplyVisualState();
	void RefreshDamageSmokeEffect();
	void RefreshDestroyedLoopEffect();
	void RefreshAttachedSmokeVisual();
	void SetAttachedSmokeOpacity(float Opacity);
	void BeginStageSmokeCrossFade();
	void UpdateStageSmokeCrossFade(float DeltaSeconds);
	void SetStageSmokeIntensity(UNiagaraComponent* EffectComponent, float SmokeEmitterStrength, float Intensity) const;
	UNiagaraComponent* CreateIncomingStageSmokeComponent();
	void ApplyExplosionDamage();
	void UpdateDamageStageFromHealth();
	void ScheduleAutomaticDamageStageAdvance();
	void AdvanceDamageStageAutomatically();
	void ValidateDamageStageAdvanceDelay();
	int32 GetActiveVisualStateIndex() const;
	int32 GetSupportedDamageStageCount() const;
	void UpdateEditorDamageRadiusPreview();
	void DestroyBarrel();
	void SpawnExplosionEffect();

	UPROPERTY(Transient)
	float CurrentHealth = 30.0f;

	UPROPERTY(Transient)
	int32 CurrentDamageStage = 0;

	UPROPERTY(Transient)
	float BurningElapsedSeconds = 0.0f;

	FTimerHandle DamageStageAdvanceTimerHandle;

	UPROPERTY(Transient)
	bool bBarrelDestroyed = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AttachedSmokeDynamicMaterial;

	/** Retained components preserve their running Niagara simulation while their gas source is faded out. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> FadingStageSmokeEffectComponents;

	TArray<float> FadingStageSmokeElapsedSeconds;
	TArray<float> FadingStageSmokeStrengths;
	float IncomingStageSmokeElapsedSeconds = 0.0f;
	float IncomingStageSmokeStrength = 1.0f;
	bool bIncomingStageSmokeFading = false;
};
