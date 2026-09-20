#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "UI/TunaSweeperTutorialPopupWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SOverlay.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Misc/PackageName.h"

namespace
{
    struct FTutorialWorldContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance, FWorldContext* Context)
        {
            auto Member = &FTutorialWorldContextAccess::WorldContext;
            Instance->*Member = Context;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaTutorialTriggerTest,
    "TunaSweeper.UI.Tutorial.FirstPassage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaTutorialTriggerTest::RunTest(const FString&)
{
    const FName Flag(TEXT("tutorial.bunker.basics_seen"));
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("TutorialBunkerMap"), CreatePackage(TEXT("/Temp/TutorialBunkerMap")));
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
    FTutorialWorldContextAccess::Attach(Instance.Get(), &Context);
    Context.OwningGameInstance = Instance.Get();
    World->SetGameInstance(Instance.Get());
    Instance->UGameInstance::Init();
    Instance->bInventoryStateInitialized = true;
    // Exercise completion without reading or writing the user's real save slots.
    Instance->RetiredDemoSaveSlotIndex = Instance->ActiveSaveSlotIndex;
    Context.GameViewport = NewObject<UGameViewportClient>(GEngine);
    TSharedRef<SOverlay> ViewportOverlay = SNew(SOverlay);
    Context.GameViewport->SetViewportOverlayWidget(nullptr, ViewportOverlay);
    ON_SCOPE_EXIT
    {
        Instance->UGameInstance::Shutdown();
        World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr;
        Context.GameViewport = nullptr;
        FTutorialWorldContextAccess::Attach(Instance.Get(), nullptr);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    World->SetGameMode(FURL());
    World->InitializeActorsForPlay(FURL());
    auto* Controller = World->SpawnActor<ATunaSweeperPlayerController>();
    auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    LocalPlayer->SetControllerId(0);
    LocalPlayer->ViewportClient = Context.GameViewport;
    Controller->SetPlayer(LocalPlayer);
    auto* Pawn = World->SpawnActor<ATunaSweeperTopDownCharacter>();
    Controller->Possess(Pawn);
    Pawn->SetActorLocation(FVector(-439.01, -135.58, 79.35));
    TestTrue(TEXT("Screenshot location is eligible on the first visit"), Controller->CanShowBunkerBasicsTutorial());
    Pawn->SetActorLocation(FVector(-1000, -135.58, 79.35));
    TestFalse(TEXT("Distant room does not trigger"), Controller->CanShowBunkerBasicsTutorial());
    Pawn->SetActorLocation(FVector(-439.01, -135.58, 400));
    TestFalse(TEXT("Different height does not trigger"), Controller->CanShowBunkerBasicsTutorial());
    Pawn->SetActorLocation(FVector(-439.01, -135.58, 79.35));
    Controller->bDialogueSequenceActive = true;
    TestFalse(TEXT("Dialogue defers tutorial"), Controller->TryShowBunkerBasicsTutorial());
    Controller->bDialogueSequenceActive = false;
    Controller->SetIgnoreMoveInput(true);
    TestFalse(TEXT("Input lock defers tutorial"), Controller->CanShowBunkerBasicsTutorial());
    Controller->SetIgnoreMoveInput(false);
    if (!TestTrue(TEXT("Actual authored popup opens"), Controller->TryShowBunkerBasicsTutorial())) return false;
    TestTrue(TEXT("World is paused"), UGameplayStatics::IsGamePaused(World));
    TestTrue(TEXT("Gameplay UI and movement blocked"), Controller->IsPauseMenuOpen() && Controller->IsMoveInputIgnored());
    TestFalse(TEXT("Popup cannot duplicate"), Controller->TryShowBunkerBasicsTutorial());
    Controller->TogglePauseMenu();
    TestNull(TEXT("Escape cannot stack a pause menu"), Controller->PauseMenuWidget.Get());
    auto* Popup = Controller->TutorialPopupWidget.Get();
    auto* Pages = Cast<UWidgetSwitcher>(Popup->WidgetTree->FindWidget(TEXT("PageSwitcher")));
    TestEqual(TEXT("Only first page is shown"), Pages->GetActiveWidgetIndex(), 0);
    auto* Continue = Cast<UButton>(Popup->WidgetTree->FindWidget(TEXT("ContinueButton")));
    Continue->OnClicked.Broadcast();
    TestFalse(TEXT("Continue resumes the world"), UGameplayStatics::IsGamePaused(World));
    TestFalse(TEXT("Continue restores gameplay input"), Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored());
    TestFalse(TEXT("Popup removed from viewport"), Popup->IsInViewport());
    TestTrue(TEXT("Completion recorded per slot even if disk save is unavailable"), Instance->IsScenarioProgressFlagSet(Flag));
    TestFalse(TEXT("Standing in or reentering the passage cannot repeat"), Controller->TryShowBunkerBasicsTutorial());
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->CompletedScenarioFlags = Instance->CompletedScenarioFlags.Array();
    TArray<uint8> Bytes;
    TestTrue(TEXT("Completion serializes through existing save format"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Loaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Save reloads"), Loaded)) TestTrue(TEXT("Completion survives reload"), Loaded->CompletedScenarioFlags.Contains(Flag));
    const FName CombatFlag(TEXT("tutorial.raid.combat_seen"));
    TestFalse(TEXT("Combat tutorial does not open in bunker"), Controller->TryShowRaidCombatTutorial());
    const FName RaidLevel = TunaSweeperBuildFlavor::GetRaidGameplayLevelName();
    World->GetOutermost()->Rename(*(TEXT("/Temp/") + FPackageName::GetShortName(RaidLevel.ToString())));
    TestFalse(TEXT("Opening a raid directly does not count as bunker departure"), Controller->TryShowRaidCombatTutorial());
    Instance->HandleLevelTravelPersistence(TEXT("BunkerMap"), RaidLevel);
    TestTrue(TEXT("Bunker departure queues combat help"), Instance->HasPendingRaidTutorial());
    auto* Transition = Instance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>();
    if (!TestNotNull(TEXT("Transition subsystem exists"), Transition)) return false;
    Transition->Phase = UTunaSweeperLevelTransitionSubsystem::ETransitionPhase::CircularRevealFinalExpand;
    TestFalse(TEXT("Reveal must finish even if input was already unlocked"), Controller->TryShowRaidCombatTutorial());
    Transition->Phase = UTunaSweeperLevelTransitionSubsystem::ETransitionPhase::Idle;
    Controller->SetIgnoreMoveInput(true);
    TestFalse(TEXT("Arrival input lock defers second tutorial"), Controller->TryShowRaidCombatTutorial());
    Controller->SetIgnoreMoveInput(false);
    Controller->bDialogueSequenceActive = true;
    TestFalse(TEXT("Arrival dialogue defers second tutorial"), Controller->TryShowRaidCombatTutorial());
    Controller->bDialogueSequenceActive = false;
    if (!TestTrue(TEXT("Combat help opens when gameplay becomes available"), Controller->TryShowRaidCombatTutorial())) return false;
    Popup = Controller->TutorialPopupWidget.Get();
    Pages = Cast<UWidgetSwitcher>(Popup->WidgetTree->FindWidget(TEXT("PageSwitcher")));
    TestEqual(TEXT("Second authored page is selected"), Pages->GetActiveWidgetIndex(), 1);
    TestTrue(TEXT("Combat help pauses gameplay"), UGameplayStatics::IsGamePaused(World));
    Cast<UButton>(Popup->WidgetTree->FindWidget(TEXT("ContinueButton")))->OnClicked.Broadcast();
    TestFalse(TEXT("Combat help restores gameplay"), UGameplayStatics::IsGamePaused(World) || Controller->IsMoveInputIgnored());
    TestTrue(TEXT("Second page has its own slot flag"), Instance->IsScenarioProgressFlagSet(CombatFlag));
    Instance->HandleLevelTravelPersistence(TEXT("BunkerMap"), RaidLevel);
    TestFalse(TEXT("Later raid departures do not queue completed help"), Instance->HasPendingRaidTutorial());
    TestFalse(TEXT("Second help cannot repeat"), Controller->TryShowRaidCombatTutorial());
    Save->CompletedScenarioFlags = Instance->CompletedScenarioFlags.Array();
    TestTrue(TEXT("Both help flags serialize"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    Loaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Both help flags reload"), Loaded))
        TestTrue(TEXT("Both completion flags survive disk format round trip"), Loaded->CompletedScenarioFlags.Contains(CombatFlag) && Loaded->CompletedScenarioFlags.Contains(Flag));
    Instance->ResetRuntimeStateForSaveSlotSelection();
    TestFalse(TEXT("New slot clears tutorial completion"), Instance->IsScenarioProgressFlagSet(Flag));
    TestFalse(TEXT("New slot clears combat help completion"), Instance->IsScenarioProgressFlagSet(CombatFlag));
    TestFalse(TEXT("Pending help never leaks into another save slot"), Instance->HasPendingRaidTutorial());
    return true;
}
#endif
