#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Game/TunaSweeperRaidPlayerStart.h"
#include "Game/TunaSweeperGameMode.h"
#include "Subsystem/TunaSweeperRaidArrivalSubsystem.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "Interaction/TunaSweeperRegionLinkActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "UObject/StrongObjectPtr.h"

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
    auto* Passage = World->SpawnActor<ATunaSweeperRegionLinkActor>(Spawn);
    TestTrue(TEXT("Ordinary passages preserve default arrival"), Passage->GetArrivalGroup() == ETunaSweeperRaidArrivalGroup::None);
    Passage->ArrivalGroup = ETunaSweeperRaidArrivalGroup::Plains;
    Pending.Prepare(TEXT("RaidPlains"), Passage->GetArrivalGroup());
    auto* South = World->SpawnActor<ATunaSweeperRaidPlayerStart>(Spawn);
    South->ArrivalGroup = ETunaSweeperRaidArrivalGroup::Plains;
    FRandomStream SouthRandom(60);
    const TArray<APlayerStart*> SouthStarts = {South, Ordinary};
    TestEqual(TEXT("Region link requests the sole southern lift start, not the ordinary start"),
        UTunaSweeperRaidArrivalSubsystem::SelectRandomStart(SouthStarts, Pending.Consume(TEXT("RaidPlains")), SouthRandom), static_cast<APlayerStart*>(South));
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperLiftAchievementTest, "TunaSweeper.Region.LiftAchievement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperLiftAchievementTest::RunTest(const FString& Parameters)
{
    const FString TestRoot = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense/AchievementHooks") / FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Isolated achievement save root"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(TestRoot))) return false;
    ON_SCOPE_EXIT { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); IFileManager::Get().DeleteDirectory(*TestRoot, false, true); };
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FContextAccess::Attach(Game.Get(), &Context); World->SetGameInstance(Game.Get());
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT { Game->UGameInstance::Shutdown(); FContextAccess::Attach(Game.Get(), nullptr);
        World->SetGameInstance(nullptr); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
    auto* Achievements = Game->GetSubsystem<UTunaSweeperAchievementSubsystem>();
    auto* Arrival = Game->GetSubsystem<UTunaSweeperRaidArrivalSubsystem>();
    auto* Mode = World->SpawnActor<ATunaSweeperGameMode>();
    auto* Controller = World->SpawnActor<APlayerController>();
    Controller->SetAsLocalPlayerController();
    auto* Start = World->SpawnActor<ATunaSweeperRaidPlayerStart>();
    Start->ArrivalGroup = ETunaSweeperRaidArrivalGroup::Forest;
    Start->SetActorLocation(FVector(2000, 0, 150));
    Arrival->PrepareArrival(TEXT("WrongMap"), ETunaSweeperRaidArrivalGroup::Forest);
    Mode->ChoosePlayerStart_Implementation(Controller);
    TestFalse(TEXT("Failed or wrong lift destination does not unlock 04"), Achievements->IsAchievementUnlocked(TEXT("04")));
    Arrival->PrepareArrival(FName(*World->GetMapName()), ETunaSweeperRaidArrivalGroup::Forest);
    TestFalse(TEXT("Choosing a lift destination is not use completion"), Achievements->IsAchievementUnlocked(TEXT("04")));
    Mode->RestartPlayer(Controller);
    TestTrue(TEXT("Lift arrival spawned the player"), Controller->GetPawn() != nullptr);
    TestTrue(TEXT("Successful lift arrival unlocks 04"), Achievements->IsAchievementUnlocked(TEXT("04")));
    return true;
}
#endif
