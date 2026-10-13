#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Character/TunaSweeperQuestPresenceNpcActor.h"
#include "Interaction/TunaSweeperNpcFeedingComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperToastSubsystem.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/TunaSweeperToastWidget.h"

namespace FeedingTests
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperFeedingRewardsTest,
    "TunaSweeper.Feeding.ExchangeAndRewardQueue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperFeedingRewardsTest::RunTest(const FString& Parameters)
{
    if (TunaSweeperBuildFlavor::IsDemo()) return true;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FeedingTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    // The fixture never loads or writes a real gameplay slot.
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        FeedingTests::FContextAccess::Attach(Game.Get(), nullptr); World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    auto* Toasts = Game->GetSubsystem<UTunaSweeperToastSubsystem>();
    auto* Npc = World->SpawnActor<ATunaSweeperQuestPresenceNpcActor>();
    auto* Pawn = World->SpawnActor<APawn>();
    auto* Root = NewObject<USceneComponent>(Pawn); Pawn->SetRootComponent(Root); Root->RegisterComponent();
    Npc->PresenceRole = ETunaSweeperQuestNpcPresenceRole::BaseResident;
    Npc->SetActorHiddenInGame(false);
    auto* Feed = Npc->FeedingComponent.Get();
    Feed->RequiredQuestId = 99101;
    Feed->FoodRewards = {{3007,6006},{6007,6027},{6008,6028}};
    FTunaSweeperQuestDefinition Quest; Quest.QuestId = Feed->RequiredQuestId;
    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset();
    Quests->QuestDefinitions.Add(Quest.QuestId, Quest);
    Quests->SetQuestState(Quest.QuestId, ETunaSweeperQuestState::RewardAvailable);
    TestFalse(TEXT("Feeding stays locked until first reward is claimed"), Feed->IsUnlocked());
    Quests->SetQuestState(Quest.QuestId, ETunaSweeperQuestState::RewardCompleted);
    Feed->RefreshAvailability();
    TestTrue(TEXT("Completed prerequisite unlocks feeding"), Feed->IsUnlocked());
    TestEqual(TEXT("Additional feeding action is available"), Feed->GetInteractionType(), ETunaSweeperInteractionType::NpcFeeding);
    Npc->PresenceRole = ETunaSweeperQuestNpcPresenceRole::FieldOffer;
    TestFalse(TEXT("Field NPC does not offer resident feeding"), Feed->IsUnlocked());
    Npc->PresenceRole = ETunaSweeperQuestNpcPresenceRole::BaseResident;
    const auto ResetItems = [&] {
        Game->ItemInstancesByUid.Reset(); Game->ResetPlayerSlotArrays();
        Game->PlayerInventorySlots.SetNum(12); Game->AuxiliaryBagSlots.Reset();
        Game->StorageSlots.SetNum(1); Game->PendingQuestRewards.Reset(); Toasts->ClearToasts();
    };
    for (const auto& Pair : Feed->FoodRewards)
    {
        ResetItems(); Game->PlayerInventorySlots[0].ItemUid = Game->CreateItemInstance(Pair.Key,1);
        TestTrue(TEXT("Single food can be fed"), Feed->TryFeed(Pawn));
        TestEqual(TEXT("Exactly one food consumed"), Game->CountInventoryItemById(Pair.Key), 0);
        TestEqual(TEXT("Matching reward granted exactly once"), Game->CountInventoryItemById(Pair.Value), 1);
        TestEqual(TEXT("Matching reward toast starts"), Toasts->ActiveReward.ItemId, Pair.Value);
    }
    ResetItems();
    int32 Index=0;
    for (const auto& Pair : Feed->FoodRewards) Game->PlayerInventorySlots[Index++].ItemUid=Game->CreateItemInstance(Pair.Key,1);
    for (int32 Step=0; Step<3; ++Step)
    {
        TMap<int32,int32> Before;
        for (const auto& Pair : Feed->FoodRewards) Before.Add(Pair.Key,Game->CountInventoryItemById(Pair.Key));
        TestTrue(TEXT("Random choice among carried foods succeeds"), Feed->TryFeed(Pawn));
        int32 Changed=0;
        for (const auto& Pair : Feed->FoodRewards)
            if (Game->CountInventoryItemById(Pair.Key)<Before[Pair.Key])
            { ++Changed; TestEqual(TEXT("Random food maps to its own reward"),Game->CountInventoryItemById(Pair.Value),1); }
        TestEqual(TEXT("One and only one food type consumed per interaction"),Changed,1);
    }
    ResetItems();
    Game->StorageSlots[0].ItemUid=Game->CreateItemInstance(3007,1);
    TestTrue(TEXT("Storage-only food gives no-food feedback"),Feed->TryFeed(Pawn));
    TestFalse(TEXT("No reward toast for no food"),Toasts->bRewardActive);
    TestTrue(TEXT("Storage item not consumed"),Game->StorageSlots[0].ItemUid.IsValid());
    TestTrue(TEXT("Hungry response scheduled"),World->GetTimerManager().IsTimerActive(Feed->HungryTimer));
    TestTrue(TEXT("Hungry response delay is one second"),FMath::IsNearlyEqual(World->GetTimerManager().GetTimerRemaining(Feed->HungryTimer),1.0f));
    Feed->TryFeed(Pawn);
    World->GetTimerManager().ClearTimer(Feed->HungryTimer);
    ResetItems(); Game->PlayerInventorySlots.SetNum(1);
    Game->PlayerInventorySlots[0].ItemUid=Game->CreateItemInstance(3007,2);
    bool Reentered=false;
    const auto Handle=Game->OnInventoryStateChanged.AddLambda([&]{Reentered|=Feed->TryFeed(Pawn);});
    TestTrue(TEXT("Full inventory still accepts feeding"),Feed->TryFeed(Pawn));
    Game->OnInventoryStateChanged.Remove(Handle);
    TestFalse(TEXT("Inventory delegate cannot reenter feeding"),Reentered);
    TestEqual(TEXT("Full inventory consumes only one food"),Game->CountInventoryItemById(3007),1);
    TestEqual(TEXT("Overflow reward retained in receiving storage"),Game->PendingQuestRewards.Num(),1);
    TestEqual(TEXT("Overflow reward identity"),Game->PendingQuestRewards[0].ItemId,6006);
    TestFalse(TEXT("Invalid reward does not consume food"),Game->ExchangeCarriedItemForReward(3007,MAX_int32));
    TestEqual(TEXT("Invalid exchange leaves food"),Game->CountInventoryItemById(3007),1);
    Toasts->ClearToasts();
    Toasts->ShowToast(FText::FromString(TEXT("Ordinary test toast")),5.0f);
    Toasts->ShowItemRewardToast(6006,1); Toasts->ShowItemRewardToast(6027,1); Toasts->ShowItemRewardToast(6028,1);
    TestEqual(TEXT("FIFO begins with first item"),Toasts->ActiveReward.ItemId,6006);
    TestEqual(TEXT("Two later items queued"),Toasts->RewardQueue.Num(),2);
    TestTrue(TEXT("Normal toast has an independent widget"),Toasts->RewardToastWidget!=Toasts->ActiveToastWidget);
    Toasts->Tick(0.99f); TestEqual(TEXT("Item remains visible before one second"),Toasts->ActiveReward.ItemId,6006);
    Toasts->Tick(0.02f); TestEqual(TEXT("Second item follows after one second"),Toasts->ActiveReward.ItemId,6027);
    TestTrue(TEXT("Normal toast is still active"),Toasts->bToastActive);
    Toasts->Tick(1.0f); TestEqual(TEXT("Third item preserves FIFO"),Toasts->ActiveReward.ItemId,6028);
    Toasts->Tick(1.0f); TestFalse(TEXT("Queue drains and hides reward toast"),Toasts->bRewardActive);
    Toasts->ShowItemRewardToast(6006,1);
    Toasts->HandleWorldCleanup(World,false,true);
    TestFalse(TEXT("Map cleanup discards transient reward queue"),Toasts->bRewardActive || !Toasts->RewardQueue.IsEmpty());
    return true;
}
#endif
