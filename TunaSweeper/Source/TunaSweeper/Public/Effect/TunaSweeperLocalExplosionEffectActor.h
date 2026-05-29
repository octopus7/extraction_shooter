#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperLocalExplosionEffectActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DistortionSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ShockwaveSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GroundSmokeSpriteA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GroundSmokeSpriteB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GroundSmokeSpriteC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GroundSmokeSpriteD;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GroundSmokeSpriteE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GroundSmokeSpriteF;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FireSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SmokeSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SmokeOffsetSpriteA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SmokeOffsetSpriteB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FlashLight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Effect")
	TSoftObjectPtr<UMaterialInterface> ExplosionFlipbookMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Effect")
	TSoftObjectPtr<UMaterialInterface> ExplosionDistortionMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Effect")
	TSoftObjectPtr<UMaterialInterface> ExplosionSmokeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "1", UIMin = "1"))
	int32 FlipbookColumns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "1", UIMin = "1"))
	int32 FlipbookRows = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "1", UIMin = "1"))
	int32 FlipbookFrameCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float TotalDurationSeconds = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ResidualSmokeDurationMultiplier = 4.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float EffectRadiusCm = 210.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SpriteBaseHeightCm = 36.0f;

private:
	void ConfigureSpriteComponent(UStaticMeshComponent* SpriteComponent, int32 SortPriority);
	void ApplyDynamicMaterials();
	void UpdateSpriteFrame(UMaterialInstanceDynamic* DynamicMaterial, int32 FrameIndex) const;
	void UpdateSpriteMaterial(
		UMaterialInstanceDynamic* DynamicMaterial,
		int32 FrameIndex,
		const FLinearColor& TintColor,
		float EmissiveStrength,
		float Opacity) const;
	void UpdateDistortionMaterial(float WavePosition, float WaveWidth, float DistortionStrength, float RefractionAmount, float Opacity) const;
	void SetSpriteDiameter(UStaticMeshComponent* SpriteComponent, float DiameterCm) const;
	void UpdateEffect(float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DistortionDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ShockwaveDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundSmokeDynamicMaterialA;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundSmokeDynamicMaterialB;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundSmokeDynamicMaterialC;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundSmokeDynamicMaterialD;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundSmokeDynamicMaterialE;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundSmokeDynamicMaterialF;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FireDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SmokeDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SmokeOffsetDynamicMaterialA;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SmokeOffsetDynamicMaterialB;

	float ElapsedSeconds = 0.0f;
};
