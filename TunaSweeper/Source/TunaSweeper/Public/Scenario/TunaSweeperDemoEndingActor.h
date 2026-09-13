#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/TunaSweeperDialogueWidget.h"
#include "TunaSweeperDemoEndingActor.generated.h"
class UCameraComponent;
class USceneComponent;
class UTunaSweeperScreenFadeWidget;
class UTunaSweeperDemoFarewellWidget;
class ATunaSweeperPlayerController;
class ATunaSweeperMoleCompanionActor;
class UTexture2D;
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperDemoEndingActor : public AActor
{
    GENERATED_BODY()
public:
    ATunaSweeperDemoEndingActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    static ATunaSweeperDemoEndingActor* Find(UWorld* World);
    static bool TryDeliverToMole(APawn* Pawn);
    void QueueEnding();
    UFUNCTION(BlueprintCallable, Category="Demo Ending")
    bool StartEnding();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<USceneComponent> LunaPosition;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<USceneComponent> MolePosition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Demo Ending")
    TArray<FTunaSweeperDialogueLine> DinnerDialogue;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Demo Ending")
    TSoftObjectPtr<UTexture2D> FarewellIllustration;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Demo Ending", meta=(ClampMin="0.05"))
    float FadeSeconds = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Demo Ending", meta=(ClampMin="0.0"))
    float FarewellBgmFadeOutSeconds = 1.5f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Demo Ending")
    bool bEndingActive = false;
private:
    void ResumePendingEnding();
    void ShowDinner();
    void StartDinnerDialogue();
    void DinnerFinished();
    void ShowFarewell();
    void ReturnToTitle();
    void OpenTitle();
    void RestoreActors();
    UPROPERTY(Transient) TObjectPtr<ATunaSweeperPlayerController> Player;
    UPROPERTY(Transient) TObjectPtr<ATunaSweeperMoleCompanionActor> Mole;
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperScreenFadeWidget> Fade;
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperDialogueWidget> Dialogue;
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperDemoFarewellWidget> Farewell;
    FTransform PreviousLuna, PreviousMole;
    bool bActorsMoved = false;
    bool bMoleTickEnabled = true;
    FTimerHandle StageTimer;
};
