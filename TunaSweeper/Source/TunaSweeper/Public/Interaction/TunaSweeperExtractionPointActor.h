#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperExtractionPointActor.generated.h"

class APawn;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMediaSource;
class UNiagaraComponent;
class UNiagaraSystem;
class UProceduralMeshComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTunaSweeperLevelTransitionWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperExtractionPointActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperExtractionPointActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Extraction")
	void ConfigureExtractionPointDefaults(
		FName InTargetLevelName,
		float InExtractionRadius,
		float InExtractionHoldSeconds,
		float InRadiusRingWidth,
		TSoftObjectPtr<UNiagaraSystem> InExtractionParticleSystem,
		TSoftObjectPtr<UMaterialInterface> InRadiusVisualMaterial,
		TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
		TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass,
		const FText& InTransitionMessage,
		FName InTransitionMessageStringKey = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Extraction|Effect")
	void SetSmokeSignalWind(FVector2D InWindDirection, float InWindSpeedCmPerSecond);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Extraction")
	float GetExtractionRadius() const { return ExtractionRadius; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Extraction")
	float GetExtractionHoldSeconds() const { return ExtractionHoldSeconds; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Extraction")
	float GetCurrentHoldSeconds() const { return CurrentHoldSeconds; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Extraction")
	float GetCurrentHoldProgress() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Extraction")
	bool ExtractPawn(APawn* InstigatorPawn);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> ExtractionArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> RadiusVisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> ExtractionEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> FallbackParticleMeshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SmokeSignalSprites;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ExtractionRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float ExtractionHoldSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	FName TargetLevelName = FName(TEXT("BunkerMap"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Visual", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float RadiusRingWidth = 4.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Visual", meta = (ClampMin = "12", ClampMax = "256", UIMin = "12", UIMax = "256"))
	int32 RadiusVisualSegments = 96;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Visual")
	float RadiusVisualZOffset = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Visual")
	FLinearColor RadiusVisualColor = FLinearColor(0.05f, 0.95f, 0.24f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Visual")
	TSoftObjectPtr<UMaterialInterface> RadiusVisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect")
	TSoftObjectPtr<UNiagaraSystem> ExtractionParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Niagara")
	FVector ExtractionNiagaraRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Niagara")
	FRotator ExtractionNiagaraRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Niagara")
	FVector ExtractionNiagaraRelativeScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Niagara")
	FVector ExtractionNiagaraSourceOffset = FVector(0.0f, 0.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Niagara")
	FVector ExtractionNiagaraSourceNonUniformScale = FVector(2.8f, 1.45f, 0.16f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Niagara", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExtractionNiagaraSourceUpVelocity = 185.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Niagara")
	bool bHideExtractionNiagaraDebugBounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect")
	bool bEnableFallbackParticleEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect")
	FLinearColor ParticleColor = FLinearColor(0.08f, 1.0f, 0.28f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FallbackParticleOrbitRadius = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FallbackParticleBaseHeight = 54.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FallbackParticleVerticalAmplitude = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal")
	bool bEnableSmokeSignalEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal")
	TSoftObjectPtr<UMaterialInterface> SmokeSignalSpriteMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal")
	FLinearColor SmokeSignalBaseColor = FLinearColor(0.02f, 1.0f, 0.18f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal")
	FLinearColor SmokeSignalTopColor = FLinearColor(0.035f, 0.04f, 0.035f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SmokeSignalBaseHeight = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SmokeSignalColumnHeight = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SmokeSignalBaseDiameter = 54.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SmokeSignalTopDiameter = 265.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal", meta = (ClampMin = "0.25", UIMin = "0.25"))
	float SmokeSignalLoopDurationSeconds = 4.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal")
	FVector2D SmokeSignalWindDirection = FVector2D(0.92f, 0.34f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SmokeSignalWindSpeedCmPerSecond = 54.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Effect|Smoke Signal", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SmokeSignalHorizontalSpread = 62.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition")
	TSoftObjectPtr<UMediaSource> TransitionMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition")
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> TransitionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition")
	FText TransitionMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition")
	FName TransitionMessageStringKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeToBlackDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeFromBlackDuration = 0.2f;

private:
	void RefreshExtractionComponents();
	void RebuildRadiusVisualMesh();
	void ApplyRadiusVisualMaterial();
	void RefreshEffectComponent();
	void ApplyExtractionNiagaraParameters();
	void UpdateExtractionProgress(float DeltaSeconds);
	void UpdateHudProgressWidget();
	void ResetHoldProgress();
	bool IsPawnInsideExtractionArea(const APawn* Pawn) const;
	bool CanExtractPawn(const APawn* Pawn) const;
	void StopPawnForExtraction(APawn* Pawn) const;
	FText ResolveTransitionMessage() const;
	void UpdateFallbackParticleEffect(float DeltaSeconds);
	void ApplyFallbackParticleMaterials();
	void UpdateSmokeSignalEffect(float DeltaSeconds);
	void ApplySmokeSignalMaterials();
	void UpdateSmokeSignalSpriteMaterial(
		UMaterialInstanceDynamic* DynamicMaterial,
		int32 FrameIndex,
		const FLinearColor& TintColor,
		float EmissiveStrength,
		float Opacity) const;
	FVector GetSmokeSignalWindVelocity() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RadiusVisualDynamicMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FallbackParticleDynamicMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SmokeSignalDynamicMaterials;

	float CurrentHoldSeconds = 0.0f;
	float EffectElapsedSeconds = 0.0f;
	bool bHasActiveNiagaraExtractionEffect = false;
	bool bExtractionTriggered = false;
};
