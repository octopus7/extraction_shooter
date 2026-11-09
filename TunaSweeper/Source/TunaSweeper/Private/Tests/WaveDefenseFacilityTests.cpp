#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseFacility.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseFacilityTest, "TunaSweeper.WaveDefense.Facility.Actions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseFacilityTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AWaveDefenseDirector* Director = World->SpawnActor<AWaveDefenseDirector>();
    Director->Definition = NewObject<UWaveDefenseDefinition>();
    Director->Definition->EncounterId = TEXT("test.wave");
    for (int32 Index = 0; Index < 6; ++Index)
    {
        FWaveDefenseWave Wave; Wave.AliveCap = 1;
        FWaveDefenseEnemyEntry Entry; Entry.EnemyClass = ATunaSweeperEnemyCharacter::StaticClass(); Entry.CombatProfile = TEXT("test.profile"); Entry.WeaponId = 1; Entry.AmmoId = 1; Entry.Count = 1;
        Wave.Enemies.Add(Entry); Director->Definition->Waves.Add(Wave);
    }
    Director->RunId = FGuid::NewGuid();
    Director->Core = World->SpawnActor<AWaveDefenseCore>();
    Director->Core->ResetForRun(Director->RunId, 1000);
    Director->BarrierClass = AWaveDefenseFacility::StaticClass();
    Director->TurretClass = AWaveDefenseFacility::StaticClass();
    FWaveDefenseFacilitySlot Barrier; Barrier.SlotId = TEXT("barrier"); Barrier.Kind = EWaveDefenseFacilityKind::Barrier; Director->FacilitySlots.Add(Barrier);
    FWaveDefenseFacilitySlot Turret; Turret.SlotId = TEXT("turret"); Turret.Kind = EWaveDefenseFacilityKind::Turret; Turret.LocalTransform.SetLocation(FVector(500,0,0)); Director->FacilitySlots.Add(Turret);
    TestTrue(TEXT("Start state"), Director->Run.Start(*Director->Definition, Director->RunId));
    TestFalse(TEXT("Full core repair rejected"), Director->TryFacilityAction(TEXT("core"), EWaveDefenseAction::RepairCore));
    TestTrue(TEXT("Barrier installed"), Director->TryFacilityAction(TEXT("barrier"), EWaveDefenseAction::InstallBarrier));
    TestEqual(TEXT("Barrier cost"), Director->GetSnapshot().Supplies, 150);
    TestFalse(TEXT("Duplicate slot rejected"), Director->TryFacilityAction(TEXT("barrier"), EWaveDefenseAction::InstallBarrier));
    TestTrue(TEXT("Turret installed"), Director->TryFacilityAction(TEXT("turret"), EWaveDefenseAction::InstallTurret));
    TestEqual(TEXT("Turret cost"), Director->GetSnapshot().Supplies, 0);
    TestFalse(TEXT("Full ammo reload rejected"), Director->TryFacilityAction(TEXT("turret"), EWaveDefenseAction::ReloadTurret));
    Director->SetReady();
    TestFalse(TEXT("Active phase purchases rejected"), Director->TryFacilityAction(TEXT("core"), EWaveDefenseAction::RepairCore));
    Director->CleanupOwnedActors();
    Director->RunId.Invalidate();
    World->DestroyWorld(false);
    return true;
}
#endif
