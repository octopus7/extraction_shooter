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
#include "HAL/PlatformTime.h"
#include "Framework/Application/SlateApplication.h"

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
    return IsLocalController() && FPlatformTime::Seconds() >= TutorialReopenBlockedUntilSeconds
        && ControlledCharacter && !ControlledCharacter->IsDead() && Instance
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

bool ATunaSweeperPlayerController::OpenTutorialReview()
{
    return IsBunkerMap() && IsTutorialGameplayReady() && ShowTutorialPage(0, NAME_None, true);
}

void ATunaSweeperPlayerController::ShowPreviousTutorialPage() { StepTutorialPage(-1); }
void ATunaSweeperPlayerController::ShowNextTutorialPage() { StepTutorialPage(1); }

void ATunaSweeperPlayerController::StepTutorialPage(int32 Direction)
{
    if (!bTutorialReviewMode || !TutorialPopupWidget) return;
    auto* Pages = Cast<UWidgetSwitcher>(TutorialPopupWidget->GetWidgetFromName(TEXT("PageSwitcher")));
    if (!Pages || Pages->GetChildrenCount() == 0) return;
    const int32 Count = Pages->GetChildrenCount();
    Pages->SetActiveWidgetIndex((Pages->GetActiveWidgetIndex() + Direction + Count) % Count);
}

bool ATunaSweeperPlayerController::ShowTutorialPage(int32 PageIndex, FName CompletionFlag, bool bReviewMode)
{
    auto* Popup = CreateWidget<UTunaSweeperTutorialPopupWidget>(this, TutorialPopupClass.LoadSynchronous());
    if (!Popup || !Popup->WidgetTree) return false;
    auto* Pages = Cast<UWidgetSwitcher>(Popup->WidgetTree->FindWidget(TEXT("PageSwitcher")));
    auto* Continue = Cast<UButton>(Popup->WidgetTree->FindWidget(TEXT("ContinueButton")));
    auto* Previous = Cast<UButton>(Popup->GetWidgetFromName(TEXT("PreviousPageButton")));
    auto* Next = Cast<UButton>(Popup->GetWidgetFromName(TEXT("NextPageButton")));
    // Never pause unless the authored asset provides a working way to resume.
    if (!Pages || PageIndex < 0 || PageIndex >= Pages->GetChildrenCount() || !Continue) return false;
    Popup->SetIsFocusable(true);
    if (bReviewMode && (!Previous || !Next)) return false;
    if (Previous) Previous->SetVisibility(bReviewMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (Next) Next->SetVisibility(bReviewMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bReviewMode)
    {
        Previous->OnClicked.AddDynamic(this, &ThisClass::ShowPreviousTutorialPage);
        Next->OnClicked.AddDynamic(this, &ThisClass::ShowNextTutorialPage);
    }
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
    bTutorialReviewMode = bReviewMode;
    SetIgnoreLookInput(true);
    ApplyDefaultGameInputMode();
    Continue->SetKeyboardFocus();
    return true;
}

void ATunaSweeperPlayerController::DismissTutorialPopup()
{
    if (!TutorialPopupWidget) return;
    CloseTutorialPopup();
}

void ATunaSweeperPlayerController::CloseTutorialPopup()
{
    if (!TutorialPopupWidget) return;
    // Block the interaction actor and automatic location check until the closing input has settled.
    TutorialReopenBlockedUntilSeconds = FPlatformTime::Seconds() + 0.35;
    if (auto* Instance = GetGameInstance<UTunaSweeperGameInstance>())
    {
        // Existing per-slot scenario persistence also clears this on a new game.
        if (!ActiveTutorialCompletionFlag.IsNone())
            Instance->MarkScenarioProgressFlag(ActiveTutorialCompletionFlag, true);
    }
    FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
    TutorialPopupWidget->RemoveFromParent();
    TutorialPopupWidget = nullptr;
    ActiveTutorialCompletionFlag = NAME_None;
    bTutorialReviewMode = false;
    SetPause(false);
    ApplyDefaultGameInputMode();
    // Removing the focused UMG button leaves Slate without a keyboard target until
    // the next mouse click unless focus is explicitly returned to the game viewport.
    FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
