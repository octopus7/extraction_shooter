#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestLevelTravelPathsTest,
    "TunaSweeper.Quest.LevelTravel.MapPaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestLevelTravelPathsTest::RunTest(const FString& Parameters)
{
    const auto Check = [&](const TCHAR* Source, const TCHAR* Target, bool Expected)
    {
        TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>());
        TStrongObjectPtr<UTunaSweeperQuestSubsystem> Quests(NewObject<UTunaSweeperQuestSubsystem>(Game.Get()));
        Quests->bQuestDataLoaded = true;
        FTunaSweeperQuestDefinition Definition;
        Definition.QuestId = 901;
        FTunaSweeperObjectiveDefinition Objective;
        Objective.ObjectiveId = TEXT("leave_base");
        Objective.Type = ETunaSweeperObjectiveType::LevelTravel;
        Objective.SourceLevelName = TEXT("BunkerMap");
        Objective.TargetLevelName = TEXT("RaidForest");
        Objective.RequiredCount = 1;
        Definition.Objectives.Add(Objective);
        Quests->QuestDefinitions.Add(901, Definition);
        Quests->SetQuestState(901, ETunaSweeperQuestState::Accepted);
        Quests->NotifyLevelTravelRequested(Source, Target);
        TestEqual(FString::Printf(TEXT("%s -> %s"), Source, Target), Quests->CanClaimQuestReward(901), Expected);
        Quests->NotifyLevelTravelRequested(Source, Target);
        TestEqual(TEXT("Repeated travel does not exceed the objective count"),
            Quests->GetObjectiveProgressCount(901, TEXT("leave_base")), Expected ? 1 : 0);
    };
    Check(TEXT("BunkerMap"), TEXT("RaidForest"), true);
    Check(TEXT("BunkerMap"), TEXT("/Game/MainRaid/RaidForest"), true);
    Check(TEXT("/Game/Maps/UEDPIE_0_BunkerMap"), TEXT("/Game/MainRaid/UEDPIE_0_RaidForest"), true);
    Check(TEXT("UEDPIE_2_BunkerMap"), TEXT("RaidForest"), true);
    Check(TEXT("BunkerMap"), TEXT("/Game/MainRaid/RaidMap"), false);
    Check(TEXT("BunkerMap"), TEXT("/Game/MainRaid/RaidVillage"), false);
    Check(TEXT("Other_BunkerMap"), TEXT("RaidForest"), false);
    Check(TEXT("BunkerMap"), TEXT("Other_RaidForest"), false);
    Check(TEXT("BunkerMap"), TEXT("None"), false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaAchievementRegionEntryTest,
    "TunaSweeper.Achievement.RegionEntry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaAchievementRegionEntryTest::RunTest(const FString& Parameters)
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
    };
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>());
    TStrongObjectPtr<UTunaSweeperAchievementSubsystem> Subject(NewObject<UTunaSweeperAchievementSubsystem>(Game.Get()));
    Subject->bRefreshingSnapshot = true; // No player inventory/save initialization or online publishing.
    if (!TestTrue(TEXT("Load authored region definitions"), Subject->LoadAchievementDefinitions())) return false;
    const auto Visit = [&](const TCHAR* Map, const TCHAR* Achievement)
    {
        const FString WorldName = FString(TEXT("UEDPIE_71_")) + Map;
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(*WorldName),
            CreatePackage(*(FString(TEXT("/Game/MainRaid/")) + WorldName)));
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        FContextAccess::Attach(Game.Get(), &Context);
        World->SetGameInstance(Game.Get());
        World->StreamingLevelsPrefix = TEXT("UEDPIE_71_");
        ON_SCOPE_EXIT
        {
            World->SetGameInstance(nullptr); FContextAccess::Attach(Game.Get(), nullptr);
            World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
        };
        Subject->HandleGameplayWorldReady(World);
        if (Achievement)
        {
            TestTrue(FString::Printf(TEXT("Arrival at %s unlocks %s"), Map, Achievement), Subject->IsAchievementUnlocked(Achievement));
            const int32 Notifications = Subject->PendingNotifications.Num();
            Subject->HandleGameplayWorldReady(World);
            TestEqual(TEXT("Same world-ready event is idempotent"), Subject->PendingNotifications.Num(), Notifications);
        }
    };
    Visit(TEXT("RaidMap"), nullptr);
    TestFalse(TEXT("Development map does not unlock forest entry"), Subject->IsAchievementUnlocked(TEXT("01")));
    Visit(TEXT("Other_RaidForest"), nullptr);
    TestFalse(TEXT("Similarly named map does not unlock forest entry"), Subject->IsAchievementUnlocked(TEXT("01")));
    Visit(TEXT("RaidForest"), TEXT("01"));
    TestFalse(TEXT("Forest does not unlock village"), Subject->IsAchievementUnlocked(TEXT("07")));
    Visit(TEXT("RaidVillage"), TEXT("07"));
    TestFalse(TEXT("Village does not unlock plains"), Subject->IsAchievementUnlocked(TEXT("19")));
    Visit(TEXT("RaidPlains"), TEXT("19"));
    return true;
}
#endif
