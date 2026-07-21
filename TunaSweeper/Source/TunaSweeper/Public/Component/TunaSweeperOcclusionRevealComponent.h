#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TunaSweeperOcclusionRevealComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;

/** Makes selected owner meshes use the shared character/cursor occlusion-reveal material. */
UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperOcclusionRevealComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperOcclusionRevealComponent();
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void ApplyOcclusionRevealMaterial();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void RestoreOriginalMaterials();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void RegisterRevealMesh(UMeshComponent* MeshComponent);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	bool bAutoCollectOwnerMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	TArray<TObjectPtr<UMeshComponent>> RevealMeshes;

	/** Replaces all material slots so any Static/Skeletal/Instanced mesh can use the reveal without material edits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	bool bOverrideAllMaterialSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	TSoftObjectPtr<UMaterialInterface> RevealMaterial;

private:
	struct FOriginalMaterialState
	{
		TWeakObjectPtr<UMeshComponent> MeshComponent;
		TArray<TObjectPtr<UMaterialInterface>> Materials;
	};

	void CollectRevealMeshes(TArray<UMeshComponent*>& OutMeshes) const;
	TArray<FOriginalMaterialState> OriginalMaterialStates;
};
