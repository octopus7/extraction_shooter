#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "TimerManager.h"
#include "TunaSweeperNpcFeedingComponent.generated.h"

/** Optional, quest-gated food exchange on a resident NPC. */
UCLASS(ClassGroup=(TunaSweeper), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperNpcFeedingComponent : public UTunaSweeperInteractableComponent
{
    GENERATED_BODY()
public:
    UTunaSweeperNpcFeedingComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feeding") int32 RequiredQuestId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feeding") TMap<int32, int32> FoodRewards;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feeding") FName NoFoodStringKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Feeding") FName HungryStringKey;
    bool IsUnlocked() const;
    bool TryFeed(APawn* Pawn);
    void RefreshAvailability();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    friend class FTunaSweeperFeedingRewardsTest;
    void ShowHungryBubble();
    void ShowBubble(AActor* Speaker, FName Key);
    FTimerHandle HungryTimer;
    bool bFeeding = false;
};
