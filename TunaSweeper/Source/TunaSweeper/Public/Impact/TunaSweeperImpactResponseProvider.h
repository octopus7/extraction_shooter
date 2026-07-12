#pragma once

#include "CoreMinimal.h"
#include "Impact/TunaSweeperImpactEffectTypes.h"
#include "UObject/Interface.h"
#include "TunaSweeperImpactResponseProvider.generated.h"

UINTERFACE(BlueprintType)
class TUNASWEEPER_API UTunaSweeperImpactResponseProvider : public UInterface
{
	GENERATED_BODY()
};

class TUNASWEEPER_API ITunaSweeperImpactResponseProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TunaSweeper|Impact")
	FTunaSweeperImpactTargetResponse GetImpactResponse(const FHitResult& Hit) const;
};
