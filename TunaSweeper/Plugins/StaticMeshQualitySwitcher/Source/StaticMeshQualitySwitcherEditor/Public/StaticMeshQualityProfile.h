#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Misc/DataValidation.h"
#include "StaticMeshQualityProfile.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType)
struct STATICMESHQUALITYSWITCHEREDITOR_API FStaticMeshQualityPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Pair")
	TSoftObjectPtr<UStaticMesh> OriginalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Pair")
	TSoftObjectPtr<UStaticMesh> LowMesh;
};

UCLASS(BlueprintType, DisplayName = "Static Mesh Quality Profile")
class STATICMESHQUALITYSWITCHEREDITOR_API UStaticMeshQualityProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh Quality")
	TArray<FStaticMeshQualityPair> MeshPairs;

	bool ValidateProfile(TArray<FText>& OutErrors) const;

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
};
