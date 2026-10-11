#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperRegionLinkActor.h"
#include "Interaction/TunaSweeperBombObstacleActor.h"
#include "Weapon/TunaSweeperThrowableDamageType.h"
#include "UObject/StrongObjectPtr.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Kismet/GameplayStatics.h"

namespace RegionPassageTest
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRegionPassageTest, "TunaSweeper.Region.PassageRepairAndBomb",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRegionPassageTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    RegionPassageTest::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Never touch player saves.
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        RegionPassageTest::FContextAccess::Attach(Game.Get(), nullptr); World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    auto* Lift = World->SpawnActor<ATunaSweeperRegionLinkActor>();
    Lift->RequiredProgressId = TEXT("test.region.lift");
    FTunaSweeperItemStack Cable; Cable.ItemId = 6010; Cable.Quantity = 1;
    FTunaSweeperItemStack Other; Other.ItemId = 6006; Other.Quantity = 1;
    Lift->RepairParts = {Cable, Other};
    TestFalse(TEXT("Broken lift prevents travel"), Lift->TravelToTargetLevel(nullptr));
    TestTrue(TEXT("Add first part"), Game->AddItemToFirstAvailableInventorySlot(6010, 1));
    TestFalse(TEXT("Only one part cannot repair"), Lift->RepairPassage(false));
    TestEqual(TEXT("Failed repair consumes nothing"), Game->CountInventoryItemById(6010), 1);
    TestTrue(TEXT("Add second part"), Game->AddItemToFirstAvailableInventorySlot(6006, 1));
    TestTrue(TEXT("Both parts repair lift"), Lift->RepairPassage(false));
    TestTrue(TEXT("Completed lift unlocks travel"), Lift->IsPassageUnlocked());
    TestEqual(TEXT("First part consumed"), Game->CountInventoryItemById(6010), 0);
    TestEqual(TEXT("Second part consumed"), Game->CountInventoryItemById(6006), 0);
    TestFalse(TEXT("Repeat repair rejected"), Lift->RepairPassage(false));
    auto* Reloaded = World->SpawnActor<ATunaSweeperRegionLinkActor>();
    Reloaded->RequiredProgressId = Lift->RequiredProgressId;
    TestTrue(TEXT("New lift instance restores completed state"), Reloaded->IsPassageUnlocked());
    auto* Gate = World->SpawnActor<ATunaSweeperRegionLinkActor>();
    Gate->RequiredProgressId = TEXT("test.region.obstacle");
    auto* Obstacle = World->SpawnActor<ATunaSweeperBombObstacleActor>();
    Obstacle->ProgressObjectId = Gate->RequiredProgressId;
    TestFalse(TEXT("Locked gate cannot be bypassed through travel call"), Gate->TravelToTargetLevel(nullptr));
    FDamageEvent Bullet;
    TestEqual(TEXT("Ordinary damage cannot break obstacle"), Obstacle->TakeDamage(999, Bullet, nullptr, nullptr), 0.0f);
    TestTrue(TEXT("Ordinary damage preserves collision"), Obstacle->GetActorEnableCollision());
    FDamageEvent Bomb(UTunaSweeperGrenadeDamageType::StaticClass());
    TestTrue(TEXT("Bomb breaks obstacle"), Obstacle->TakeDamage(1, Bomb, nullptr, nullptr) > 0);
    TestFalse(TEXT("Bomb disables obstacle collision"), Obstacle->GetActorEnableCollision());
    TestTrue(TEXT("Bomb unlocks paired gate"), Gate->IsPassageUnlocked());
    auto* RestoredObstacle = World->SpawnActor<ATunaSweeperBombObstacleActor>();
    RestoredObstacle->ProgressObjectId = Obstacle->ProgressObjectId;
    RestoredObstacle->RefreshObstacle();
    TestFalse(TEXT("Destroyed obstacle stays gone on recreation"), RestoredObstacle->GetActorEnableCollision());
    auto* Vitals = NewObject<UTunaSweeperVitalsComponent>(Lift);
    Game->PendingRegionVitalsDestination = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
    Game->PendingRegionVitalsRatios = FVector(0.25, 0.1, 0.2);
    Game->ConsumePendingRegionVitals(Vitals);
    TestEqual(TEXT("Regional travel retains low health"), Vitals->GetVitalsState().Health, 25.f);
    TestEqual(TEXT("Regional travel does not apply base food recovery"), Vitals->GetVitalsState().Food, 10.f);
    TestEqual(TEXT("Regional travel does not apply base hydration recovery"), Vitals->GetVitalsState().Hydration, 20.f);
    TestTrue(TEXT("Arrival consumes carried vitals once"), Game->PendingRegionVitalsDestination.IsNone());
    return true;
}
#endif
