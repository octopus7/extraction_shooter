#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperRequirementPreview.generated.h"

class UTexture2D;

/** One material in an interaction's requirement badge; quantities come from gameplay data. */
USTRUCT(BlueprintType)
struct FTunaSweeperRequirementPreview
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0;

	bool operator==(const FTunaSweeperRequirementPreview& Other) const
	{ return Icon == Other.Icon && Quantity == Other.Quantity; }
};
