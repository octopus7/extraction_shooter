#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperQuestData
{
    bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

namespace QuestEnemyClassTest
{
    bool Parse(const FString& Fields, FTunaSweeperObjectiveDefinition& Out)
    {
        TSharedPtr<FJsonObject> Json;
        return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(
            TEXT("{\"objective_id\":\"kill\",\"type\":\"enemy_killed\",\"required_count\":2") + Fields + TEXT("}")), Json)
            && TunaSweeperQuestData::ParseObjective(Json, Out);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestEnemyClassDataTest,
    "TunaSweeper.Quest.EnemyClass.DataValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestEnemyClassDataTest::RunTest(const FString& Parameters)
{
    FTunaSweeperObjectiveDefinition Objective;
    TestTrue(TEXT("Legacy unrestricted kills remain supported"), QuestEnemyClassTest::Parse(TEXT(""), Objective));
    TestTrue(TEXT("Legacy placement ID remains supported"), QuestEnemyClassTest::Parse(TEXT(",\"enemy_id\":\"placement.1\""), Objective));
    TestTrue(TEXT("Exact native class path loads"), QuestEnemyClassTest::Parse(
        TEXT(",\"enemy_class_paths\":[\"/Script/TunaSweeper.TunaSweeperEnemyCharacter\"]"), Objective));
    for (const FString Invalid : { TEXT("[]"), TEXT("null"), TEXT("true"), TEXT("17"), TEXT("\"Blue\""),
        TEXT("[null]"), TEXT("[17]"), TEXT("[\"\"]"), TEXT("[\"Blue\"]"), TEXT("[\"/Game/Blueprints/Blue\"]") })
    {
        TestFalse(TEXT("Malformed class filter must not become unrestricted kills"),
            QuestEnemyClassTest::Parse(TEXT(",\"enemy_class_paths\":") + Invalid, Objective));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestEnemyClassTest,
    "TunaSweeper.Quest.EnemyClass.DamageAcceptanceAndRewards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestEnemyClassTest::RunTest(const FString& Parameters)
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        FContextAccess::Attach(Game.Get(), nullptr); World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Real initialized quest subsystem"), Quests)) return false;
    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset(); Quests->bQuestDataLoaded = true;
    Quests->CoinBalance = 0;
    UClass* Gray = LoadClass<ATunaSweeperEnemyCharacter>(nullptr, TEXT("/Game/Blueprints/BP_QuadrupedGunEnemy.BP_QuadrupedGunEnemy_C"));
    UClass* Dark = LoadClass<ATunaSweeperEnemyCharacter>(nullptr, TEXT("/Game/Blueprints/Enemies/QuadrupedVariants/BP_QuadrupedGunEnemy_DarkGray.BP_QuadrupedGunEnemy_DarkGray_C"));
    UClass* Blue = LoadClass<ATunaSweeperEnemyCharacter>(nullptr, TEXT("/Game/Blueprints/Enemies/QuadrupedVariants/BP_QuadrupedGunEnemy_Blue.BP_QuadrupedGunEnemy_Blue_C"));
    UClass* Red = LoadClass<ATunaSweeperEnemyCharacter>(nullptr, TEXT("/Game/Blueprints/Enemies/QuadrupedVariants/BP_QuadrupedGunEnemy_Red.BP_QuadrupedGunEnemy_Red_C"));
    if (!TestNotNull(TEXT("Gray class"), Gray) || !TestNotNull(TEXT("Dark gray class"), Dark) ||
        !TestNotNull(TEXT("Blue class"), Blue) || !TestNotNull(TEXT("Red class"), Red)) return false;
    auto AddQuest = [&](int32 Id, const FString& Fields)
    {
        FTunaSweeperObjectiveDefinition Objective;
        TestTrue(TEXT("Objective parsed through production loader"), QuestEnemyClassTest::Parse(Fields, Objective));
        FTunaSweeperQuestDefinition Definition;
        Definition.QuestId = Id; Definition.Objectives = { Objective }; Definition.Rewards.Coins = 7;
        Quests->QuestDefinitions.Add(Id, Definition);
    };
    constexpr int32 GrayId = 900301, BlueId = 900302, LegacyId = 900303;
    AddQuest(GrayId, FString::Printf(TEXT(",\"enemy_class_paths\":[\"%s\",\"%s\"]"), *Gray->GetPathName(), *Dark->GetPathName()));
    AddQuest(BlueId, FString::Printf(TEXT(",\"enemy_class_paths\":[\"%s\"]"), *Blue->GetPathName()));
    AddQuest(LegacyId, TEXT(",\"enemy_id\":\"placement.1\""));
    auto* Killer = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Player kill instigator"), Killer)) return false;
    auto Kill = [&](UClass* Class)
    {
        FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>(Class, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Enemy spawned"), Enemy)) return;
        Enemy->TakeDamage(100000, FDamageEvent(), Killer, nullptr);
        TestEqual(TEXT("Lethal damage kills the real enemy"), Enemy->GetHealth(), 0.0f);
        Enemy->TakeDamage(100000, FDamageEvent(), Killer, nullptr);
    };
    Kill(Gray); Kill(Blue);
    TestTrue(TEXT("Accept gray quest after prior kills"), Quests->AcceptQuest(GrayId));
    TestTrue(TEXT("Accept blue quest after prior kills"), Quests->AcceptQuest(BlueId));
    TestEqual(TEXT("Gray kills before acceptance are ignored"), Quests->GetObjectiveProgressCount(GrayId, TEXT("kill")), 0);
    TestEqual(TEXT("Blue kills before acceptance are ignored"), Quests->GetObjectiveProgressCount(BlueId, TEXT("kill")), 0);
    Kill(Red);
    Quests->NotifyEnemyKilled(NAME_None);
    TestEqual(TEXT("Other colors and missing class cannot advance gray"), Quests->GetObjectiveProgressCount(GrayId, TEXT("kill")), 0);
    TestEqual(TEXT("Other colors and missing class cannot advance blue"), Quests->GetObjectiveProgressCount(BlueId, TEXT("kill")), 0);
    Kill(Blue);
    TestEqual(TEXT("Blue subclass never counts toward exact gray base"), Quests->GetObjectiveProgressCount(GrayId, TEXT("kill")), 0);
    TestEqual(TEXT("Repeated damage credits one blue kill"), Quests->GetObjectiveProgressCount(BlueId, TEXT("kill")), 1);
    Kill(Gray); Kill(Dark);
    TestEqual(TEXT("Two allowed gray classes complete the objective"), Quests->GetQuestState(GrayId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("Gray kills do not complete blue"), Quests->GetQuestState(BlueId), ETunaSweeperQuestState::Accepted);
    Kill(Blue);
    TestEqual(TEXT("Second blue kill makes reward available"), Quests->GetQuestState(BlueId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("Kills do not automatically grant rewards"), Quests->GetCoinBalance(), 0);
    TestTrue(TEXT("Reward explicitly claimed"), Quests->ClaimQuestReward(BlueId));
    TestEqual(TEXT("Claim grants reward once"), Quests->GetCoinBalance(), 7);
    TestFalse(TEXT("Duplicate reward claim rejected"), Quests->ClaimQuestReward(BlueId));
    Quests->AcceptQuest(LegacyId);
    Quests->NotifyEnemyKilled(TEXT("placement.other"));
    TestEqual(TEXT("Legacy ID filter rejects other placements"), Quests->GetObjectiveProgressCount(LegacyId, TEXT("kill")), 0);
    Quests->NotifyEnemyKilled(TEXT("placement.1")); Quests->NotifyEnemyKilled(TEXT("placement.1"));
    TestEqual(TEXT("Legacy callers still advance ID-only quests"), Quests->GetQuestState(LegacyId), ETunaSweeperQuestState::RewardAvailable);
    UClass* HopperClass = LoadClass<ATunaSweeperHopperEnemyCharacter>(nullptr,
        TEXT("/Game/Characters/Hopper/BP_HopperEnemy.BP_HopperEnemy_C"));
    if (!TestNotNull(TEXT("Actual hopper BP"), HopperClass)) return false;
    constexpr int32 HopperId = 900304;
    AddQuest(HopperId, FString::Printf(TEXT(",\"enemy_class_paths\":[\"%s\"]"), *HopperClass->GetPathName()));
    constexpr int32 HopperCompleteId = 900305;
    Quests->QuestDefinitions.Add(HopperCompleteId, Quests->QuestDefinitions[HopperId]);
    Quests->QuestDefinitions[HopperCompleteId].QuestId = HopperCompleteId;
    Quests->QuestDefinitions[HopperCompleteId].Objectives[0].RequiredCount = 1;
    TestTrue(TEXT("Accept single mech objective"), Quests->AcceptQuest(HopperCompleteId));
    TestTrue(TEXT("Accept mech defeat objective"), Quests->AcceptQuest(HopperId));
    FActorSpawnParameters HopperSpawn;
    HopperSpawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Hopper = World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(HopperClass, FVector::ZeroVector, FRotator::ZeroRotator, HopperSpawn);
    if (!TestNotNull(TEXT("Actual hopper BP instance"), Hopper)) return false;
    Hopper->TakeDamage(Hopper->GetMechHealth()-1.f, FDamageEvent(), Killer, nullptr);
    TestEqual(TEXT("Surviving mech does not count as a kill"), Quests->GetObjectiveProgressCount(HopperId, TEXT("kill")), 0);
    Hopper->TakeDamage(1.f, FDamageEvent(), Killer, nullptr);
    TestTrue(TEXT("Mech zero health ends combat immediately"), Hopper->IsDead());
    TestFalse(TEXT("Open cockpit wreck remains in level"), Hopper->IsActorBeingDestroyed());
    TestEqual(TEXT("Destroyed mech leaves cockpit open"), Hopper->GetPresentation()->GetHatchOpenAlpha(), 1.f);
    Hopper->GetPresentation()->TickComponent(5.f, LEVELTICK_All, nullptr);
    TestEqual(TEXT("Cockpit does not close again after defeat"), Hopper->GetPresentation()->GetHatchOpenAlpha(), 1.f);
    TestEqual(TEXT("Mech destruction immediately credits one kill"), Quests->GetObjectiveProgressCount(HopperId, TEXT("kill")), 1);
    Hopper->TakeDamage(9999.f, FDamageEvent(), Killer, nullptr);
    TestEqual(TEXT("Wreck cannot credit duplicate kills"), Quests->GetObjectiveProgressCount(HopperId, TEXT("kill")), 1);
    TestEqual(TEXT("No pilot combat follows mech defeat"), Hopper->GetCombatPhase(), EHopperCombatPhase::Dead);
    TestEqual(TEXT("One mech defeat immediately enables reward claim"), Quests->GetQuestState(HopperCompleteId), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("Mech kill does not automatically grant quest rewards"), Quests->GetCoinBalance(), 7);
    return true;
}
#endif
