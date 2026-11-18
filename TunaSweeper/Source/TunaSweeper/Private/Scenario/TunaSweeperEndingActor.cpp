#include "Scenario/TunaSweeperEndingActor.h"
#include "Scenario/TunaSweeperEndingPresentation.h"
#include "Scenario/SSTOLaunchCinematic.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/Astra/EndingMonologueGPT62Astra.h"
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
    MonologueClass = TSoftClassPtr<UEndingMonologueGPT62Astra>(FSoftObjectPath(TEXT(
        "/Game/UI/Story/Astra/WBP_EndingMonologue_GPT62Astra.WBP_EndingMonologue_GPT62Astra_C")));
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
    if (!Presentation) return false;
    const bool bLaunch = bTrueEndingSelected && IsValid(LaunchLander) && !Presentation->CinematicMonologueStringKey.IsNone();
    TSubclassOf<UEndingMonologueGPT62Astra> CardClass;
    if (bLaunch)
    {
        CardClass = MonologueClass.LoadSynchronous();
        const auto* Strings = Instance->GetSubsystem<UTunaSweeperTextSubsystem>();
        if (!CardClass || !Strings || !Strings->TryGetTextByKey(Presentation->CinematicMonologueStringKey,
            Instance->GetCurrentTextLanguage(), MonologueLine) || MonologueLine.IsEmpty()) return false;
    }
    else if (!Presentation->ResolveDialogue(Instance, StoryLines)) return false;
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
    if (bLaunch) Monologue = CreateWidget<UEndingMonologueGPT62Astra>(Player, CardClass);
    else Dialogue = CreateWidget<UTunaSweeperDialogueWidget>(Player);
    if (!Fade || (!Monologue && !Dialogue)) { ReleasePresentation(); return false; }
    bOwnsPause = Player->SetPause(true);
    if (!bOwnsPause) { ReleasePresentation(); return false; }
    bEndingActive = true;
    if (auto* MovingCharacter = Cast<ACharacter>(InstigatorPawn)) MovingCharacter->GetCharacterMovement()->StopMovementImmediately();
    Player->SetEndingInputLock(true);
    Player->SetInputMode(FInputModeUIOnly());
    Player->bShowMouseCursor = !bLaunch;
    Fade->AddToViewport(1000);
    Fade->StartFadeToBlack(FadeSeconds, FSimpleDelegate::CreateUObject(this, bLaunch ? &ThisClass::StartLaunch : &ThisClass::ShowStory));
    return true;
}

void ATunaSweeperEndingActor::StartLaunch()
{
    if (!IsValid(Player) || !IsValid(LaunchLander)) { ReleasePresentation(); return; }
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.ObjectFlags |= RF_Transient;
    LaunchStage = GetWorld()->SpawnActor<ASSTOLaunchCinematic>(ASSTOLaunchCinematic::StaticClass(),
        FTransform::Identity, Params);
    if (!LaunchStage || !LaunchStage->Start(Player, LaunchLander)) { ReleasePresentation(); return; }
    LaunchStage->OnOrbitFinished.BindUObject(this, &ThisClass::FadeOutOfOrbit);
    LaunchStage->OnInterrupted.BindUObject(this, &ThisClass::ReleasePresentation);
    Fade->StartFadeFromBlack(0.65f);
}

void ATunaSweeperEndingActor::FadeOutOfOrbit()
{
    if (!bEndingActive || !Fade) return;
    if (!Fade->IsInViewport()) Fade->AddToViewport(1000);
    Fade->StartFadeToBlack(1.7f, FSimpleDelegate::CreateUObject(this, &ThisClass::ShowMonologue));
}

void ATunaSweeperEndingActor::ShowMonologue()
{
    if (!IsValid(Player) || !Monologue) { ReleasePresentation(); return; }
    RemoveLaunchStage();
    Monologue->AddToViewport(1100);
    Monologue->PlayLine(MonologueLine, FSimpleDelegate::CreateUObject(this, &ThisClass::FinishEnding));
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(Monologue->TakeWidget());
    Player->SetInputMode(Input);
    Monologue->SetUserFocus(Player);
    Monologue->SetKeyboardFocus();
}

void ATunaSweeperEndingActor::RemoveLaunchStage()
{
    if (IsValid(LaunchStage))
    {
        LaunchStage->OnOrbitFinished.Unbind();
        LaunchStage->OnInterrupted.Unbind();
        LaunchStage->Stop();
        LaunchStage->Destroy();
    }
    LaunchStage = nullptr;
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
    if (!bEndingActive || !Game.IsValid() || !IsValid(Player)) { ReleasePresentation(); return; }
    if (!Game->SaveFreePlayEnding(bTrueEndingSelected))
    {
        FTunaSweeperDialogueLine Retry;
        Retry.DialogueText = Game->ResolveLocalizedText(TEXT("ui.freeplay_ending.save_failed"), FText::GetEmpty());
        if (Monologue)
        {
            Player->bShowMouseCursor = true;
            Monologue->ShowSaveRetry(Retry.DialogueText, FSimpleDelegate::CreateUObject(this, &ThisClass::FinishEnding));
        }
        else Dialogue->StartDialogue({Retry}, Game->GetDialogueCharactersPerSecond());
        return;
    }
    ReleasePresentation();
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/IntroMap"));
}

void ATunaSweeperEndingActor::ReleasePresentation()
{
    RemoveLaunchStage();
    if (Monologue) Monologue->RemoveFromParent();
    Monologue = nullptr;
    MonologueLine = FText::GetEmpty();
    if (Fade) Fade->RemoveFromParent();
    if (Dialogue) { Dialogue->SetFinishedDelegate({}); Dialogue->RemoveFromParent(); }
    if (IsValid(Player))
    {
        if (bEndingActive) Player->SetEndingInputLock(false);
        if (bOwnsPause) Player->SetPause(false);
        if (bEndingActive) Player->ApplyDefaultGameInputMode();
    }
    else if (bOwnsPause && GetWorld()) UGameplayStatics::SetGamePaused(this, false);
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
