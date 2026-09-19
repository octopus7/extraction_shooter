#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "TunaSweeperATVPhysicsCommandlet.generated.h"
/** One-off asset authoring. Removed immediately after the asset commit. */
UCLASS()
class UTunaSweeperATVPhysicsCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	virtual int32 Main(const FString& Params) override;
	UFUNCTION(BlueprintCallable, Category="OneOff")
	int32 GenerateAsset() { return Main(FString()); }
};
