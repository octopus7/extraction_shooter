#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "SogSplatTypes.h"
#include "SogSplatComponent.generated.h"

class UMaterialInterface;
class USogAsset;
struct FSogSplatRenderData;

UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent))
class SOGSPLAT_API USogSplatComponent : public UMeshComponent
{
	GENERATED_BODY()

public:
	USogSplatComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOG")
	TObjectPtr<USogAsset> SourceAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG|Rendering", meta = (ClampMin = "1"))
	int32 InstanceStride = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG|Rendering", meta = (ClampMin = "0"))
	int32 MaxRenderedInstances = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG|Rendering")
	TObjectPtr<UMaterialInterface> MaterialOverride;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 RenderedInstanceCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FText LastError;

	UFUNCTION(BlueprintCallable, Category = "SOG")
	void SetSogAsset(USogAsset* InAsset);

	UFUNCTION(BlueprintCallable, Category = "SOG")
	bool RebuildInstances();

	UFUNCTION(BlueprintCallable, Category = "SOG")
	bool LoadSogFile(const FString& FilePath, FSogDecodeOptions Options);

	virtual void OnRegister() override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual int32 GetNumMaterials() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyDefaultMaterial();
	UMaterialInterface* ResolveMaterial() const;

	bool bInstancesBuilt = false;
	FBoxSphereBounds CachedLocalBounds = FBoxSphereBounds(EForceInit::ForceInit);
	TSharedPtr<FSogSplatRenderData, ESPMode::ThreadSafe> RenderData;
};
