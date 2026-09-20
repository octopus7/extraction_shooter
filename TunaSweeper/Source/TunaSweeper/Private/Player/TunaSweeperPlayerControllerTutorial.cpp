#include "Player/TunaSweeperPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperTutorialPopupWidget.h"

namespace
{
    const FName BasicsTutorialFlag(TEXT("tutorial.bunker.basics_seen"));
}

bool ATunaSweeperPlayerController::CanShowBunkerBasicsTutorial() const
{
    const auto* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
    const auto* Instance = GetGameInstance<UTunaSweeperGameInstance>();
    return IsLocalController() && IsBunkerMap() && ControlledCharacter && !ControlledCharacter->IsDead() && Instance
        && !Instance->IsScenarioProgressFlagSet(BasicsTutorialFlag)
        && !IsPauseMenuOpen() && !UGameplayStatics::IsGamePaused(this)
        && !bDialogueSequenceActive && !IsHousingModeOpen()
        && !IsMoveInputIgnored() && !IsLookInputIgnored()
        && !(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport())
        && !(GameHudWidget && GameHudWidget->GetHudMode() != ETunaSweeperHudMode::None)
        && FBox::BuildAABB(BasicsTutorialCenter, BasicsTutorialExtent).IsInsideOrOn(ControlledCharacter->GetActorLocation());
}

bool ATunaSweeperPlayerController::TryShowBunkerBasicsTutorial()
{
    if (!CanShowBunkerBasicsTutorial()) return false;
    auto* Popup = CreateWidget<UTunaSweeperTutorialPopupWidget>(this, TutorialPopupClass.LoadSynchronous());
    if (!Popup || !Popup->WidgetTree) return false;
    auto* Pages = Cast<UWidgetSwitcher>(Popup->WidgetTree->FindWidget(TEXT("PageSwitcher")));
    auto* Continue = Cast<UButton>(Popup->WidgetTree->FindWidget(TEXT("ContinueButton")));
    // Never pause unless the authored asset provides a working way to resume.
    if (!Pages || Pages->GetChildrenCount() < 1 || !Continue) return false;
    Pages->SetActiveWidgetIndex(0);
    Continue->OnClicked.AddDynamic(this, &ThisClass::DismissBunkerBasicsTutorial);
    CancelPawnGameplayActions();
    if (!SetPause(true)) return false;
    TutorialPopupWidget = Popup;
    Popup->AddToViewport(950);
    if (!Popup->IsInViewport())
    {
        TutorialPopupWidget = nullptr;
        SetPause(false);
        return false;
    }
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    ApplyDefaultGameInputMode();
    Continue->SetKeyboardFocus();
    return true;
}

void ATunaSweeperPlayerController::DismissBunkerBasicsTutorial()
{
    if (!TutorialPopupWidget) return;
    if (auto* Instance = GetGameInstance<UTunaSweeperGameInstance>())
    {
        // Existing per-slot scenario persistence also clears this on a new game.
        Instance->MarkScenarioProgressFlag(BasicsTutorialFlag, true);
    }
    TutorialPopupWidget->RemoveFromParent();
    TutorialPopupWidget = nullptr;
    SetPause(false);
    ApplyDefaultGameInputMode();
}
