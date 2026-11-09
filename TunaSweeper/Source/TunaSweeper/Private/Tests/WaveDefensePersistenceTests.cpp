#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefensePersistenceTest, "TunaSweeper.WaveDefense.Persistence.Transactions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefensePersistenceTest::RunTest(const FString& Parameters)
{
    const FString TestRoot = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") / FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Temporary root accepted"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(TestRoot))) return false;
    ON_SCOPE_EXIT { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); };
    struct FContextAccess : UGameInstance {
        static void Attach(UGameInstance* Game, FWorldContext* Context) { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context); Context.OwningGameInstance = Game; World->SetGameInstance(Game);
    Game->bInventoryStateInitialized = true;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        FContextAccess::Attach(Game, nullptr); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Quest subsystem"), Quests)) return false;
    const FName Encounter(TEXT("test.wave"));
    FGuid Run;
    Game->bWaveDefenseTestFailSave = true;
    TestFalse(TEXT("Start fails closed on save error"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, Run));
    TestFalse(TEXT("Failed start leaves no record"), Game->WaveDefenseProgress.Contains(Encounter));
    Game->bWaveDefenseTestFailSave = false;
    TestTrue(TEXT("Start persists"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, Run));
    FGuid Other;
    TestFalse(TEXT("Duplicate start rejected"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, Other));
    Game->bWaveDefenseTestFailSave = true;
    TestFalse(TEXT("Finalize fails closed"), Game->TryFinalizeWaveDefenseRun(Encounter, Run, EWaveDefenseOutcome::Succeeded, 6));
    TestEqual(TEXT("Failure restores active outcome"), Game->FindWaveDefenseProgress(Encounter)->Outcome, EWaveDefenseOutcome::None);
    Game->bWaveDefenseTestFailSave = false;
    TestTrue(TEXT("Finalize succeeds"), Game->TryFinalizeWaveDefenseRun(Encounter, Run, EWaveDefenseOutcome::Succeeded, 6));
    TestTrue(TEXT("Finalize idempotent"), Game->TryFinalizeWaveDefenseRun(Encounter, Run, EWaveDefenseOutcome::Succeeded, 6));
    TestEqual(TEXT("Success counted once"), Game->FindWaveDefenseProgress(Encounter)->CompletedRuns, 1);
    const int32 Before = Quests->GetCoinBalance();
    Game->bWaveDefenseTestFailSave = true;
    TestFalse(TEXT("Claim save failure"), Game->TryClaimWaveDefenseReward(Encounter, Run));
    TestEqual(TEXT("Claim rolls back coins"), Quests->GetCoinBalance(), Before);
    TestTrue(TEXT("Claim rolls back flag"), Game->FindWaveDefenseProgress(Encounter)->bRewardPending);
    Game->bWaveDefenseTestFailSave = false;
    TestTrue(TEXT("Claim succeeds"), Game->TryClaimWaveDefenseReward(Encounter, Run));
    TestEqual(TEXT("Coins granted once"), Quests->GetCoinBalance(), Before + 300);
    TestFalse(TEXT("Repeated claim rejected"), Game->TryClaimWaveDefenseReward(Encounter, Run));
    Game->WaveDefenseProgress.Reset();
    TestTrue(TEXT("Reload saved data"), Game->LoadGameState());
    TestFalse(TEXT("Reload keeps claimed flag"), Game->FindWaveDefenseProgress(Encounter)->bRewardPending);
    TestEqual(TEXT("Reload preserves coins"), Quests->GetCoinBalance(), Before + 300);
    return true;
}
#endif
