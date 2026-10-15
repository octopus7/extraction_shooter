#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AIController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"

namespace TunaSweeperQuestData
{
    bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

namespace QuestRoadkillTest
{
    bool Parse(const FString& Fields, FTunaSweeperObjectiveDefinition& Out)
    {
        TSharedPtr<FJsonObject> Json;
        return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(
            TEXT("{\"objective_id\":\"kill\",\"type\":\"enemy_killed\",\"required_count\":2") + Fields + TEXT("}")), Json)
            && TunaSweeperQuestData::ParseObjective(Json, Out);
    }

}

    struct FTunaSweeperRoadkillTestScene
    {
        struct FContextAccess : UGameInstance
        {
            static void Attach(UGameInstance* Instance, FWorldContext* Context)
            { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
        };
        UWorld* World = nullptr;
        FWorldContext* Context = nullptr;
        TStrongObjectPtr<UTunaSweeperGameInstance> Game;
        UTunaSweeperQuestSubsystem* Quests = nullptr;
        APlayerController* PlayerController = nullptr;

        FTunaSweeperRoadkillTestScene()
        {
            World = UWorld::CreateWorld(EWorldType::Game, false);
            Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
            Context->SetCurrentWorld(World);
            Game.Reset(NewObject<UTunaSweeperGameInstance>(GEngine));
            FContextAccess::Attach(Game.Get(), Context);
            Context->OwningGameInstance = Game.Get();
            World->SetGameInstance(Game.Get());
            Game->bInventoryStateInitialized = true;
            Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
            Game->UGameInstance::Init();
            Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
            Quests->QuestDefinitions.Reset();
            Quests->QuestProgressById.Reset();
            Quests->bQuestDataLoaded = true;
            PlayerController = World->SpawnActor<APlayerController>();
        }
        ~FTunaSweeperRoadkillTestScene()
        {
            if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Quit);
            Game->UGameInstance::Shutdown();
            World->SetGameInstance(nullptr);
            Context->OwningGameInstance = nullptr;
            FContextAccess::Attach(Game.Get(), nullptr);
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
            World->RemoveFromRoot();
        }
        bool AddQuest(int32 Id, const FString& Fields)
        {
            FTunaSweeperObjectiveDefinition Objective;
            if (!QuestRoadkillTest::Parse(Fields, Objective)) return false;
            FTunaSweeperQuestDefinition Definition;
            Definition.QuestId = Id;
            Definition.Objectives = { Objective };
            Quests->QuestDefinitions.Add(Id, Definition);
            return Quests->AcceptQuest(Id);
        }
        int32 Count(int32 Id) const { return Quests->GetObjectiveProgressCount(Id, TEXT("kill")); }
        ATunaSweeperEnemyCharacter* Enemy(FVector Location = FVector::ZeroVector)
        {
            FActorSpawnParameters Spawn;
            Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            auto* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>(Location, FRotator::ZeroRotator, Spawn);
            Enemy->ConfigureCombatProfile(FTunaSweeperEnemyCombatProfile(), 22, NAME_None, INDEX_NONE);
            return Enemy;
        }
        void BeginPlay()
        {
            World->InitializeActorsForPlay(FURL());
            World->GetWorldSettings()->NotifyBeginPlay();
            World->GetWorldSettings()->NotifyMatchStarted();
            World->BeginPlay();
        }
    };

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRoadkillDataTest,
    "TunaSweeper.Quest.Roadkill.StrictFilter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRoadkillDataTest::RunTest(const FString& Parameters)
{
    FTunaSweeperObjectiveDefinition Objective;
    TestTrue(TEXT("Roadkill boolean accepted"), QuestRoadkillTest::Parse(TEXT(",\"requires_vehicle_collision\":true"), Objective));
    TestTrue(TEXT("Explicit unrestricted kill accepted"), QuestRoadkillTest::Parse(TEXT(",\"requires_vehicle_collision\":false"), Objective));
    for (const FString Invalid : { TEXT("null"), TEXT("17"), TEXT("\"true\""), TEXT("[]") })
        TestFalse(TEXT("Malformed roadkill filter cannot become generic kills"),
            QuestRoadkillTest::Parse(TEXT(",\"requires_vehicle_collision\":") + Invalid, Objective));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRoadkillGunTest,
    "TunaSweeper.Quest.Roadkill.GunDamageExcluded", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRoadkillGunTest::RunTest(const FString& Parameters)
{
    FTunaSweeperRoadkillTestScene Scene;
    constexpr int32 Roadkill = 900401, Generic = 900402;
    TestTrue(TEXT("Accept roadkill quest"), Scene.AddQuest(Roadkill, TEXT(",\"requires_vehicle_collision\":true")));
    TestTrue(TEXT("Accept ordinary kill quest"), Scene.AddQuest(Generic, TEXT("")));
    auto* Enemy = Scene.Enemy();
    Enemy->TakeDamage(100000, FDamageEvent(), Scene.PlayerController, nullptr);
    Enemy->TakeDamage(100000, FDamageEvent(), Scene.PlayerController, nullptr);
    TestEqual(TEXT("Gun death never advances roadkill quest"), Scene.Count(Roadkill), 0);
    TestEqual(TEXT("Ordinary kill still counts exactly once"), Scene.Count(Generic), 1);
    auto* AI = Scene.World->SpawnActor<AAIController>();
    Scene.Enemy()->TakeDamage(100000, FDamageEvent(), AI, nullptr);
    TestEqual(TEXT("Other AI kills cannot advance player quest"), Scene.Count(Generic), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRoadkillATVTest,
    "TunaSweeper.Quest.Roadkill.ATVSweptCollision", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRoadkillATVTest::RunTest(const FString& Parameters)
{
    FTunaSweeperRoadkillTestScene Scene;
    constexpr int32 Roadkill = 900403, WrongClass = 900404;
    TestTrue(TEXT("Accept vehicle kill of exact enemy class"), Scene.AddQuest(Roadkill, TEXT(",\"requires_vehicle_collision\":true,\"enemy_class_paths\":[\"/Script/TunaSweeper.TunaSweeperEnemyCharacter\"]")));
    TestTrue(TEXT("Accept other-class vehicle kill quest"), Scene.AddQuest(WrongClass, TEXT(",\"requires_vehicle_collision\":true,\"enemy_class_paths\":[\"/Script/TunaSweeper.TunaSweeperPatternEnemyCharacter\"]")));
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* ATV = Scene.World->SpawnActor<ATunaSweeperATVActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    auto* Rider = Scene.World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector(0, 140, 90), FRotator::ZeroRotator, Spawn);
    Scene.PlayerController->Possess(Rider);
    ATV->MountComponent->EngineStartSound = ATV->MountComponent->EngineIdleSound = ATV->MountComponent->EngineStopSound = nullptr;
    ATV->MountComponent->EngineDriveSound = ATV->MountComponent->EngineBoostSound = nullptr;
    auto* Victim = Scene.Enemy(FVector(80, 0, 88));
    Scene.BeginPlay();
    const float InitialHealth = Victim->GetHealth();
    ATV->VehicleMesh->SetPhysicsLinearVelocity(FVector(1000, 0, 0));
    ATV->Tick(0.1f);
    TestEqual(TEXT("Unoccupied ATV cannot award collision kills"), Victim->GetHealth(), InitialHealth);
    if (!TestTrue(TEXT("Real rider mounts ATV"), ATV->MountComponent->TryMount(Rider))) return false;
    ATV->VehicleMesh->SetPhysicsLinearVelocity(FVector(100, 0, 0));
    ATV->SetActorLocation(FVector(30, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
    ATV->Tick(0.1f);
    TestEqual(TEXT("Low-speed contact cannot damage enemy"), Victim->GetHealth(), InitialHealth);
    Victim->SetActorLocation(FVector(220, 0, 88), false, nullptr, ETeleportType::TeleportPhysics);
    auto* Friendly = Scene.Enemy(FVector(220, 70, 88));
    Friendly->ConfigureCombatProfile(FTunaSweeperEnemyCombatProfile(), 1, NAME_None, INDEX_NONE);
    const float FriendlyHealth = Friendly->GetHealth();
    ATV->VehicleMesh->SetPhysicsLinearVelocity(FVector(1400, 0, 0));
    ATV->SetActorLocation(FVector(430, 0, 0), false, nullptr, ETeleportType::TeleportPhysics);
    ATV->Tick(0.3f);
    TestTrue(TEXT("Moving occupied ATV kills an intersected enemy through real damage path"), Victim->IsDead());
    TestEqual(TEXT("ATV kill credits the seated player's roadkill objective"), Scene.Count(Roadkill), 1);
    TestEqual(TEXT("Class restriction still applies to ATV kills"), Scene.Count(WrongClass), 0);
    TestEqual(TEXT("Player-friendly faction in the swept path is unharmed"), Friendly->GetHealth(), FriendlyHealth);
    ATV->Tick(0.1f);
    TestEqual(TEXT("Repeated contact with a dead enemy gives no duplicate credit"), Scene.Count(Roadkill), 1);
    auto* ToughEnemy = Scene.Enemy(FVector(480, 0, 88));
    ToughEnemy->ConfigureSpawnData(TSoftObjectPtr<UMaterialInterface>(), NAME_None, INDEX_NONE, INDEX_NONE, 1000, 0);
    ATV->Tick(0.1f);
    const float HealthAfterImpact = ToughEnemy->GetHealth();
    TestTrue(TEXT("Occupied fast chassis damages a live contact"), HealthAfterImpact < 1000);
    ATV->Tick(0.1f);
    TestEqual(TEXT("Sustained overlap does not apply impact damage every frame"), ToughEnemy->GetHealth(), HealthAfterImpact);
    return true;
}
#endif
