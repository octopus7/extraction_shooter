#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tree")
	TObjectPtr<UStaticMeshComponent> Leaves;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tree")
	TObjectPtr<UStaticMeshComponent> ShadowProxy;

	/** Editor property changes refresh automatically. Call after runtime BP property changes. */
	UFUNCTION(BlueprintCallable, Category="Tree")
	void RefreshTree();
	UFUNCTION(BlueprintCallable, Category="Tree")
	void SetTreeParameters(float NewWindStrength, float NewLeafCardScale, float NewLeafDensity);

private:
	UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> LeafInstance;
	UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> ShadowInstance;
};
