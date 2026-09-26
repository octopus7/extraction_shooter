#include "Scenario/TunaSweeperDemoEndingActor.h"
#include "Camera/CameraComponent.h"
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperScenarioSubsystem.h"
#include "UI/TunaSweeperDemoFarewellWidget.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "TimerManager.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperBgmSubsystem.h"
namespace
{
const FName FinalQuest(TEXT("demo_q4_todays_reward"));
const FName EndingSeen(TEXT("demo.ending.farewell_seen"));
}
ATunaSweeperDemoEndingActor::ATunaSweeperDemoEndingActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("EndingCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->SetRelativeLocation(FVector(-420,0,400));
    Camera->SetRelativeRotation(FRotator(-40,0,0));
    Camera->SetFieldOfView(55);
    LunaPosition = CreateDefaultSubobject<USceneComponent>(TEXT("LunaPosition"));
    LunaPosition->SetupAttachment(RootComponent);
    LunaPosition->SetRelativeLocation(FVector(0,-110,0));
    LunaPosition->SetRelativeRotation(FRotator(0,90,0));
    MolePosition = CreateDefaultSubobject<USceneComponent>(TEXT("MolePosition"));
    MolePosition->SetupAttachment(RootComponent);
    MolePosition->SetRelativeLocation(FVector(0,110,0));
    MolePosition->SetRelativeRotation(FRotator(0,-90,0));
}
ATunaSweeperDemoEndingActor* ATunaSweeperDemoEndingActor::Find(UWorld* World)
{
    if (World) for (TActorIterator<ATunaSweeperDemoEndingActor> It(World); It; ++It) return *It;
    return nullptr;
}
void ATunaSweeperDemoEndingActor::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(StageTimer, this, &ThisClass::ResumePendingEnding, 1.f, false);
}
void ATunaSweeperDemoEndingActor::ResumePendingEnding()
{
    // Main is unlimited play: the demo's ending/title-return flow must never run there.
    if (!TunaSweeperBuildFlavor::IsDemo()) return;
    auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Quests = GI ? GI->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
    if (Quests && Quests->GetQuestState(FinalQuest) == ETunaSweeperQuestState::RewardCompleted && !GI->IsScenarioProgressFlagSet(EndingSeen))
        StartEnding();
}
void ATunaSweeperDemoEndingActor::QueueEnding()
{
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::ResumePendingEnding));
}
bool ATunaSweeperDemoEndingActor::StartEnding()
{
    if (!TunaSweeperBuildFlavor::IsDemo()) return false;
    Player = Cast<ATunaSweeperPlayerController>(UGameplayStatics::GetPlayerController(this,0));
    auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
    auto* ScenarioSubsystem = GI ? GI->GetSubsystem<UTunaSweeperScenarioSubsystem>() : nullptr;
    FTunaSweeperScenarioPresentation Presentation;
    if (bEndingActive || !Player || !Player->GetPawn() || !FarewellIllustration.LoadSynchronous() ||
        !ScenarioSubsystem || !ScenarioSubsystem->TryResolveScenario(
            FName(TEXT("demo.ending.dinner")), FName(*GetWorld()->GetMapName()), false, Presentation) ||
        Presentation.DialogueLines.IsEmpty()) return false;
    DinnerDialogueLines = MoveTemp(Presentation.DialogueLines);
    DinnerDialogueCompletionFlag = Presentation.CompletionFlag;
    if (Player->IsDialogueSequenceActive())
    {
        GetWorldTimerManager().SetTimer(StageTimer,this,&ThisClass::ResumePendingEnding,.25f,false);
        return false;
    }
    Fade = CreateWidget<UTunaSweeperScreenFadeWidget>(Player);
    Dialogue = CreateWidget<UTunaSweeperDialogueWidget>(Player);
    Farewell = CreateWidget<UTunaSweeperDemoFarewellWidget>(Player);
    if (!Fade || !Dialogue || !Farewell) return false;
    bEndingActive = true;
    Player->SetDemoEndingInputLock(true);
    Player->SetInputMode(FInputModeUIOnly());
    Player->bShowMouseCursor = true;
    Fade->AddToViewport(1000);
    Fade->StartFadeToBlack(FadeSeconds, FSimpleDelegate::CreateUObject(this,&ThisClass::ShowDinner));
    return true;
}
void ATunaSweeperDemoEndingActor::ShowDinner()
{
    if (!Player || !Player->GetPawn()) return;
    APawn* Pawn = Player->GetPawn();
    PreviousLuna = Pawn->GetActorTransform();
    if (auto* Character = Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->StopMovementImmediately();
    Pawn->SetActorLocationAndRotation(LunaPosition->GetComponentLocation(),LunaPosition->GetComponentRotation(),false,nullptr,ETeleportType::TeleportPhysics);
    for (TActorIterator<ATunaSweeperMoleCompanionActor> It(GetWorld()); It; ++It) { Mole = *It; break; }
    if (Mole)
    {
        PreviousMole = Mole->GetActorTransform();
        bMoleTickEnabled = Mole->IsActorTickEnabled();
        Mole->SetActorTickEnabled(false);
        Mole->SetActorLocationAndRotation(MolePosition->GetComponentLocation(),MolePosition->GetComponentRotation(),false,nullptr,ETeleportType::TeleportPhysics);
    }
    bActorsMoved = true;
    Player->SetViewTarget(this);
    Fade->StartFadeFromBlack(FadeSeconds);
    GetWorldTimerManager().SetTimer(StageTimer,this,&ThisClass::StartDinnerDialogue,FadeSeconds+.05f,false);
}
void ATunaSweeperDemoEndingActor::StartDinnerDialogue()
{
    Dialogue->SetFinishedDelegate(FTunaSweeperDialogueFinishedDelegate::CreateUObject(this,&ThisClass::DinnerFinished));
    Dialogue->AddToViewport(500);
    auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
    Dialogue->StartDialogue(DinnerDialogueLines,GI ? GI->GetDialogueCharactersPerSecond() : 25.f);
    FInputModeUIOnly Input; Input.SetWidgetToFocus(Dialogue->TakeWidget());
    Player->SetInputMode(Input); Dialogue->SetKeyboardFocus();
}
void ATunaSweeperDemoEndingActor::DinnerFinished()
{
    Player->SetInputMode(FInputModeUIOnly());
    if (DinnerDialogueCompletionFlag != NAME_None)
    {
        if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
        {
            GI->MarkScenarioProgressFlag(DinnerDialogueCompletionFlag, true);
        }
    }
    Fade->AddToViewport(1000);
    Fade->StartFadeToBlack(FadeSeconds,FSimpleDelegate::CreateUObject(this,&ThisClass::ShowFarewell));
}
void ATunaSweeperDemoEndingActor::ShowFarewell()
{
    if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
    {
        if (auto* BgmSubsystem = GI->GetSubsystem<UTunaSweeperBgmSubsystem>())
        {
            BgmSubsystem->FadeOutAndStop(FarewellBgmFadeOutSeconds);
        }
    }
    Dialogue->RemoveFromParent();
    RestoreActors();
    Farewell->Illustration = FarewellIllustration.LoadSynchronous();
    Farewell->OnContinue = FSimpleDelegate::CreateUObject(this,&ThisClass::ReturnToTitle);
    Farewell->AddToViewport(600);
    if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
    {
        GI->MarkScenarioProgressFlag(EndingSeen,true);
        GI->DeleteCompletedDemoSave();
    }
    Fade->StartFadeFromBlack(FadeSeconds);
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(Farewell->TakeWidget());
    Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Input);
    Farewell->SetUserFocus(Player);
    Farewell->SetKeyboardFocus();
}
void ATunaSweeperDemoEndingActor::ReturnToTitle()
{
    Fade->AddToViewport(1000);
    Fade->StartFadeToBlack(FadeSeconds,FSimpleDelegate::CreateUObject(this,&ThisClass::OpenTitle));
}
void ATunaSweeperDemoEndingActor::OpenTitle()
{
    if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
    {
        // Retry only if the earlier deletion failed and the completion flag remains.
        if (GI->IsScenarioProgressFlagSet(EndingSeen)) GI->DeleteCompletedDemoSave();
    }
    UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/IntroMap"));
}
void ATunaSweeperDemoEndingActor::RestoreActors()
{
    if (!bActorsMoved) return;
    if (Player && Player->GetPawn()) Player->GetPawn()->SetActorTransform(PreviousLuna,false,nullptr,ETeleportType::TeleportPhysics);
    if (Mole) { Mole->SetActorTransform(PreviousMole,false,nullptr,ETeleportType::TeleportPhysics); Mole->SetActorTickEnabled(bMoleTickEnabled); }
    bActorsMoved = false;
}
void ATunaSweeperDemoEndingActor::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    RestoreActors();
    if (Fade) Fade->RemoveFromParent();
    if (Dialogue) Dialogue->RemoveFromParent();
    if (Farewell) Farewell->RemoveFromParent();
    if (Player && bEndingActive) Player->SetDemoEndingInputLock(false);
    Super::EndPlay(Reason);
}
