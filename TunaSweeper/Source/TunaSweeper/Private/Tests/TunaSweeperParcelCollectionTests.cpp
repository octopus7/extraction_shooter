#if WITH_DEV_AUTOMATION_TESTS
#include "Interaction/TunaSweeperParcelContainerActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperParcelCollectionTest,
    "TunaSweeper.Quest.ParcelPickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperParcelCollectionTest::RunTest(const FString& Parameters)
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Game, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context);
    Context.OwningGameInstance = Game;
    World->SetGameInstance(Game);
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game, nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    TestFalse(TEXT("Parcel must not inherit ordinary loot-container behavior"),
        ATunaSweeperParcelContainerActor::StaticClass()->IsChildOf(ATunaSweeperLootContainerActor::StaticClass()));
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset(); Quests->bQuestDataLoaded = true;
    FTunaSweeperQuestDefinition Definition;
    Definition.QuestId = 900030;
    FTunaSweeperObjectiveDefinition Objective;
    Objective.ObjectiveId = TEXT("deliver_parcel"); Objective.Type = ETunaSweeperObjectiveType::ItemSubmitted;
    Objective.ItemId = 6011; Objective.RequiredCount = 1; Objective.TargetProviderId = TEXT("delivery.test");
    Definition.Objectives.Add(Objective);
    Quests->QuestDefinitions.Add(Definition.QuestId, Definition);
    Definition.QuestId = 900041; Definition.Objectives[0].ItemId = 6017;
    Quests->QuestDefinitions.Add(Definition.QuestId, Definition);
    auto* Parcel = World->SpawnActor<ATunaSweeperParcelContainerActor>();
    Parcel->QuestId = 900030; Parcel->ObjectiveId = TEXT("deliver_parcel");
    Parcel->DispatchBeginPlay();
    TestTrue(TEXT("Unaccepted parcel is hidden"), Parcel->IsHidden());
    TestFalse(TEXT("Unaccepted parcel cannot be collected"), Parcel->CanPickUp());
    Quests->QuestProgressById.FindOrAdd(900030).QuestId = 900030;
    Quests->QuestProgressById[900030].State = ETunaSweeperQuestState::Accepted;
    Quests->OnQuestProgressChanged.Broadcast();
    TestFalse(TEXT("Accepting quest reveals parcel immediately"), Parcel->IsHidden());
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Pawn = World->SpawnActor<ADefaultPawn>(Spawn);
    auto* Controller = World->SpawnActor<APlayerController>(); Controller->Possess(Pawn);
    Pawn->SetPlayerState(World->SpawnActor<APlayerState>());
    TestFalse(TEXT("Missing player cannot pick up"), Parcel->PickUp(nullptr));
    Pawn->SetActorLocation(FVector(0, 0, 10000));
    TestFalse(TEXT("Cannot collect through distant floors"), Parcel->PickUp(Pawn));
    Pawn->SetActorLocation(Parcel->GetActorLocation());
    bool bReentered = false;
    bool bObserverSawHidden = false;
    const auto Handle = Game->OnInventoryStateChanged.AddLambda([&] {
        bObserverSawHidden = Parcel->IsHidden() && !Parcel->GetActorEnableCollision();
        bReentered = Parcel->PickUp(Pawn);
    });
    TestTrue(TEXT("Direct pickup succeeds"), Parcel->PickUp(Pawn));
    Game->OnInventoryStateChanged.Remove(Handle);
    TestTrue(TEXT("Grant observers see the box already hidden"), bObserverSawHidden);
    TestFalse(TEXT("Pickup cannot reenter through inventory callbacks"), bReentered);
    TestEqual(TEXT("Exactly one volatile jar granted"), Game->CountInventoryItemById(6011), 1);
    TestTrue(TEXT("Successful pickup hides parcel"), Parcel->IsHidden());
    TestFalse(TEXT("Successful pickup disables collision"), Parcel->GetActorEnableCollision());
    TestFalse(TEXT("Repeated pickup is rejected"), Parcel->PickUp(Pawn));
    Quests->OnQuestProgressChanged.Broadcast();
    TestTrue(TEXT("Quest refresh does not resurrect collected parcel"), Parcel->IsHidden());
    TestNull(TEXT("Pickup never opens a loot container"), Game->GetActiveLootContainerOwner());
    TestEqual(TEXT("Possession does not finish delivery"), Quests->GetQuestState(900030), ETunaSweeperQuestState::Accepted);

    auto* Boat = World->SpawnActor<ATunaSweeperParcelContainerActor>();
    Boat->QuestId = 900041; Boat->ObjectiveId = TEXT("deliver_parcel");
    Quests->QuestProgressById.FindOrAdd(900041).QuestId = 900041;
    Quests->QuestProgressById[900041].State = ETunaSweeperQuestState::Accepted;
    Boat->DispatchBeginPlay();
    while (Game->AddItemToFirstAvailableInventorySlot(6011, 1)) {}
    TestFalse(TEXT("Full inventory rejects pickup"), Boat->PickUp(Pawn));
    TestFalse(TEXT("Full inventory leaves box visible"), Boat->IsHidden());
    TestTrue(TEXT("Full inventory leaves box interactable"), Boat->CanPickUp());
    TestEqual(TEXT("No boat granted without room"), Game->CountInventoryItemById(6017), 0);
    TestNull(TEXT("Full inventory never opens loot window"), Game->GetActiveLootContainerOwner());
    TestEqual(TEXT("Free one slot"), Game->ConsumeInventoryItemById(6011, 1), 1);
    TestTrue(TEXT("Retry succeeds after freeing space"), Boat->PickUp(Pawn));
    TestEqual(TEXT("Retry grants exactly one boat"), Game->CountInventoryItemById(6017), 1);
    TestTrue(TEXT("Boat hidden after retry"), Boat->IsHidden());

    auto* Invalid = World->SpawnActor<ATunaSweeperParcelContainerActor>();
    Invalid->QuestId = 900030; Invalid->ObjectiveId = TEXT("missing");
    TestFalse(TEXT("Invalid objective rejected"), Invalid->CanPickUp());
    Invalid->ObjectiveId = TEXT("deliver_parcel");
    Quests->QuestDefinitions[900030].Objectives[0].ItemId = 3007;
    TestFalse(TEXT("Nonvolatile item rejected"), Invalid->CanPickUp());
    Quests->QuestDefinitions[900030].Objectives[0].ItemId = 6011;
    Quests->QuestProgressById[900030].State = ETunaSweeperQuestState::RewardAvailable;
    Quests->OnQuestProgressChanged.Broadcast();
    TestFalse(TEXT("Completed delivery cannot be collected again"), Invalid->CanPickUp());

    for (const TCHAR* Path : {
        TEXT("/Game/Interaction/Parcels/BP_Parcel_ToadJar.BP_Parcel_ToadJar_C"),
        TEXT("/Game/Interaction/Parcels/BP_Parcel_ModelBoat.BP_Parcel_ModelBoat_C") })
    {
        auto* ParcelClass = LoadClass<ATunaSweeperParcelContainerActor>(nullptr, Path);
        if (!TestNotNull(TEXT("Authored parcel BP loads"), ParcelClass)) return false;
        TestFalse(TEXT("Authored BP does not inherit loot container"), ParcelClass->IsChildOf(ATunaSweeperLootContainerActor::StaticClass()));
        auto* Authored = World->SpawnActor<ATunaSweeperParcelContainerActor>(ParcelClass);
        TestTrue(TEXT("BP has quest and objective assigned"), Authored->QuestId > 0 && !Authored->ObjectiveId.IsNone());
        TestNotNull(TEXT("BP has cardboard mesh"), Authored->VisualMesh->GetStaticMesh().Get());
        auto* Replacement = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        Authored->VisualMesh->SetStaticMesh(Replacement);
        Authored->DispatchBeginPlay();
        Quests->OnQuestProgressChanged.Broadcast();
        TestTrue(TEXT("BP replacement mesh survives presence refresh"), Authored->VisualMesh->GetStaticMesh() == Replacement);
    }
    return true;
}
#endif
