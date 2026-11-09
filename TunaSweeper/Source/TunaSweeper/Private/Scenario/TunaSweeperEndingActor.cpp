#include "Scenario/TunaSweeperEndingActor.h"
#include "Scenario/TunaSweeperEndingPresentation.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"

ATunaSweeperEndingActor::ATunaSweeperEndingActor()
{
    ConfigureInteractionDefaults(ETunaSweeperInteractionType::Ending, FText::GetEmpty(),
        TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
        TEXT("ui.freeplay_ending.normal_prompt"));
    VisualMesh->SetRelativeLocation(FVector(0, 0, 65));
    VisualMesh->SetRelativeScale3D(FVector(.45, .8, 1.3));
    InteractableComponent->SetRelativeLocation(FVector(0, 0, 150));
}

void ATunaSweeperEndingActor::BeginPlay()
{
    Super::BeginPlay();
    Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (Game.IsValid()) Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshEndingPrompt);
    RefreshEndingPrompt();
}

void ATunaSweeperEndingActor::RefreshEndingPrompt()
{
    auto* Instance = GetGameInstance<UTunaSweeperGameInstance>();
    InteractableComponent->SetInteractionDisplayNameStringKey(Instance && Instance->IsRaidStorageReadyForTrueEnding()
        ? TEXT("ui.freeplay_ending.true_prompt") : TEXT("ui.freeplay_ending.normal_prompt"));
}

bool ATunaSweeperEndingActor::StartEnding(APawn* InstigatorPawn)
{
    auto* Instance = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Controller = InstigatorPawn ? Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController()) : nullptr;
    const auto* Character = Cast<ATunaSweeperTopDownCharacter>(InstigatorPawn);
    if (bEndingActive || !Instance || Instance->IsCombatTestSession() || !Controller ||
        !Controller->IsLocalController() || Controller->IsDialogueSequenceActive() ||
        UGameplayStatics::IsGamePaused(this) || (Character && Character->IsDead())) return false;
    bTrueEndingSelected = Instance->IsRaidStorageReadyForTrueEnding();
    const auto* Presentation = bTrueEndingSelected ? TrueEnding.Get() : NormalEnding.Get();
    if (!Presentation || !Presentation->ResolveDialogue(Instance, StoryLines)) return false;
    // Preflight writes the current state without marking an unseen ending completed.
    if (!Instance->SaveForGameplayExit(false))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 8, FColor::Red,
            Instance->ResolveLocalizedText(TEXT("ui.toast.save_failed"), FText::GetEmpty()).ToString());
        return false;
    }
    Player = Controller;
    Game = Instance;
    Fade = CreateWidget<UTunaSweeperScreenFadeWidget>(Player);
    Dialogue = CreateWidget<UTunaSweeperDialogueWidget>(Player);
    if (!Fade || !Dialogue) { ReleasePresentation(); return false; }
    bOwnsPause = Player->SetPause(true);
    if (!bOwnsPause) { ReleasePresentation(); return false; }
    bEndingActive = true;
    if (auto* MovingCharacter = Cast<ACharacter>(InstigatorPawn)) MovingCharacter->GetCharacterMovement()->StopMovementImmediately();
    Player->SetDemoEndingInputLock(true);
    Player->SetInputMode(FInputModeUIOnly());
    Player->bShowMouseCursor = true;
    Fade->AddToViewport(1000);
    Fade->StartFadeToBlack(FadeSeconds, FSimpleDelegate::CreateUObject(this, &ThisClass::ShowStory));
    return true;
}

void ATunaSweeperEndingActor::ShowStory()
{
    if (!IsValid(Player) || !Game.IsValid() || !Dialogue) { ReleasePresentation(); return; }
    Dialogue->SetFinishedDelegate(FSimpleDelegate::CreateUObject(this, &ThisClass::FinishEnding));
    Dialogue->AddToViewport(1100);
    Dialogue->StartDialogue(StoryLines, Game->GetDialogueCharactersPerSecond());
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(Dialogue->TakeWidget());
    Player->SetInputMode(Input);
    Dialogue->SetUserFocus(Player);
    Dialogue->SetKeyboardFocus();
}

void ATunaSweeperEndingActor::FinishEnding()
{
    if (!bEndingActive || !Game.IsValid()) { ReleasePresentation(); return; }
    if (!Game->SaveFreePlayEnding(bTrueEndingSelected))
    {
        FTunaSweeperDialogueLine Retry;
        Retry.DialogueText = Game->ResolveLocalizedText(TEXT("ui.freeplay_ending.save_failed"), FText::GetEmpty());
        Dialogue->StartDialogue({Retry}, Game->GetDialogueCharactersPerSecond());
        return;
    }
    ReleasePresentation();
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/IntroMap"));
}

void ATunaSweeperEndingActor::ReleasePresentation()
{
    if (Fade) Fade->RemoveFromParent();
    if (Dialogue) { Dialogue->SetFinishedDelegate({}); Dialogue->RemoveFromParent(); }
    if (IsValid(Player))
    {
        if (bEndingActive) Player->SetDemoEndingInputLock(false);
        if (bOwnsPause) Player->SetPause(false);
        if (bEndingActive) Player->ApplyDefaultGameInputMode();
    }
    bOwnsPause = false;
    bEndingActive = false;
    Fade = nullptr; Dialogue = nullptr; Player = nullptr;
}

void ATunaSweeperEndingActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Game.IsValid()) Game->OnInventoryStateChanged.RemoveAll(this);
    ReleasePresentation();
    Super::EndPlay(Reason);
}
