#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "WaveDefense/WaveDefenseRun.h"
#include "WaveDefense/WaveDefenseDefinition.h"

namespace WaveDefenseRulesTests
{
    UWaveDefenseDefinition* MakeDefinition()
    {
        UWaveDefenseDefinition* Definition = NewObject<UWaveDefenseDefinition>();
        Definition->EncounterId = TEXT("test.wave_defense");
        const int32 Counts[] = {6, 8, 10, 12, 14, 16};
        const int32 Caps[] = {4, 5, 6, 7, 8, 8};
        for (int32 Index = 0; Index < 6; ++Index)
        {
            FWaveDefenseWave Wave;
            Wave.AliveCap = Caps[Index];
            FWaveDefenseEnemyEntry Enemy;
            Enemy.EnemyClass = ATunaSweeperEnemyCharacter::StaticClass();
            Enemy.CombatProfile = TEXT("test.profile");
            Enemy.WeaponId = 1;
            Enemy.AmmoId = 1;
            Enemy.Count = Counts[Index];
            Wave.Enemies.Add(Enemy);
            Definition->Waves.Add(Wave);
        }
        return Definition;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseRulesTest, "TunaSweeper.WaveDefense.Rules.StateTransitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseRulesTest::RunTest(const FString& Parameters)
{
    using namespace WaveDefenseRulesTests;
    FWaveDefenseRun Invalid;
    TestFalse(TEXT("Empty definition rejected"), Invalid.Start(*NewObject<UWaveDefenseDefinition>(), FGuid::NewGuid()));
    UWaveDefenseDefinition* Definition = MakeDefinition();
    TArray<FName> ErrorKeys;
    TestTrue(TEXT("Valid definition accepted"), Definition->ValidateDefinition(ErrorKeys));
    Definition->Waves[0].Enemies[0].Count = 0;
    TestFalse(TEXT("Zero count rejected"), Definition->ValidateDefinition(ErrorKeys));
    TestTrue(TEXT("Error is a localization key"), ErrorKeys.Contains(TEXT("ui.wave_defense.invalid_definition")));
    Definition->Waves[0].Enemies[0].Count = 6;
    FWaveDefenseRun Run;
    TestTrue(TEXT("Start valid run"), Run.Start(*Definition, FGuid::NewGuid()));
    TestFalse(TEXT("Repeated start rejected"), Run.Start(*Definition, FGuid::NewGuid()));
    TestEqual(TEXT("Initial supplies"), Run.GetSnapshot().Supplies, 200);
    Run.AdvanceTime(-1.0f);
    TestEqual(TEXT("Negative delta ignored"), Run.GetSnapshot().SecondsRemaining, 45.0f);
    Run.AdvanceTime(45.0f);
    TestEqual(TEXT("Preparation ends"), Run.GetSnapshot().Phase, EWaveDefensePhase::Active);
    Run.ResolveFrame(false, false);
    TestEqual(TEXT("Queued enemies prevent clear"), Run.GetSnapshot().Phase, EWaveDefensePhase::Active);
    int32 Total = 0;
    const int32 Counts[] = {6, 8, 10, 12, 14, 16};
    const int32 Caps[] = {4, 5, 6, 7, 8, 8};
    for (int32 WaveIndex = 0; WaveIndex < 6; ++WaveIndex)
    {
        TestEqual(TEXT("Queued wave size"), Run.GetSnapshot().Queued, Counts[WaveIndex]);
        TArray<FGuid> Active;
        for (int32 Index = 0; Index < Caps[WaveIndex]; ++Index)
        {
            const FGuid Token = FGuid::NewGuid();
            TestTrue(TEXT("Spawn accepted below cap"), Run.NotifySpawned(Token));
            Active.Add(Token);
        }
        TestFalse(TEXT("Spawn rejected at cap"), Run.NotifySpawned(FGuid::NewGuid()));
        for (const FGuid Token : Active)
        {
            TestTrue(TEXT("Death accepted once"), Run.NotifyEnemyDied(Token));
            TestFalse(TEXT("Duplicate death rejected"), Run.NotifyEnemyDied(Token));
        }
        for (int32 Index = Caps[WaveIndex]; Index < Counts[WaveIndex]; ++Index)
        {
            const FGuid Token = FGuid::NewGuid();
            TestTrue(TEXT("Remaining spawn accepted"), Run.NotifySpawned(Token));
            TestTrue(TEXT("Remaining death accepted"), Run.NotifyEnemyDied(Token));
        }
        Total += Counts[WaveIndex];
        Run.ResolveFrame(false, false);
        if (WaveIndex < 5)
        {
            TestEqual(TEXT("Intermission"), Run.GetSnapshot().Phase, EWaveDefensePhase::Intermission);
            TestEqual(TEXT("Wave supplies"), Run.GetSnapshot().Supplies, 200 + 100 * (WaveIndex + 1));
            Run.AdvanceTime(30.0f);
            TestEqual(TEXT("Next wave begins"), Run.GetSnapshot().Phase, EWaveDefensePhase::Active);
        }
        else TestEqual(TEXT("Final wave resolves"), Run.GetSnapshot().Phase, EWaveDefensePhase::Resolving);
    }
    TestEqual(TEXT("Total enemies"), Total, 66);
    FWaveDefenseRun Failure;
    TestTrue(TEXT("Second run starts"), Failure.Start(*Definition, FGuid::NewGuid()));
    Failure.AdvanceTime(45.0f);
    Failure.ResolveFrame(true, true);
    TestEqual(TEXT("Player death takes precedence"), Failure.GetSnapshot().Outcome, EWaveDefenseOutcome::PlayerDied);
    return true;
}
#endif

