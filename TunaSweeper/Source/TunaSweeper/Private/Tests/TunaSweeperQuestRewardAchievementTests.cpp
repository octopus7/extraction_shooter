#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaQuestRewardAchievementsTest,
    "TunaSweeper.Achievement.QuestRewardCompletion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaQuestRewardAchievementsTest::RunTest(const FString& Parameters)
{
    const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/WaveDefense/QuestAchievements")/FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Isolated save root"),TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
    ON_SCOPE_EXIT { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); IFileManager::Get().DeleteDirectory(*Root,false,true); };
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance,FWorldContext* Context)
        { auto Member=&FContextAccess::WorldContext; Instance->*Member=Context; }
    };
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    FWorldContext& Context=GEngine->CreateNewWorldContext(EWorldType::Game);Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FContextAccess::Attach(Game.Get(),&Context);World->SetGameInstance(Game.Get());Context.OwningGameInstance=Game.Get();
    Game->bInventoryStateInitialized=true;Game->UGameInstance::Init();
    ON_SCOPE_EXIT { Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);Context.OwningGameInstance=nullptr;
        FContextAccess::Attach(Game.Get(),nullptr);World->DestroyWorld(false);GEngine->DestroyWorldContext(World);World->RemoveFromRoot(); };
    Game->ResetPlayerSlotArrays();Game->ActiveSaveSlotIndex=1;Game->bActiveSaveSlotDifficultySelected=true;
    auto* Quests=Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    auto* Achievements=Game->GetSubsystem<UTunaSweeperAchievementSubsystem>();
    Quests->QuestDefinitions.Reset();Quests->QuestProgressById.Reset();Quests->bQuestDataLoaded=true;
    for (int32 Id : {4,11,12,13,14,24,900777})
    {
        FTunaSweeperQuestDefinition D;D.QuestId=Id;D.bCompleteOnAccept=true;D.Rewards.Coins=3;
        Quests->QuestDefinitions.Add(Id,D);
    }
    TestTrue(TEXT("Unrelated reward quest accepted"),Quests->AcceptQuest(900777));
    TestTrue(TEXT("Unrelated reward claimed"),Quests->ClaimQuestReward(900777));
    TestFalse(TEXT("Unrelated reward does not unlock 03"),Achievements->IsAchievementUnlocked(TEXT("03")));
    TestFalse(TEXT("Unrelated reward does not unlock 08"),Achievements->IsAchievementUnlocked(TEXT("08")));
    TestTrue(TEXT("Boar quest accepted"),Quests->AcceptQuest(4));
    TestEqual(TEXT("Boar objectives complete, reward still pending"),Quests->GetQuestState(4),ETunaSweeperQuestState::RewardAvailable);
    TestFalse(TEXT("Objectives alone do not unlock 03"),Achievements->IsAchievementUnlocked(TEXT("03")));
    Game->bSuppressSaveWritesForTests=true;
    TestTrue(TEXT("Reward reaches runtime completed state"),Quests->ClaimQuestReward(4));
    TestFalse(TEXT("Failed gameplay save does not unlock 03"),Achievements->IsAchievementUnlocked(TEXT("03")));
    Game->bSuppressSaveWritesForTests=false;
    TestTrue(TEXT("Retry saves completed reward"),Game->SaveGameStateInternal());
    TestTrue(TEXT("Committed boar reward unlocks 03"),Achievements->IsAchievementUnlocked(TEXT("03")));
    const int32 Coins=Quests->GetCoinBalance();
    TestFalse(TEXT("Cannot claim reward twice"),Quests->ClaimQuestReward(4));
    TestEqual(TEXT("Achievement reconciliation does not grant more coins"),Quests->GetCoinBalance(),Coins);
    // A legacy save may have completed relocation before the achievement was enabled.
    TArray<FTunaSweeperNumericQuestProgressSaveData> Saved;
    FTunaSweeperNumericQuestProgressSaveData Legacy;Legacy.QuestId=24;Legacy.State=ETunaSweeperQuestState::RewardAvailable;Saved.Add(Legacy);
    Quests->LoadQuestProgressFromSave(Saved,0,Coins);
    TestFalse(TEXT("Loaded unclaimed reward does not unlock 08"),Achievements->IsAchievementUnlocked(TEXT("08")));
    Saved[0].State=ETunaSweeperQuestState::RewardCompleted;
    Quests->LoadQuestProgressFromSave(Saved,0,Coins);
    TestTrue(TEXT("Legacy completed reward unlocks 08 on load"),Achievements->IsAchievementUnlocked(TEXT("08")));
    Quests->LoadQuestProgressFromSave(Saved,0,Coins);
    TestTrue(TEXT("Repeated load preserves unlock"),Achievements->IsAchievementUnlocked(TEXT("08")));
    TestEqual(TEXT("Legacy unlock does not grant the quest reward again"),Quests->GetCoinBalance(),Coins);
    Saved.Reset();
    for (int32 Id : {11,12,13})
    {
        FTunaSweeperNumericQuestProgressSaveData Completed;
        Completed.QuestId=Id;Completed.State=ETunaSweeperQuestState::RewardCompleted;
        Saved.Add(Completed);
    }
    FTunaSweeperNumericQuestProgressSaveData Storage;
    Storage.QuestId=14;Storage.State=ETunaSweeperQuestState::RewardAvailable;Saved.Add(Storage);
    Quests->LoadQuestProgressFromSave(Saved,0,Coins);
    TestFalse(TEXT("Home requires storage reward, not just objectives"),Achievements->IsAchievementUnlocked(TEXT("05")));
    Game->bSuppressSaveWritesForTests=true;
    TestTrue(TEXT("Storage reward claimed in memory"),Quests->ClaimQuestReward(14));
    TestFalse(TEXT("Home remains locked until save commits"),Achievements->IsAchievementUnlocked(TEXT("05")));
    Game->bSuppressSaveWritesForTests=false;
    TestTrue(TEXT("Save all four completed rewards"),Game->SaveGameStateInternal());
    TestTrue(TEXT("All four committed rewards unlock home"),Achievements->IsAchievementUnlocked(TEXT("05")));
    return true;
}
#endif
