#if WITH_DEV_AUTOMATION_TESTS
#include "Interaction/TunaSweeperDeliveryDestinationActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperDeliveryTest,
    "TunaSweeper.Quest.DeliveryDestination",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperDeliveryTest::RunTest(const FString& Parameters)
{
    struct FContextAccess : UGameInstance {
        static void Attach(UGameInstance* Game, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context); Context.OwningGameInstance = Game; World->SetGameInstance(Game);
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game, nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset(); Quests->bQuestDataLoaded = true;
    FTunaSweeperQuestDefinition Definition;
    Definition.QuestId = 900003;
    FTunaSweeperObjectiveDefinition Objective;
    Objective.ObjectiveId = TEXT("deliver"); Objective.Type = ETunaSweeperObjectiveType::ItemSubmitted;
    Objective.ItemId = 6011; Objective.RequiredCount = 1; Objective.TargetProviderId = TEXT("delivery.test");
    Definition.Objectives.Add(Objective);
    Quests->QuestDefinitions.Add(Definition.QuestId, Definition);
    Definition.QuestId = 900004;
    Quests->QuestDefinitions.Add(Definition.QuestId, Definition);
    TestFalse(TEXT("Unaccepted quest cannot deliver"), Quests->TrySubmitItemsForObjective(900003, TEXT("deliver"), TEXT("delivery.test"), false));
    Quests->QuestProgressById.FindOrAdd(900003).State = ETunaSweeperQuestState::Accepted;
    Quests->QuestProgressById.FindOrAdd(900004).State = ETunaSweeperQuestState::Accepted;
    Quests->QuestProgressById.FindOrAdd(900003).QuestId = 900003;
    Quests->QuestProgressById.FindOrAdd(900004).QuestId = 900004;
    TestFalse(TEXT("Missing item cannot complete"), Quests->TrySubmitItemsForObjective(900003, TEXT("deliver"), TEXT("delivery.test"), false));
    TestTrue(TEXT("Add parcel and an extra"), Game->AddItemToFirstAvailableInventorySlot(6011, 2));
    TestFalse(TEXT("Wrong destination cannot consume"), Quests->TrySubmitItemsForObjective(900003, TEXT("deliver"), TEXT("delivery.wrong"), false));
    TestFalse(TEXT("Wrong objective cannot consume"), Quests->TrySubmitItemsForObjective(900003, TEXT("wrong"), TEXT("delivery.test"), false));
    TestEqual(TEXT("Rejected delivery preserves both items"), Game->CountInventoryItemById(6011), 2);
    bool bAtomic = false;
    bool bReentered = false;
    const auto Handle = Game->OnInventoryStateChanged.AddLambda([&] {
        bAtomic = Game->CountInventoryItemById(6011) == 1 && Quests->GetQuestState(900003) == ETunaSweeperQuestState::RewardAvailable;
        bReentered = Quests->TrySubmitItemsForObjective(900004, TEXT("deliver"), TEXT("delivery.test"), false);
    });
    TestTrue(TEXT("Destination consumes and completes"), Quests->TrySubmitItemsForObjective(900003, TEXT("deliver"), TEXT("delivery.test"), false));
    Game->OnInventoryStateChanged.Remove(Handle);
    TestTrue(TEXT("Observers see consumption and completion together"), bAtomic);
    TestFalse(TEXT("Submission is not reentrant"), bReentered);
    TestEqual(TEXT("Same destination does not complete another quest"), Quests->GetQuestState(900004), ETunaSweeperQuestState::Accepted);
    TestFalse(TEXT("Completed delivery cannot consume again"), Quests->TrySubmitItemsForObjective(900003, TEXT("deliver"), TEXT("delivery.test"), false));
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Quests->ExportQuestProgressForSave(Save->NumericQuestProgressStates, Save->NumericTrackedQuestId, Save->QuestCoinBalance);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Delivery progress serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Loaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Delivery save reloads"), Loaded)) return false;
    Quests->LoadQuestProgressFromSave(Loaded->NumericQuestProgressStates, Loaded->NumericTrackedQuestId, Loaded->QuestCoinBalance);
    TestEqual(TEXT("Completed delivery state survives reload"), Quests->GetQuestState(900003), ETunaSweeperQuestState::RewardAvailable);
    TestEqual(TEXT("Other delivery remains accepted after reload"), Quests->GetQuestState(900004), ETunaSweeperQuestState::Accepted);
    TestFalse(TEXT("Reload cannot consume delivery again"), Quests->TrySubmitItemsForObjective(900003, TEXT("deliver"), TEXT("delivery.test"), false));
    TestEqual(TEXT("Extra parcel remains"), Game->CountInventoryItemById(6011), 1);
    auto* Destination = World->SpawnActor<ATunaSweeperDeliveryDestinationActor>();
    Destination->QuestId = 900004; Destination->ObjectiveId = TEXT("deliver"); Destination->DestinationId = TEXT("delivery.test");
    Destination->DispatchBeginPlay();
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Pawn = World->SpawnActor<ADefaultPawn>(Spawn);
    auto* Controller = World->SpawnActor<APlayerController>(); Controller->Possess(Pawn);
    Pawn->SetPlayerState(World->SpawnActor<APlayerState>());
    Pawn->SetActorLocation(FVector(10000, 0, 0));
    TestFalse(TEXT("Cannot deliver remotely"), Destination->Deliver(Pawn));
    Pawn->SetActorLocation(FVector(0, 0, 10000));
    TestFalse(TEXT("Cannot deliver through distant floors"), Destination->Deliver(Pawn));
    Pawn->SetActorLocation(Destination->GetActorLocation());
    TestTrue(TEXT("Test pawn is player controlled"), Pawn->IsPlayerControlled());
    TestTrue(TEXT("Accepted destination with item is ready"), Destination->CanDeliver());
    TestTrue(TEXT("Delivery marker is in interaction range"), Destination->Interactable->IsWithinInteractionDistance(Pawn));
    TestTrue(TEXT("Destination is visible"), !Destination->IsHidden());
    TestTrue(TEXT("Player interacts at destination"), Destination->Deliver(Pawn));
    TestFalse(TEXT("Completed destination disables delivery"), Destination->CanDeliver());
    TestEqual(TEXT("Second delivery consumes exactly one"), Game->CountInventoryItemById(6011), 0);
    return true;
}
#endif
