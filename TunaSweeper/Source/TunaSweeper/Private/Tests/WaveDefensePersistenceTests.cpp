#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "Component/TunaSweeperFactionComponent.h"

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
    FGuid PendingRun;
    TestTrue(TEXT("Start second run"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, PendingRun));
    TestTrue(TEXT("Save pending reward in slot A"), Game->TryFinalizeWaveDefenseRun(Encounter, PendingRun, EWaveDefenseOutcome::Succeeded, 6));
    TestTrue(TEXT("Switch to slot B"), Game->SetActiveSaveSlotIndex(2));
    Game->GetInventorySlots();
    TestFalse(TEXT("Fresh slot B does not inherit A reward"), Game->WaveDefenseProgress.Contains(Encounter));
    TestTrue(TEXT("Fresh slot B can start"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, Other));
    Game->bWaveDefenseTestFailSave = true;
    TestFalse(TEXT("Abort write failure"), Game->TryFinalizeWaveDefenseRun(Encounter, Other, EWaveDefenseOutcome::Abandoned, 0));
    Game->StageWaveDefenseInterrupted(Encounter, Other);
    FGuid RetryRun;
    TestFalse(TEXT("Cannot restart while interrupted state cannot be saved"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, RetryRun));
    Game->bWaveDefenseTestFailSave = false;
    TestTrue(TEXT("Restart retries interrupted save"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, RetryRun));
    Game->ResetRuntimeStateForSaveSlotSelection();
    TestTrue(TEXT("Slot reset clears wave state"), Game->WaveDefenseProgress.IsEmpty());
    Game->WaveDefenseProgress.Add(Encounter, FWaveDefenseProgress());
    Game->bWaveDefenseInterruptedNeedsSave = true;
    Game->GenerateDefaultInventoryState();
    TestTrue(TEXT("New game clears wave state"), Game->WaveDefenseProgress.IsEmpty());
    TestFalse(TEXT("New game clears interrupted save flag"), Game->bWaveDefenseInterruptedNeedsSave);
    Game->bInventoryStateInitialized = true;
    auto* Definition = NewObject<UWaveDefenseDefinition>(); Definition->EncounterId = Encounter;
    for (int32 Index = 0; Index < 6; ++Index)
    {
        FWaveDefenseWave Wave; Wave.AliveCap = 1;
        FWaveDefenseEnemyEntry Entry; Entry.EnemyClass = ATunaSweeperEnemyCharacter::StaticClass(); Entry.CombatProfile = TEXT("test.profile"); Entry.Count = 1;
        Wave.Enemies.Add(Entry); Definition->Waves.Add(Wave);
    }
    FGuid CombatRun;
    TestTrue(TEXT("Start lifecycle fixture"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, CombatRun));
    auto* Director = World->SpawnActor<AWaveDefenseDirector>();
    auto* Core = World->SpawnActor<AWaveDefenseCore>();
    auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>();
    auto* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>();
    Director->Definition = Definition; Director->Core = Core; Director->PlayerPawn = Player; Director->RunId = CombatRun;
    Core->ResetForRun(CombatRun, 0);
    TestTrue(TEXT("Lifecycle run initialized"), Director->Run.Start(*Definition, CombatRun));
    Director->Run.AdvanceTime(45.0f);
    const FGuid EnemyToken = FGuid::NewGuid();
    TestTrue(TEXT("Lifecycle enemy tracked"), Director->Run.NotifySpawned(EnemyToken));
    Director->Enemies.Add(EnemyToken, Enemy); Director->OwnedEnemies.Add(Enemy);
    Director->Run.ResolveFrame(false, true);
    Game->bWaveDefenseTestFailSave = true;
    Director->Tick(0.01f);
    TestTrue(TEXT("Failed result save stops encounter actors"), Director->Enemies.IsEmpty() && Director->OwnedEnemies.IsEmpty());
    TestTrue(TEXT("Failed result remains retryable"), Director->GetSnapshot().bResultSavePending);
    Game->bWaveDefenseTestFailSave = false;
    Director->Tick(0.01f);
    TestEqual(TEXT("Result finishes after save retry"), Director->GetSnapshot().Phase, EWaveDefensePhase::Finished);
    TestEqual(TEXT("Core failure persisted"), Game->FindWaveDefenseProgress(Encounter)->Outcome, EWaveDefenseOutcome::CoreDestroyed);
    FGuid FinalRun;
    TestTrue(TEXT("Start same-frame fixture"), Game->TryBeginWaveDefenseRun(Encounter, 300, 1, FinalRun));
    Director->Run = FWaveDefenseRun(); Director->RunId = FinalRun; Director->PlayerPawn = Player;
    Director->bTerminalCleanupDone = false; Director->SuccessCandidateFrame = 0; Director->LastCompletedWave = 5;
    Core->SetDirector(Director); Core->ResetForRun(FinalRun, 5);
    TestTrue(TEXT("Initialize same-frame run"), Director->Run.Start(*Definition, FinalRun));
    Director->Run.AdvanceTime(45.0f);
    for (int32 Wave = 0; Wave < 5; ++Wave)
    {
        const FGuid Token = FGuid::NewGuid(); Director->Run.NotifySpawned(Token); Director->Run.NotifyEnemyDied(Token);
        Director->Run.ResolveFrame(false, false); Director->Run.AdvanceTime(30.0f);
    }
    auto* FinalEnemy = World->SpawnActor<ATunaSweeperEnemyCharacter>();
    const FGuid FinalToken = FGuid::NewGuid();
    auto* Participant = NewObject<UWaveDefenseParticipantComponent>(FinalEnemy);
    Participant->Initialize(Director, FinalRun, FinalToken, EWaveDefenseEnemyRole::CoreAssault); Participant->RegisterComponent();
    Director->Enemies.Add(FinalToken, FinalEnemy); Director->Participants.Add(FinalToken, FinalEnemy); Director->OwnedEnemies.Add(FinalEnemy);
    Director->Run.NotifySpawned(FinalToken);
    auto* Shot = World->SpawnActor<AActor>(); Shot->SetInstigator(FinalEnemy);
    auto* ShotParticipant = NewObject<UWaveDefenseParticipantComponent>(Shot);
    ShotParticipant->Initialize(Director, FinalRun, FinalToken, EWaveDefenseEnemyRole::CoreAssault); ShotParticipant->RegisterComponent();
    auto* ShotFaction = NewObject<UTunaSweeperFactionComponent>(Shot); ShotFaction->SetFactionId(TunaSweeperFactionIds::Enemy); ShotFaction->RegisterComponent();
    Director->RegisterOwnedActor(Shot);
    FinalEnemy->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
    Director->Tick(0.01f);
    TestEqual(TEXT("Success is only a candidate in the killing frame"), Director->GetSnapshot().Phase, EWaveDefensePhase::Resolving);
    TestEqual(TEXT("Last projectile can destroy core in same frame"), Core->TakeDamage(10.0f, FDamageEvent(), nullptr, Shot), 5.0f);
    Director->Tick(0.01f);
    TestEqual(TEXT("Same-frame core death overrides success"), Director->GetSnapshot().Outcome, EWaveDefenseOutcome::CoreDestroyed);
    TestFalse(TEXT("Same-frame failure grants no reward"), Game->FindWaveDefenseProgress(Encounter)->bRewardPending);
    return true;
}
#endif
