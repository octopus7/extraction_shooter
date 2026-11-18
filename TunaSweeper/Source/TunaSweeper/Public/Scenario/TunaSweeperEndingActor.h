#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "UI/TunaSweeperDialogueWidget.h"
#include "TunaSweeperEndingActor.generated.h"

class UTunaSweeperEndingPresentation;
class UTunaSweeperScreenFadeWidget;
class UTunaSweeperGameInstance;
class ATunaSweeperPlayerController;
class ASSTOLaunchCinematic;
class UEndingMonologueGPT62Astra;

/** Repeatable ending interaction. Never retires or deletes a save slot. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperEndingActor : public ATunaSweeperInteractableActor
{
    GENERATED_BODY()
public:
    ATunaSweeperEndingActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintCallable, Category="Ending")
    bool StartEnding(APawn* InstigatorPawn);
    UFUNCTION(BlueprintCallable, Category="Ending")
    void RefreshEndingPrompt();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ending")
    TObjectPtr<UTunaSweeperEndingPresentation> NormalEnding;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ending")
    TObjectPtr<UTunaSweeperEndingPresentation> TrueEnding;
    /** The actual lander containing this terminal. Set on the level instance. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Ending|SSTO")
    TObjectPtr<AActor> LaunchLander;
    UPROPERTY(EditDefaultsOnly, Category="Ending|SSTO")
    TSoftClassPtr<UEndingMonologueGPT62Astra> MonologueClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending", meta=(ClampMin="0.05"))
    float FadeSeconds = 0.6f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Ending")
    bool bEndingActive = false;
private:
    void ShowStory();
    void StartLaunch();
    void FadeOutOfOrbit();
    void ShowMonologue();
    void RemoveLaunchStage();
    void FinishEnding();
    void ReleasePresentation();
    UPROPERTY(Transient) TObjectPtr<ATunaSweeperPlayerController> Player;
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperScreenFadeWidget> Fade;
    UPROPERTY(Transient) TObjectPtr<UTunaSweeperDialogueWidget> Dialogue;
    UPROPERTY(Transient) TObjectPtr<UEndingMonologueGPT62Astra> Monologue;
    UPROPERTY(Transient) TObjectPtr<ASSTOLaunchCinematic> LaunchStage;
    FText MonologueLine;
    TWeakObjectPtr<UTunaSweeperGameInstance> Game;
    TArray<FTunaSweeperDialogueLine> StoryLines;
    bool bTrueEndingSelected = false;
    bool bOwnsPause = false;
};
