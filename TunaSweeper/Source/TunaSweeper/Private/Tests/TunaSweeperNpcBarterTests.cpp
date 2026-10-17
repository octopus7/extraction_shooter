#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Character/TunaSweeperFacilityNpcActor.h"
#include "Interaction/TunaSweeperNpcBarterComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"

namespace NpcBarterTests
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperNpcBarterTest,
    "TunaSweeper.Interaction.NpcBarter.QuestGateAndTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperNpcBarterTest::RunTest(const FString& Parameters)
{
    // Catches early unlock, incorrect provider, wrong ratios, repeated/reentrant payment and partial commits.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    NpcBarterTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get();
    World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        Game->UGameInstance::Shutdown();
        World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr;
        NpcBarterTests::FContextAccess::Attach(Game.Get(), nullptr);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    };
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Quest subsystem available"), Quests)) return false;
    auto* Npc = World->SpawnActor<ATunaSweeperFacilityNpcActor>();
    auto* Barter = Npc->FindComponentByClass<UTunaSweeperNpcBarterComponent>();
    if (!TestNotNull(TEXT("Facility NPC provides an optional reusable barter interaction"), Barter)) return false;
    TestFalse(TEXT("Native barter is disabled without configuration"), Barter->IsUnlocked());
    auto* Pawn = World->SpawnActor<APawn>();
    auto* Root = NewObject<USceneComponent>(Pawn);
    Pawn->SetRootComponent(Root); Root->RegisterComponent();
    auto* Controller = World->SpawnActor<APlayerController>();
    // This isolated world has no game mode to create the state used by UE 5.7 IsPlayerControlled.
    Controller->SetPlayerState(World->SpawnActor<APlayerState>());
    Controller->Possess(Pawn);
    TestTrue(TEXT("Fixture pawn represents a player"), Pawn->IsPlayerControlled());
    const FName Provider = TEXT("test.barter.recipient");
    auto* ProviderProperty = FindFProperty<FNameProperty>(Npc->GetClass(), TEXT("QuestProviderId"));
    ProviderProperty->SetPropertyValue_InContainer(Npc, Provider);
    Barter->RequiredQuestId = 900400;
    Barter->InputItemId = 6009; Barter->InputQuantity = 2;
    Barter->OutputItemId = 6010; Barter->OutputQuantity = 1;
    Barter->OfferStringKey = TEXT("ui.interaction.fermented_fuel_supply");
    Barter->ShortageStringKey = TEXT("ui.barter.ethanol_shortage");
    Barter->FailureStringKey = TEXT("ui.barter.failed");
    FTunaSweeperQuestDefinition Quest;
    Quest.QuestId = Barter->RequiredQuestId;
    Quest.ProviderId = TEXT("test.barter.origin");
    Quest.CompletionProviderId = Provider;
    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset();
    Quests->QuestDefinitions.Add(Quest.QuestId, Quest);
    Quests->bQuestDataLoaded = true;
    World->InitializeActorsForPlay(FURL());
    Npc->DispatchBeginPlay();
    for (const auto State : {ETunaSweeperQuestState::Accepted, ETunaSweeperQuestState::RewardAvailable})
    {
        Quests->SetQuestState(Quest.QuestId, State);
        Quests->OnQuestProgressChanged.Broadcast();
        TestFalse(TEXT("Unclaimed quest reward keeps barter locked"), Barter->IsUnlocked());
        TestEqual(TEXT("Locked barter has no prompt"), Barter->GetInteractionType(), ETunaSweeperInteractionType::None);
        TestFalse(TEXT("Locked barter cannot be executed"), Barter->TryBarter(Pawn, false));
    }
    Quests->SetQuestState(Quest.QuestId, ETunaSweeperQuestState::RewardCompleted);
    Quests->OnQuestProgressChanged.Broadcast();
    TestTrue(TEXT("Claimed reward unlocks barter"), Barter->IsUnlocked());
    TestEqual(TEXT("Reward delegate immediately exposes barter"), Barter->GetInteractionType(), ETunaSweeperInteractionType::NpcBarter);
    TestTrue(TEXT("Empty inventory still offers barter"), World->GetSubsystem<UTunaSweeperInteractionSubsystem>()->CanOfferInteraction(Barter));
    ProviderProperty->SetPropertyValue_InContainer(Npc, Quest.ProviderId);
    TestFalse(TEXT("Origin provider cannot replace completion provider"), Barter->IsUnlocked());
    ProviderProperty->SetPropertyValue_InContainer(Npc, Provider);
    Npc->SetActorHiddenInGame(true);
    TestFalse(TEXT("Hidden NPC cannot trade"), Barter->TryBarter(Pawn, false));
    Npc->SetActorHiddenInGame(false);
    TestFalse(TEXT("Null pawn cannot trade"), Barter->TryBarter(nullptr, false));
    Controller->UnPossess();
    TestFalse(TEXT("Nonplayer pawn cannot trade"), Barter->TryBarter(Pawn, false));
    Controller->Possess(Pawn);
    TestTrue(TEXT("Repossessed fixture pawn is player controlled"), Pawn->IsPlayerControlled());
    Pawn->SetActorLocation(FVector(10000,0,0));
    TestFalse(TEXT("Distant player cannot trade"), Barter->TryBarter(Pawn, false));
    Pawn->SetActorLocation(Npc->GetActorLocation());
    TestTrue(TEXT("Returned player is within barter range"), Barter->IsWithinInteractionDistance(Pawn));
    auto ResetItems = [&]
    {
        Game->ItemInstancesByUid.Reset(); Game->ResetPlayerSlotArrays();
        Game->PlayerInventorySlots.SetNum(12); Game->AuxiliaryBagSlots.Reset();
        Game->StorageSlots.SetNum(1); Game->PendingQuestRewards.Reset(); Game->EverAcquiredItemIds.Reset();
    };
    ResetItems();
    const FGuid StoredUid = Game->CreateItemInstance(6009, 10);
    Game->StorageSlots[0].ItemUid = StoredUid;
    TestTrue(TEXT("Storage-only shortage handles interaction feedback"), Barter->TryBarter(Pawn, false));
    TestEqual(TEXT("Stored ingredients untouched"), Game->ItemInstancesByUid[StoredUid].Quantity, 10);
    TestEqual(TEXT("No output for shortage"), Game->CountInventoryItemById(6010), 0);
    const FGuid InputUid = Game->CreateItemInstance(6009, 1);
    Game->PlayerInventorySlots[0].ItemUid = InputUid;
    TestTrue(TEXT("One ingredient gives shortage feedback"), Barter->TryBarter(Pawn, false));
    TestEqual(TEXT("Shortage retains ingredient"), Game->CountInventoryItemById(6009), 1);
    Game->ItemInstancesByUid[InputUid].Quantity = 4;
    int32 Notifications = 0;
    bool Reentered = false;
    bool NestedExchange = false;
    const auto Handle = Game->OnInventoryStateChanged.AddLambda([&]
    {
        ++Notifications;
        Reentered |= Barter->TryBarter(Pawn, false);
        NestedExchange |= Game->ExchangeCarriedItemBatch(6009, 2, 6010, 1, false);
    });
    TestTrue(TEXT("Save refusal is handled with feedback"), Barter->TryBarter(Pawn, true));
    TestEqual(TEXT("Failed save restores payment"), Game->CountInventoryItemById(6009), 4);
    TestEqual(TEXT("Failed save restores output"), Game->CountInventoryItemById(6010), 0);
    TestFalse(TEXT("Failed save restores acquisition history"), Game->EverAcquiredItemIds.Contains(6010));
    TestEqual(TEXT("Failed save emits no inventory notification"), Notifications, 0);
    for (int32 Batch = 1; Batch <= 2; ++Batch)
    {
        TestTrue(TEXT("Repeat barter succeeds"), Barter->TryBarter(Pawn, false));
        TestEqual(TEXT("Every barter consumes exactly two"), Game->CountInventoryItemById(6009), 4 - 2 * Batch);
        TestEqual(TEXT("Every barter grants exactly one"), Game->CountInventoryItemById(6010), Batch);
    }
    TestFalse(TEXT("Inventory callback cannot reenter barter"), Reentered);
    TestFalse(TEXT("Inventory callback cannot nest a shared exchange"), NestedExchange);
    TestEqual(TEXT("Only committed trades publish inventory changes"), Notifications, 2);
    Game->OnInventoryStateChanged.Remove(Handle);
    TestEqual(TEXT("Barter leaves original reward claimed"), Quests->GetQuestState(Quest.QuestId), ETunaSweeperQuestState::RewardCompleted);
    TArray<FTunaSweeperNumericQuestProgressSaveData> Saved;
    int32 TrackedId = 0, Coins = 0;
    Quests->ExportQuestProgressForSave(Saved, TrackedId, Coins);
    // A non-game world suppresses account achievement reporting for the synthetic reload fixture.
    World->WorldType = EWorldType::Editor;
    Quests->LoadQuestProgressFromSave(Saved, TrackedId, Coins);
    World->WorldType = EWorldType::Game;
    TestTrue(TEXT("Existing claimed save restores repeat barter"), Barter->IsUnlocked());
    Game->PlayerInventorySlots[0].ItemUid = Game->CreateItemInstance(6009, 2);
    TestTrue(TEXT("Can trade again after reload"), Barter->TryBarter(Pawn, false));
    TestEqual(TEXT("Reloaded trade accumulates one reward"), Game->CountInventoryItemById(6010), 3);
    ResetItems();
    for (auto& Slot : Game->PlayerInventorySlots) Slot.ItemUid = Game->CreateItemInstance(3004, 1);
    const FGuid FullInputUid = Game->PlayerInventorySlots[0].ItemUid;
    Game->ItemInstancesByUid[FullInputUid].ItemId = 6009;
    Game->ItemInstancesByUid[FullInputUid].Quantity = 3;
    const int32 ItemCountBefore = Game->ItemInstancesByUid.Num();
    TestTrue(TEXT("Full inventory failure handled with feedback"), Barter->TryBarter(Pawn, false));
    TestEqual(TEXT("Full inventory restores payment"), Game->CountInventoryItemById(6009), 3);
    TestEqual(TEXT("Full inventory restores item map"), Game->ItemInstancesByUid.Num(), ItemCountBefore);
    TestEqual(TEXT("Full inventory preserves slot identity"), Game->PlayerInventorySlots[0].ItemUid, FullInputUid);
    TestEqual(TEXT("No overflow reward queue for barter"), Game->PendingQuestRewards.Num(), 0);
    TestFalse(TEXT("Full inventory leaves acquisition history unchanged"), Game->EverAcquiredItemIds.Contains(6010));
    TestFalse(TEXT("Invalid output cannot consume inputs"), Game->ExchangeCarriedItemBatch(6009,2,MAX_int32,1,false));
    TestEqual(TEXT("Invalid recipe preserves inputs"), Game->CountInventoryItemById(6009), 3);
    TestFalse(TEXT("Facility production still requires completed world progress"),
        Game->ProduceFacilityBatch(TEXT("test.unrepaired"),6009,2,6010,1,false));
    return true;
}
#endif
