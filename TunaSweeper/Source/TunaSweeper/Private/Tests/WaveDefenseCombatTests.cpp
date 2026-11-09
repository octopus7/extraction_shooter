#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseStructureDamageTest, "TunaSweeper.WaveDefense.Combat.StructureDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseStructureDamageTest::RunTest(const FString& Parameters)
{
    const FGuid Run = FGuid::NewGuid();
    TestEqual(TEXT("Half difficulty"), FWaveDefenseDamageRules::Resolve(10.0f, 5000, Run, Run, true, true), 5.0f);
    TestEqual(TEXT("Normal difficulty"), FWaveDefenseDamageRules::Resolve(10.0f, 10000, Run, Run, true, true), 10.0f);
    TestEqual(TEXT("Double difficulty"), FWaveDefenseDamageRules::Resolve(10.0f, 20000, Run, Run, true, true), 20.0f);
    TestEqual(TEXT("Friendly damage rejected"), FWaveDefenseDamageRules::Resolve(10.0f, 10000, Run, Run, false, true), 0.0f);
    TestEqual(TEXT("Inactive attacker rejected"), FWaveDefenseDamageRules::Resolve(10.0f, 10000, Run, Run, true, false), 0.0f);
    TestEqual(TEXT("Other run rejected"), FWaveDefenseDamageRules::Resolve(10.0f, 10000, Run, FGuid::NewGuid(), true, true), 0.0f);
    TestEqual(TEXT("Invalid run rejected"), FWaveDefenseDamageRules::Resolve(10.0f, 10000, FGuid(), FGuid(), true, true), 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseRealCoreDamageTest, "TunaSweeper.WaveDefense.Combat.CoreDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseRealCoreDamageTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AWaveDefenseDirector* Director = World->SpawnActor<AWaveDefenseDirector>();
    AWaveDefenseCore* Core = World->SpawnActor<AWaveDefenseCore>();
    ATunaSweeperEnemyCharacter* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>();
    UWaveDefenseParticipantComponent* Participant = NewObject<UWaveDefenseParticipantComponent>(Enemy);
    Participant->RegisterComponent();
    const FGuid RunId = FGuid::NewGuid(), Token = FGuid::NewGuid();
    Participant->Initialize(Director, RunId, Token, EWaveDefenseEnemyRole::CoreAssault);
    auto* Definition = NewObject<UWaveDefenseDefinition>(); Definition->EncounterId = TEXT("test.wave");
    for (int32 Index = 0; Index < 6; ++Index)
    {
        FWaveDefenseWave Wave; Wave.AliveCap = 1;
        FWaveDefenseEnemyEntry Entry; Entry.EnemyClass = ATunaSweeperEnemyCharacter::StaticClass(); Entry.CombatProfile = TEXT("test.profile"); Entry.Count = 1;
        Wave.Enemies.Add(Entry); Definition->Waves.Add(Wave);
    }
    Director->RunId = RunId; Director->Run.Start(*Definition, RunId); Director->Run.AdvanceTime(45.0f); Director->Run.NotifySpawned(Token);
    Director->Enemies.Add(Token, Enemy);
    Director->DamageMultiplier = 5000;
    Core->SetDirector(Director);
    Core->ResetForRun(RunId, 1000);
    FDamageEvent DamageEvent;
    TestEqual(TEXT("Owned enemy deals scaled damage"), Core->TakeDamage(10.0f, DamageEvent, nullptr, Enemy), 5.0f);
    TestEqual(TEXT("Health reduced"), Core->GetHealth(), 995);
    Core->ResetForRun(FGuid::NewGuid(), 1000);
    TestEqual(TEXT("Different run blocked"), Core->TakeDamage(10.0f, DamageEvent, nullptr, Enemy), 0.0f);
    Director->Enemies.Reset();
    Core->ResetForRun(RunId, 1000);
    TestEqual(TEXT("Untracked enemy blocked"), Core->TakeDamage(10.0f, DamageEvent, nullptr, Enemy), 0.0f);
    World->DestroyWorld(false);
    return true;
}
#endif
