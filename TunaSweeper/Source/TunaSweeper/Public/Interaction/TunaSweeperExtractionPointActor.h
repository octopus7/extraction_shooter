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
class UTunaSweeperExtractionProgressWidget;
class UTunaSweeperLevelTransitionWidget;
class UWidgetComponent;

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
		TSoftClassPtr<UTunaSweeperExtractionProgressWidget> InProgressWidgetClass,
		TSoftObjectPtr<UNiagaraSystem> InExtractionParticleSystem,
		TSoftObjectPtr<UMaterialInterface> InRadiusVisualMaterial,
		TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
		TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass,
		const FText& InTransitionMessage);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> ExtractionArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> RadiusVisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> ExtractionEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> ProgressWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> FallbackParticleMeshes;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Progress")
	TSoftClassPtr<UTunaSweeperExtractionProgressWidget> ProgressWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Progress")
	FVector2D ProgressWidgetDrawSize = FVector2D(180.0f, 36.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Progress")
	float ProgressWidgetHeightOffset = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Progress")
	bool bShowProgressWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition")
	TSoftObjectPtr<UMediaSource> TransitionMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition")
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> TransitionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition")
	FText TransitionMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeToBlackDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction|Transition", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeFromBlackDuration = 0.2f;

private:
	void RefreshExtractionComponents();
	void RebuildRadiusVisualMesh();
	void ApplyRadiusVisualMaterial();
	void RefreshEffectComponent();
	void RefreshProgressWidgetComponent();
	void UpdateExtractionProgress(float DeltaSeconds);
	void UpdateProgressWidget();
	void ResetHoldProgress();
	bool IsPawnInsideExtractionArea(const APawn* Pawn) const;
	bool CanExtractPawn(const APawn* Pawn) const;
	void StopPawnForExtraction(APawn* Pawn) const;
	void UpdateFallbackParticleEffect(float DeltaSeconds);
	void ApplyFallbackParticleMaterials();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RadiusVisualDynamicMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FallbackParticleDynamicMaterials;

	float CurrentHoldSeconds = 0.0f;
	float EffectElapsedSeconds = 0.0f;
	bool bExtractionTriggered = false;
};
