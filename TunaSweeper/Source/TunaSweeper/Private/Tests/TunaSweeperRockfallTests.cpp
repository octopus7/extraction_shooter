#if WITH_DEV_AUTOMATION_TESTS
#include "Interaction/TunaSweeperRockfallBarrierActor.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"

namespace RockfallTest
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance,FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRockfallTest,"TunaSweeper.Interaction.Rockfall.TransactionCountdownRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRockfallTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game,false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    RockfallTest::FContextAccess::Attach(Game.Get(),&Context); Context.OwningGameInstance=Game.Get(); World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized=true; Game->RetiredDemoSaveSlotIndex=Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false); Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance=nullptr; RockfallTest::FContextAccess::Attach(Game.Get(),nullptr);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays(); Game->WorldProgressStatesById.Reset();
    // SpawnActor must run PostInitializeComponents. RouteEndPlay intentionally skips actors
    // that were never initialized, even if DispatchBeginPlay was called by a test.
    World->InitializeActorsForPlay(FURL());
    auto* Barrier=World->SpawnActor<ATunaSweeperRockfallBarrierActor>();
    TestEqual(TEXT("Five second default fuse"),Barrier->CountdownSeconds,5.f);
    TestEqual(TEXT("Production explosive requirement"),Barrier->RequiredItemId,6031);
    TestFalse(TEXT("No persistent ID rejects installation"),Barrier->ArmExplosive(false));
    Barrier->ProgressObjectId=TEXT("test.rockfall"); Barrier->RequiredItemId=6020;
    Barrier->DispatchBeginPlay();
    TestTrue(TEXT("Fixture actor is initialized"),Barrier->IsActorInitialized());
    TestTrue(TEXT("Fixture actor entered BeginPlay"),Barrier->HasActorBegunPlay());
    TestFalse(TEXT("Missing item rejects installation"),Barrier->ArmExplosive(false));
    TestTrue(TEXT("Supply two test items"),Game->AddItemToFirstAvailableInventorySlot(6020,2));
    int32 ProgressNotifications=0;
    const FDelegateHandle Listener=Game->OnWorldProgressChanged.AddLambda([&](){++ProgressNotifications;});
    TestFalse(TEXT("Refused disk save aborts installation"),Barrier->ArmExplosive(true));
    TestEqual(TEXT("Failed save preserves inventory"),Game->CountInventoryItemById(6020),2);
    TestFalse(TEXT("Failed save leaves no world progress"),Game->WorldProgressStatesById.Contains(Barrier->ProgressObjectId));
    TestEqual(TEXT("Failed save publishes no progress"),ProgressNotifications,0);
    TestFalse(TEXT("Failed save starts no fuse"),Barrier->IsArmed());
    TestTrue(TEXT("Installation succeeds"),Barrier->ArmExplosive(false));
    TestFalse(TEXT("Successful retry clears the previous save-failure state"),Barrier->bSaveFailed);
    TestEqual(TEXT("Exactly one item consumed"),Game->CountInventoryItemById(6020),1);
    TestFalse(TEXT("Second installation rejected"),Barrier->ArmExplosive(false));
    TestEqual(TEXT("Duplicate interaction consumes nothing"),Game->CountInventoryItemById(6020),1);
    TestTrue(TEXT("Armed danger marker visible"),Barrier->WarningDecal->IsVisible());
    TestEqual(TEXT("Warning radius equals damage radius"),Barrier->WarningDecal->DecalSize.Y,static_cast<double>(Barrier->ExplosionRadiusCm));
    TestEqual(TEXT("Installation does not complete progression"),Game->WorldProgressStatesById[Barrier->ProgressObjectId].State,ETunaSweeperWorldProgressState::InProgress);
    auto Advance=[&](float Seconds){++GFrameCounter;World->GetTimerManager().Tick(Seconds);};
    Advance(.01f);Advance(4.8f);
    TestFalse(TEXT("No early explosion"),Barrier->IsCleared());
    // Destroying during the fuse preserves the paid installation and cancels the old timer.
    const FTimerHandle OldTimer=Barrier->DetonationTimer;
    TestTrue(TEXT("Destroy routes the initialized actor through teardown"),Barrier->Destroy());
    TestFalse(TEXT("Actual EndPlay resets the actor play state"),Barrier->HasActorBegunPlay());
    TestFalse(TEXT("EndPlay cancels fuse"),World->GetTimerManager().TimerExists(OldTimer));
    auto* Restored=World->SpawnActor<ATunaSweeperRockfallBarrierActor>();
    Restored->ProgressObjectId=TEXT("test.rockfall"); Restored->RequiredItemId=6020; Restored->bSaveChanges=false;
    Restored->DispatchBeginPlay();
    TestTrue(TEXT("Paid installation resumes after reload"),Restored->IsArmed());
    TestEqual(TEXT("Reload consumes no additional item"),Game->CountInventoryItemById(6020),1);
    TestTrue(TEXT("Reload restores full escape interval"),Restored->GetCountdownRemaining()>4.9f);
    // Exercise the real player TakeDamage -> armor/faction checks -> vitals path,
    // without world ticks (which would also introduce hunger/movement into this test).
    FActorSpawnParameters PawnSpawn;
    PawnSpawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Exposed=World->SpawnActor<ATunaSweeperTopDownCharacter>(ATunaSweeperTopDownCharacter::StaticClass(),
        Restored->GetActorLocation()+FVector(0,0,90),FRotator::ZeroRotator,PawnSpawn);
    auto* Escaped=World->SpawnActor<ATunaSweeperTopDownCharacter>(ATunaSweeperTopDownCharacter::StaticClass(),
        Restored->GetActorLocation()+FVector(Restored->ExplosionRadiusCm+150,0,90),FRotator::ZeroRotator,PawnSpawn);
    if (!TestNotNull(TEXT("Exposed player pawn"),Exposed) || !TestNotNull(TEXT("Escaped player pawn"),Escaped)) return false;
    Exposed->GetVitalsComponent()->SetVitalsState(FTunaSweeperVitalsState());
    Escaped->GetVitalsComponent()->SetVitalsState(FTunaSweeperVitalsState());
    const auto ExposedHealth=[&](){return Exposed->GetVitalsComponent()->GetVitalsState().Health;};
    const auto EscapedHealth=[&](){return Escaped->GetVitalsComponent()->GetVitalsState().Health;};
    // Failed completion keeps the obstacle and does not publish the unlock.
    Restored->bSaveChanges=true; Restored->Detonate();
    TestFalse(TEXT("Completion save failure keeps barrier"),Restored->IsCleared());
    TestEqual(TEXT("Failed completion must not damage nearby player"),ExposedHealth(),100.f);
    TestEqual(TEXT("Completion save failure rolls back progress"),Game->WorldProgressStatesById[Restored->ProgressObjectId].State,ETunaSweeperWorldProgressState::InProgress);
    Restored->bSaveChanges=false; Restored->StartCountdown();
    Advance(.01f);Advance(4.8f);
    TestFalse(TEXT("Restored countdown still blocks before expiry"),Restored->IsCleared());
    TestEqual(TEXT("Player remains unharmed during escape countdown"),ExposedHealth(),100.f);
    Advance(.3f);
    TestTrue(TEXT("Fuse expiry clears rocks"),Restored->IsCleared());
    TestEqual(TEXT("Explosion applies actual player health damage once"),ExposedHealth(),40.f);
    TestEqual(TEXT("Player outside warning radius takes no damage"),EscapedHealth(),100.f);
    TestEqual(TEXT("Completion saved"),Game->WorldProgressStatesById[Restored->ProgressObjectId].State,ETunaSweeperWorldProgressState::Completed);
    TestEqual(TEXT("Central blocker disabled"),Restored->BlockingCollision->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Central rocks hidden"),Restored->CentralRocks->IsVisible());
    TestTrue(TEXT("Left cliff remains"),Restored->LeftCliff->IsVisible());
    TestEqual(TEXT("Right cliff retains collision"),Restored->RightCliff->GetCollisionEnabled(),ECollisionEnabled::QueryAndPhysics);
    TestFalse(TEXT("Danger marker stops after explosion"),Restored->WarningDecal->IsVisible());
    const int32 BeforeDuplicate=ProgressNotifications; Restored->Detonate();
    TestEqual(TEXT("Duplicate detonation has no effect"),ProgressNotifications,BeforeDuplicate);
    TestEqual(TEXT("Duplicate detonation cannot damage player again"),ExposedHealth(),40.f);
    auto* Completed=World->SpawnActor<ATunaSweeperRockfallBarrierActor>();
    Completed->ProgressObjectId=TEXT("test.rockfall");Completed->DispatchBeginPlay();
    TestTrue(TEXT("Completed reload stays clear"),Completed->IsCleared());
    TestEqual(TEXT("Completed reload does not replay blast damage"),ExposedHealth(),40.f);
    TestFalse(TEXT("Completed reload never starts fuse"),World->GetTimerManager().TimerExists(Completed->DetonationTimer));
    TestEqual(TEXT("Blast center matches configured damage"),Completed->GetExplosionDamageAtLocation(Completed->GetActorLocation()),Completed->ExplosionDamage);
    TestEqual(TEXT("Visible danger edge remains damaging"),Completed->GetExplosionDamageAtLocation(Completed->GetActorLocation()+FVector(Completed->ExplosionRadiusCm,0,0)),Completed->EdgeExplosionDamage);
    TestEqual(TEXT("Outside danger circle is safe"),Completed->GetExplosionDamageAtLocation(Completed->GetActorLocation()+FVector(Completed->ExplosionRadiusCm+1,0,0)),0.f);
    Game->OnWorldProgressChanged.Remove(Listener);
    return true;
}
#endif
