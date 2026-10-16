#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "TunaSweeperRailInteractionComponent.generated.h"

class ALoopRailStation;
class ALoopRailTrain;
class APlayerController;
class UTunaSweeperScreenFadeWidget;

/** Project interactions mounted on the existing countdown monument, without coupling LoopRail to game data. */
UCLASS(ClassGroup=(TunaSweeper), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperRailInteractionComponent : public UTunaSweeperInteractableComponent
{
    GENERATED_BODY()
public:
    UTunaSweeperRailInteractionComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail") bool bTravelInteraction = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rail") int32 OilItemId = 6014;
    UFUNCTION(BlueprintPure, Category="Rail") bool IsLubricated() const;
    UFUNCTION(BlueprintPure, Category="Rail") bool IsTravelUnlocked() const;
    UFUNCTION(BlueprintPure, Category="Rail") ALoopRailStation* GetNextStation() const;
    UFUNCTION(BlueprintCallable, Category="Rail") void RefreshState();
    bool CanOffer() const;
    bool Interact(APawn* Pawn);
    bool Lubricate(bool bSaveImmediately = true);
    static FName ProgressId(const ALoopRailStation* Station);
private:
    friend class FTunaSweeperRailInteractionTest;
    friend class FTunaSweeperRailTravelTest;
    bool ResolveTravel(APawn* Pawn, ALoopRailStation*& Station, ALoopRailTrain*& Train, FVector& Location) const;
    bool ExecuteTravel(APawn* Pawn);
    bool StartTravel(APawn* Pawn);
    void CompleteFadeOut();
    void FinishTravel();
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperScreenFadeWidget> FadeWidget;
    TWeakObjectPtr<APawn> PendingPawn;
    TWeakObjectPtr<APlayerController> PendingController;
    bool bTravelling = false;
    bool bLubricating = false;
};
