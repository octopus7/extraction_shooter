#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Impact/TunaSweeperImpactEffectTypes.h"
#include "TunaSweeperImpactEffectProfile.generated.h"

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperImpactEffectProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UTunaSweeperImpactEffectProfile();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	FTunaSweeperImpactEffectSpec DefaultEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	TArray<FTunaSweeperImpactTagRule> TagRules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Impact")
	TArray<FTunaSweeperImpactSurfaceRule> SurfaceRules;
};
