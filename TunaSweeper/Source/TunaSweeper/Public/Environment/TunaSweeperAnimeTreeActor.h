#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "TunaSweeperAnimeTreeActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Web v06 card foliage: view-facing geometry with stable baked canopy shading. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperAnimeTreeActor : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperAnimeTreeActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree", meta=(ClampMin="0", ClampMax="1.5", UIMin="0", UIMax="1.5"))
	float WindStrength = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree", meta=(ClampMin="0.2", ClampMax="1.4", UIMin="0.2", UIMax="1.4"))
	float LeafCardScale = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree", meta=(ClampMin="0", ClampMax="1", UIMin="0", UIMax="1"))
	float LeafDensity = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tree")
	TObjectPtr<UMaterialInterface> LeafMaterial;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tree")
	TObjectPtr<UStaticMeshComponent> Trunk;
	/** Its local +X points toward the light color. Translation, rotation and X scale shape the shared ramp. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tree|Gradient")
	TObjectPtr<USceneComponent> GradientGuide;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree|Gradient", meta=(ClampMin="1", UIMin="50", UIMax="1000", Units="cm"))
	float GradientWidth = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree|Gradient", meta=(ClampMin="0", ClampMax="1"))
	float GradientStrength = .65f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree|Gradient", meta=(ClampMin="0.1", ClampMax="4"))
	float GradientExponent = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree|Gradient")
	FLinearColor GradientDarkColor = FLinearColor(.32f, .53f, .48f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree|Gradient")
	FLinearColor GradientLightColor = FLinearColor(1.f, 1.f, .83f);
	/** Blend between neutral leaf color and the clump's baked volume shading. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tree|Gradient", meta=(ClampMin="0", ClampMax="1"))
	float ClumpShadingStrength = .7f;

	/** Editor property changes refresh automatically. Call after runtime BP property changes. */
	UFUNCTION(BlueprintCallable, Category="Tree")
	void RefreshTree();
	UFUNCTION(BlueprintCallable, Category="Tree")
	void SetTreeParameters(float NewWindStrength, float NewLeafCardScale, float NewLeafDensity);
	/** Duplicate tagged BP components to add clumps; their transforms remain directly editable. */
	UFUNCTION(BlueprintPure, Category="Tree")
	TArray<UStaticMeshComponent*> GetLeafClumps() const;

private:
	void UpdateGradientData();
	void OnTreeComponentTransformUpdated(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport);
	bool bRefreshingTree = false;
	UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> LeafInstance;
	UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> ShadowInstance;
};
