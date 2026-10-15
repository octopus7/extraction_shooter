#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Interaction/TunaSweeperExtractionPointActor.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UI/TunaSweeperRaidExperienceWidget.h"
#include "Widgets/SWidget.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperExtractionSafetyTest,
    "TunaSweeper.Extraction.CompletionSafety", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperExtractionSafetyTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
    World->SetGameInstance(Instance.Get());
    Context.OwningGameInstance = Instance.Get();
    ON_SCOPE_EXIT {
        Context.TravelURL.Empty();
        World->EndPlay(EEndPlayReason::Quit);
        World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr;
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    FURL URL; URL.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    World->SetGameMode(URL);
    World->InitializeActorsForPlay(URL);
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>();
    Controller->Possess(Player);
    auto* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>(FVector(1000, 0, 200), FRotator::ZeroRotator);
    auto* Extraction = World->SpawnActor<ATunaSweeperExtractionPointActor>();
    World->GetWorldSettings()->NotifyBeginPlay();
    World->GetWorldSettings()->NotifyMatchStarted();
    World->BeginPlay();
    TestFalse(TEXT("Hold phase does not freeze combat"), World->IsPaused());
    TestFalse(TEXT("Player remains vulnerable before completion"), Player->IsDamageInvulnerable());
    auto* Vitals = Player->FindComponentByClass<UTunaSweeperVitalsComponent>();
    TestTrue(TEXT("Pre-completion attacks still deal damage"), Player->TakeDamage(1, FDamageEvent(), nullptr, nullptr) > 0);
    const float Health = Vitals->GetVitalsState().Health;
    Enemy->GetCharacterMovement()->Velocity = FVector(100, 0, 0);
    const FVector EnemyPosition = Enemy->GetActorLocation();
    int32 AttackTimerCalls = 0;
    FTimerHandle AttackTimer;
    World->GetTimerManager().SetTimer(AttackTimer, [&AttackTimerCalls] { ++AttackTimerCalls; }, 0.01f, true);
    World->GetTimerManager().Tick(0.0f); // Activate the pending attack timer before the pause.
    ++GFrameCounter;
    TestTrue(TEXT("Extraction completion is accepted"), Extraction->ExtractPawn(Player));
    Context.TravelURL.Empty(); // Do not travel or access the user's saves in this fixture.
    TestTrue(TEXT("Completed extraction pauses world combat"), World->IsPaused());
    TestTrue(TEXT("In-flight projectile guard sees invulnerability"), Player->IsDamageInvulnerable());
    TestEqual(TEXT("A late direct damage callback is ignored"), Player->TakeDamage(10, FDamageEvent(), nullptr, nullptr), 0.0f);
    TestEqual(TEXT("A late radial damage callback is ignored"), Player->TakeDamage(10, FRadialDamageEvent(), nullptr, nullptr), 0.0f);
    TestEqual(TEXT("Health stays unchanged throughout completion"), Vitals->GetVitalsState().Health, Health);
    World->Tick(LEVELTICK_All, 0.1f);
    TestEqual(TEXT("Attack timers stop during extraction"), AttackTimerCalls, 0);
    TestTrue(TEXT("Enemy movement stops during extraction"), Enemy->GetActorLocation().Equals(EnemyPosition));
    TestTrue(TEXT("Travel presentation can tick while paused"), GetDefault<UTunaSweeperLevelTransitionSubsystem>()->IsTickableWhenPaused());
    auto* ExperienceWidget = CreateWidget<UTunaSweeperRaidExperienceWidget>(World, UTunaSweeperRaidExperienceWidget::StaticClass());
    TestNotNull(TEXT("Experience presentation widget exists"), ExperienceWidget);
    if (ExperienceWidget)
    {
        TSharedRef<SWidget> Slate = ExperienceWidget->TakeWidget();
        FTunaSweeperExperienceAnimationState Animation;
        Animation.AnimationDurationSeconds = 1.0f;
        ExperienceWidget->StartExperiencePresentation(Animation);
        TestTrue(TEXT("Combat is still paused before Slate animation tick"), World->IsPaused());
        Slate->Tick(FGeometry(), 0.0, 1.1f);
        TestTrue(TEXT("Experience animation finishes while combat is paused"), ExperienceWidget->IsExperienceAnimationFinished());
        bool bContinued = false;
        ExperienceWidget->OnContinueRequested.AddLambda([&bContinued] { bContinued = true; });
        ExperienceWidget->SetContinueReady(true);
        Slate->OnKeyDown(FGeometry(), FKeyEvent(EKeys::SpaceBar, FModifierKeysState(), 0, false, 0, 0));
        TestTrue(TEXT("Continue input works while combat remains paused"), bContinued);
        ExperienceWidget->OnContinueRequested.Clear();
    }
    World->GetTimerManager().ClearTimer(AttackTimer);
    GEngine->OnTravelFailure().Broadcast(World, ETravelFailure::InvalidURL, TEXT("Synthetic extraction failure"));
    TestFalse(TEXT("Travel failure releases combat pause"), World->IsPaused());
    TestFalse(TEXT("Travel failure restores player vulnerability"), Player->IsDamageInvulnerable());
    TestTrue(TEXT("Failed extraction can be retried"), Extraction->ExtractPawn(Player));
    Context.TravelURL.Empty();
    Extraction->Destroy();
    TestFalse(TEXT("Destroyed extraction actor releases its pause"), World->IsPaused());
    TestFalse(TEXT("Destroyed extraction actor restores damage state"), Player->IsDamageInvulnerable());
    // Preserve protection/pause that existed before this extraction owned anything.
    Player->SetCanBeDamaged(false);
    Controller->SetPause(true);
    auto* SecondExtraction = World->SpawnActor<ATunaSweeperExtractionPointActor>();
    TestTrue(TEXT("Already paused extraction is accepted"), SecondExtraction->ExtractPawn(Player));
    Context.TravelURL.Empty();
    SecondExtraction->Destroy();
    TestTrue(TEXT("Existing pause is not released by extraction cleanup"), World->IsPaused());
    TestFalse(TEXT("Existing invulnerability is preserved"), Player->CanBeDamaged());
    Controller->SetPause(false);
    Player->SetCanBeDamaged(true);
    return true;
}
#endif
