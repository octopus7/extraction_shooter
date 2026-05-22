#pragma once

#include "CoreMinimal.h"
#include "SogSplatTypes.h"
#include "UObject/Object.h"
#include "SogAsset.generated.h"

class UMaterialInterface;

UCLASS(BlueprintType)
class SOGSPLAT_API USogAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FString SourceFilePath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 SourceGaussianCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 ImportedSplatCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FSogDecodeOptions DecodeOptions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FBoxSphereBounds LocalBounds = FBoxSphereBounds(EForceInit::ForceInit);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOG|Rendering")
	TSoftObjectPtr<UMaterialInterface> DefaultMaterial;

	UFUNCTION(BlueprintPure, Category = "SOG")
	int32 GetSplatCount() const;

	const TArray<FSogSplatInstance>& GetSplats() const;
	void SetSplats(TArray<FSogSplatInstance>&& InSplats);
	void ClearSplats();

private:
	UPROPERTY()
	TArray<FSogSplatInstance> Splats;
};
