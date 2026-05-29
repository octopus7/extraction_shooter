#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperShopActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperShopActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperShopActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Shop")
	int32 GetShopId() const { return ShopId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	void SetShopId(int32 InShopId);

	void ConfigureShopDefaults(int32 InShopId);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Shop", meta = (ClampMin = "1", UIMin = "1"))
	int32 ShopId = 1;
};
