#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "WaveDefense/WaveDefenseFacility.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/DefaultPawn.h"
#include "Component/TunaSweeperFactionComponent.h"

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
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
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
    Director->Participants.Add(Token, Enemy);
    Director->DamageMultiplier = 5000;
    Director->Core = Core;
    Core->SetDirector(Director);
    Core->ResetForRun(RunId, 1000);
    FDamageEvent DamageEvent;
    TestEqual(TEXT("Owned enemy deals scaled damage"), Core->TakeDamage(10.0f, DamageEvent, nullptr, Enemy), 5.0f);
    TestEqual(TEXT("Health reduced"), Core->GetHealth(), 995);
    Enemy->SetActorLocation(FVector(0, 0, 100));
    Core->SetActorLocation(FVector(600, 0, 100));
    ADefaultPawn* Player = World->SpawnActor<ADefaultPawn>();
    Player->SetActorLocation(FVector(3000, 3000, 100));
    Director->PlayerPawn = Player;
    AWaveDefenseFacility* Barrier = World->SpawnActor<AWaveDefenseFacility>();
    Barrier->InitializeForRun(Director, RunId, TEXT("barrier"), EWaveDefenseFacilityKind::Barrier);
    Barrier->Configure(*Definition);
    Barrier->SetActorLocation(FVector(180, 0, 100));
    Director->Facilities.Add(TEXT("barrier"), Barrier);
    TestTrue(TEXT("Nearby blocking barrier is prioritized"), Director->GetPreferredTarget(Enemy, RunId, Token, EWaveDefenseEnemyRole::CoreAssault) == Barrier);
    Director->Facilities.Reset();
    Barrier->SetActorEnableCollision(false);
    TestTrue(TEXT("Ranged attacker without player sight approaches core"), Director->GetPreferredTarget(Enemy, RunId, Token, EWaveDefenseEnemyRole::PlayerPressure) == Core);
    AActor* Projectile = World->SpawnActor<AActor>();
    Projectile->SetInstigator(Enemy);
    auto* ProjectileParticipant = NewObject<UWaveDefenseParticipantComponent>(Projectile);
    ProjectileParticipant->Initialize(Director, RunId, Token, EWaveDefenseEnemyRole::CoreAssault); ProjectileParticipant->RegisterComponent();
    auto* ProjectileFaction = NewObject<UTunaSweeperFactionComponent>(Projectile);
    ProjectileFaction->SetFactionId(TunaSweeperFactionIds::Enemy); ProjectileFaction->RegisterComponent();
    Director->RegisterOwnedActor(Projectile);
    Enemy->TakeDamage(100000.0f, DamageEvent, nullptr, nullptr);
    Core->ResetForRun(RunId, 1000);
    TestEqual(TEXT("Already fired projectile remains owned after enemy death"), Core->TakeDamage(10.0f, DamageEvent, nullptr, Projectile), 5.0f);
    Core->ResetForRun(FGuid::NewGuid(), 1000);
    TestEqual(TEXT("Different run blocked"), Core->TakeDamage(10.0f, DamageEvent, nullptr, Enemy), 0.0f);
    Director->Enemies.Reset();
    Director->Participants.Reset();
    Core->ResetForRun(RunId, 1000);
    TestEqual(TEXT("Untracked enemy blocked"), Core->TakeDamage(10.0f, DamageEvent, nullptr, Enemy), 0.0f);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
