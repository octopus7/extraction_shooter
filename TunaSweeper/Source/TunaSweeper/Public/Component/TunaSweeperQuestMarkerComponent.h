#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "TunaSweeperQuestMarkerComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

/** World-space medal visual. Owning NPC remains responsible for quest-state visibility. */
UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperQuestMarkerComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperQuestMarkerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest Marker")
	void SetMarkerHeight(float InHeight);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest Marker")
	void RebuildMarker();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Layout", meta = (ClampMin = "1.0", UIMin = "20.0", UIMax = "80.0"))
	float MarkerDiameter = 44.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Layout", meta = (ClampMin = "0.2", UIMin = "1.0", UIMax = "8.0"))
	float MarkerThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Layout", meta = (ClampMin = "0.0", UIMin = "100.0", UIMax = "300.0"))
	float MarkerHeight = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Layout", meta = (ClampMin = "0.1", ClampMax = "0.98"))
	float FaceDiameterRatio = 0.76f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Layout", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float HaloDiameterScale = 1.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Layout", meta = (ClampMin = "3.0", UIMin = "12.0", UIMax = "30.0"))
	float ExclamationHeight = 21.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Layout", meta = (ClampMin = "1.0", UIMin = "3.0", UIMax = "9.0"))
	float ExclamationWidth = 5.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Animation", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "8.0"))
	float BobbingAmplitude = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Animation", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.5"))
	float BobbingSpeed = 0.42f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Glow", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "8.0"))
	float GlowStrength = 1.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Glow", meta = (ClampMin = "0.0", ClampMax = "0.35"))
	float GlowVariationAmount = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Glow", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5"))
	float GlowVariationSpeed = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Color")
	FLinearColor GoldColor = FLinearColor(1.0f, 0.58f, 0.055f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Color")
	FLinearColor CreamRimColor = FLinearColor(1.0f, 0.91f, 0.68f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Color")
	FLinearColor ExclamationColor = FLinearColor(0.20f, 0.075f, 0.025f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Marker|Color")
	FLinearColor GlowColor = FLinearColor(1.0f, 0.55f, 0.035f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest Marker|Material")
	TObjectPtr<UMaterialInterface> SurfaceMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest Marker|Material")
	TObjectPtr<UMaterialInterface> HaloMaterial;

private:
	void CreateMarkerMaterials();
	void UpdateMaterialParameters(float AnimatedGlowStrength);
	void FaceLocalPlayerCamera();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GoldMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RimMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ExclamationMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HaloMaterialInstance;

	float AnimationTime = 0.0f;
	float AnimationPhase = 0.0f;
};
