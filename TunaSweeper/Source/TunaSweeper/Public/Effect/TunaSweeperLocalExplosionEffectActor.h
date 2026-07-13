#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperLocalExplosionEffectActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTunaSweeperVisionSubjectComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLocalExplosionEffectActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLocalExplosionEffectActor();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Effect")
	void ConfigureExplosion(float InRadiusCm, float InDurationSeconds);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Retained only for the screen-space refraction ring. All fire/smoke cards were removed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DistortionSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVisionSubjectComponent> VisionSubjectComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Effect")
	TSoftObjectPtr<UMaterialInterface> ExplosionDistortionMaterial;

	/** One-shot Niagara fireball. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Effect|Niagara")
	TSoftObjectPtr<UNiagaraSystem> FireBurstNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float TotalDurationSeconds = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float EffectRadiusCm = 210.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpriteBaseHeightCm = 36.0f;

private:
	void ConfigureDistortionComponent();
	void ApplyDynamicMaterials();
	void UpdateDistortionMaterial(float WavePosition, float WaveWidth, float DistortionStrength, float RefractionAmount, float Opacity) const;
	void SetSpriteDiameter(float DiameterCm) const;
	void SpawnNiagaraBurstEffect();
	void UpdateEffect(float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DistortionDynamicMaterial;

	float ElapsedSeconds = 0.0f;
};
