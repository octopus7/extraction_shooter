#pragma once

#include "CoreMinimal.h"
#include "SogSplatTypes.generated.h"

USTRUCT(BlueprintType)
struct FSogDecodeOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG", meta = (ClampMin = "0.001"))
	float UnitsPerSogUnit = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG", meta = (ClampMin = "0.01"))
	float GaussianRadiusMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG", meta = (ClampMin = "0.0"))
	float MinCardDiameterCm = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG", meta = (ClampMin = "1"))
	int32 ImportStride = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG", meta = (ClampMin = "0"))
	int32 MaxImportedSplats = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOG")
	bool bConvertGammaColorToLinear = true;
};

USTRUCT(BlueprintType)
struct FSogSplatInstance
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FLinearColor Color = FLinearColor::White;
};
