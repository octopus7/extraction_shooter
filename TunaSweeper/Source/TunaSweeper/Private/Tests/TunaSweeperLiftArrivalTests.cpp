#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Game/TunaSweeperRaidPlayerStart.h"
#include "Game/TunaSweeperGameMode.h"
#include "Subsystem/TunaSweeperRaidArrivalSubsystem.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperLiftArrivalTest, "TunaSweeper.Region.LiftArrival",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperLiftArrivalTest::RunTest(const FString& Parameters)
{
    FTunaSweeperPendingRaidArrival Pending;
    Pending.Prepare(TEXT("/Game/Maps/Village"), ETunaSweeperRaidArrivalGroup::VillageEast);
    TestTrue(TEXT("PIE destination consumes east request"),
        Pending.Consume(TEXT("UEDPIE_0_Village")) == ETunaSweeperRaidArrivalGroup::VillageEast);
    TestTrue(TEXT("Request consumed only once"), Pending.Consume(TEXT("Village")) == ETunaSweeperRaidArrivalGroup::None);
    Pending.Prepare(TEXT("Village"), ETunaSweeperRaidArrivalGroup::VillageWest);
    TestTrue(TEXT("Wrong map discards pending request"), Pending.Consume(TEXT("Forest")) == ETunaSweeperRaidArrivalGroup::None);
    TestTrue(TEXT("Discarded request cannot leak to later trip"), Pending.Consume(TEXT("Village")) == ETunaSweeperRaidArrivalGroup::None);
    Pending.Prepare(TEXT("Village"), ETunaSweeperRaidArrivalGroup::VillageEast);
    Pending.Prepare(TEXT("Village"), ETunaSweeperRaidArrivalGroup::None);
    TestTrue(TEXT("Ordinary travel clears lift choice"), Pending.Consume(TEXT("Village")) == ETunaSweeperRaidArrivalGroup::None);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); World->RemoveFromRoot(); };
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* EastA = World->SpawnActor<ATunaSweeperRaidPlayerStart>(Spawn);
    auto* EastB = World->SpawnActor<ATunaSweeperRaidPlayerStart>(Spawn);
    auto* West = World->SpawnActor<ATunaSweeperRaidPlayerStart>(Spawn);
    auto* Ordinary = World->SpawnActor<APlayerStart>(Spawn);
    EastA->ArrivalGroup = EastB->ArrivalGroup = ETunaSweeperRaidArrivalGroup::VillageEast;
    West->ArrivalGroup = ETunaSweeperRaidArrivalGroup::VillageWest;
    const TArray<APlayerStart*> Starts = {EastA, EastB, West, Ordinary};
    FRandomStream Random(420);
    TSet<APlayerStart*> Seen;
    for (int32 Index = 0; Index < 100; ++Index)
    {
        APlayerStart* Selected = UTunaSweeperRaidArrivalSubsystem::SelectRandomStart(Starts, ETunaSweeperRaidArrivalGroup::VillageEast, Random);
        TestTrue(TEXT("East excludes west and ordinary starts"), Selected == EastA || Selected == EastB);
        Seen.Add(Selected);
    }
    TestEqual(TEXT("Random selection reaches both east starts"), Seen.Num(), 2);
    TestEqual(TEXT("West group remains disjoint"), UTunaSweeperRaidArrivalSubsystem::SelectRandomStart(Starts, ETunaSweeperRaidArrivalGroup::VillageWest, Random), static_cast<APlayerStart*>(West));
    TestNull(TEXT("Missing group never selects a different group"), UTunaSweeperRaidArrivalSubsystem::SelectRandomStart(Starts, ETunaSweeperRaidArrivalGroup::Plains, Random));
    TestNull(TEXT("Ordinary trip never selects dedicated lift start"), UTunaSweeperRaidArrivalSubsystem::SelectRandomStart(Starts, ETunaSweeperRaidArrivalGroup::None, Random));
    auto* Mode = World->SpawnActor<ATunaSweeperGameMode>(Spawn);
    for (int32 Index = 0; Index < 20; ++Index)
    {
        TestEqual(TEXT("Actual ordinary GameMode selection excludes dedicated lift starts"),
            Mode->ChoosePlayerStart_Implementation(nullptr), static_cast<AActor*>(Ordinary));
    }
    return true;
}
#endif
