#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateMaskWaterAssetsCommandlet.generated.h"
class AStylizedWaterBodyActor;
class UTexture2D;
// ONE_SHOT_WATER_ASSET_GENERATOR: remove immediately after the verified asset commit.
UCLASS()
class UGenerateMaskWaterAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGenerateMaskWaterAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
    UFUNCTION(BlueprintCallable, Category="One Shot")
    static UTexture2D* BakeActorMask(AStylizedWaterBodyActor* Body, const FString& PackagePath);
};
