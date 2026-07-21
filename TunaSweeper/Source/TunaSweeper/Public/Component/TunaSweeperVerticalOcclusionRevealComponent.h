#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TunaSweeperVerticalOcclusionRevealComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;

USTRUCT()
struct FVerticalOcclusionRevealMaterialState
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TWeakObjectPtr<UMeshComponent> MeshComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
};

/** Reveals the upper section of an occluder when the player or cursor approaches its bounds. */
UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperVerticalOcclusionRevealComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperVerticalOcclusionRevealComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vertical Occlusion Reveal")
	bool bAutoCollectOwnerMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vertical Occlusion Reveal")
	TArray<TObjectPtr<UMeshComponent>> RevealMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vertical Occlusion Reveal")
	bool bOverrideAllMaterialSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vertical Occlusion Reveal")
	bool bUsePlayerProximity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vertical Occlusion Reveal")
	bool bUseCursorProximity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vertical Occlusion Reveal", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float ProximityRadiusOverrideCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vertical Occlusion Reveal")
	TSoftObjectPtr<UMaterialInterface> VerticalRevealMaterial;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vertical Occlusion Reveal")
	void ApplyVerticalRevealMaterial();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vertical Occlusion Reveal")
	void RestoreOriginalMaterials();

private:
	void CollectRevealMeshes(TArray<UMeshComponent*>& OutMeshes) const;
	bool ResolveCursorWorldPoint(class APlayerController* PlayerController, float PlaneZ, FVector& OutCursorWorldPoint) const;
	bool IsPointNearOwnerBoundsXY(const FVector& Point, float RadiusCm) const;
	void UpdateRevealParameters();

	UPROPERTY(Transient)
	TArray<FVerticalOcclusionRevealMaterialState> MaterialStates;
};
