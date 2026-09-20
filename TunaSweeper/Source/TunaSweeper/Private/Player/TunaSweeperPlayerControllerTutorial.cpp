#include "Player/TunaSweeperPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperTutorialPopupWidget.h"

namespace
{
    const FName BasicsTutorialFlag(TEXT("tutorial.bunker.basics_seen"));
    const FName CombatTutorialFlag(TEXT("tutorial.raid.combat_seen"));
}

bool ATunaSweeperPlayerController::IsTutorialGameplayReady() const
{
    const auto* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
    const auto* Instance = GetGameInstance<UTunaSweeperGameInstance>();
    const auto* Transition = Instance ? Instance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>() : nullptr;
    return IsLocalController() && ControlledCharacter && !ControlledCharacter->IsDead() && Instance
        && !(Transition && Transition->IsTransitionActive())
        && !IsPauseMenuOpen() && !UGameplayStatics::IsGamePaused(this)
        && !bDialogueSequenceActive && !IsHousingModeOpen()
        && !IsMoveInputIgnored() && !IsLookInputIgnored()
        && !(DifficultyAdjustmentWidget && DifficultyAdjustmentWidget->IsInViewport())
        && !(GameHudWidget && GameHudWidget->GetHudMode() != ETunaSweeperHudMode::None);
}

bool ATunaSweeperPlayerController::CanShowBunkerBasicsTutorial() const
{
    return IsBunkerMap() && IsTutorialGameplayReady()
        && !GetGameInstance<UTunaSweeperGameInstance>()->IsScenarioProgressFlagSet(BasicsTutorialFlag)
        && FBox::BuildAABB(BasicsTutorialCenter, BasicsTutorialExtent).IsInsideOrOn(GetPawn()->GetActorLocation());
}

bool ATunaSweeperPlayerController::CanShowRaidCombatTutorial() const
{
    const auto* Instance = GetGameInstance<UTunaSweeperGameInstance>();
    return IsRaidMap() && Instance && Instance->HasPendingRaidTutorial()
        && !Instance->IsCombatTestSession() && !Instance->IsScenarioProgressFlagSet(CombatTutorialFlag)
        && IsTutorialGameplayReady();
}

bool ATunaSweeperPlayerController::TryShowBunkerBasicsTutorial()
{
    return CanShowBunkerBasicsTutorial() && ShowTutorialPage(0, BasicsTutorialFlag);
}

bool ATunaSweeperPlayerController::TryShowRaidCombatTutorial()
{
    return CanShowRaidCombatTutorial() && ShowTutorialPage(1, CombatTutorialFlag);
}

bool ATunaSweeperPlayerController::ShowTutorialPage(int32 PageIndex, FName CompletionFlag)
{
    auto* Popup = CreateWidget<UTunaSweeperTutorialPopupWidget>(this, TutorialPopupClass.LoadSynchronous());
    if (!Popup || !Popup->WidgetTree) return false;
    auto* Pages = Cast<UWidgetSwitcher>(Popup->WidgetTree->FindWidget(TEXT("PageSwitcher")));
    auto* Continue = Cast<UButton>(Popup->WidgetTree->FindWidget(TEXT("ContinueButton")));
    // Never pause unless the authored asset provides a working way to resume.
    if (!Pages || PageIndex < 0 || PageIndex >= Pages->GetChildrenCount() || !Continue) return false;
    Pages->SetActiveWidgetIndex(PageIndex);
    Continue->OnClicked.AddDynamic(this, &ThisClass::DismissTutorialPopup);
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
    ActiveTutorialCompletionFlag = CompletionFlag;
    SetIgnoreLookInput(true);
    ApplyDefaultGameInputMode();
    Continue->SetKeyboardFocus();
    return true;
}

void ATunaSweeperPlayerController::DismissTutorialPopup()
{
    if (!TutorialPopupWidget) return;
    if (auto* Instance = GetGameInstance<UTunaSweeperGameInstance>())
    {
        // Existing per-slot scenario persistence also clears this on a new game.
        Instance->MarkScenarioProgressFlag(ActiveTutorialCompletionFlag, true);
    }
    TutorialPopupWidget->RemoveFromParent();
    TutorialPopupWidget = nullptr;
    ActiveTutorialCompletionFlag = NAME_None;
    SetPause(false);
    ApplyDefaultGameInputMode();
}
