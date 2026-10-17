#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "TunaSweeperNpcBarterComponent.generated.h"

/** Repeatable carried-inventory exchange, unlocked by a claimed quest reward. */
UCLASS(ClassGroup=(TunaSweeper), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperNpcBarterComponent : public UTunaSweeperInteractableComponent
{
    GENERATED_BODY()
public:
    UTunaSweeperNpcBarterComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter") int32 RequiredQuestId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter") int32 InputItemId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter", meta=(ClampMin="1")) int32 InputQuantity = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter") int32 OutputItemId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter", meta=(ClampMin="1")) int32 OutputQuantity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter") FName OfferStringKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter") FName ShortageStringKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Barter") FName FailureStringKey;
    bool IsUnlocked() const;
    bool TryBarter(APawn* Pawn, bool bSaveImmediately = true);
    void RefreshAvailability();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void ShowBubble(FName Key);
    bool bExchanging = false;
};
