#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "TunaSweeperRaidStorageActor.generated.h"

// Presentation lives in BP_RaidStorage. No runtime mesh generation or lights.
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperRaidStorageActor : public ATunaSweeperLootContainerActor
{
    GENERATED_BODY()
public:
    ATunaSweeperRaidStorageActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool OpenRuntimeContainer(UTunaSweeperGameInstance* Game, FTunaSweeperLootContainerInstance& OutInstance) override;

    // BP static mesh components carry RaidStorageLED_0 .. RaidStorageLED_9 tags.
    UFUNCTION(BlueprintCallable, Category = "Raid Storage")
    void RefreshIndicators();

    UFUNCTION(BlueprintPure, Category = "Raid Storage")
    FLinearColor GetSlotIndicatorColor(int32 ItemId, int32 Quantity) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Raid Storage|Indicators")
    FLinearColor FullTunaColor = FLinearColor(0.01f, 1.0f, 0.005f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Raid Storage|Indicators", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PartialTunaBrightness = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Raid Storage|Indicators")
    FLinearColor OtherItemColor = FLinearColor(1.0f, 0.18f, 0.0f);

    int32 CountLoadedStorageActors() const;

private:
    void HandleContainerClosed();
    TWeakObjectPtr<UTunaSweeperGameInstance> StorageGame;
};
