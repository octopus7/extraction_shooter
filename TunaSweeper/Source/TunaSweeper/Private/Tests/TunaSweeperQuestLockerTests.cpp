#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Character/TunaSweeperQuestPresenceNpcActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UI/TunaSweeperQuestWidget.h"
#include "Components/Button.h"

namespace TunaSweeperQuestData
{
	bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

namespace TunaSweeperQuestLockerTests
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestLockerTest,
    "TunaSweeper.Quest.RewardLocker", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestLockerTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperQuestLockerTests;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	// Initialize subsystems without reading/writing the user's save slots.
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	Game->ResetPlayerSlotArrays();
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	if (!TestNotNull(TEXT("Quest subsystem"), Quests)) return false;

    Quests->LoadQuestData(false);
    Quests->QuestDefinitions.Reset();
    Quests->QuestProgressById.Reset();
    Game->ItemInstancesByUid.Reset();
    Game->ResetPlayerSlotArrays();
    Game->StorageSlots.SetNum(2);
    const auto Stored = Game->CreateItemInstance(3004, 1);
    Game->StorageSlots[0].ItemUid = Stored;
    FTunaSweeperQuestDefinition Quest;
    Quest.QuestId = 99001;
    Quest.ProviderId = TEXT("provider.test");
    FTunaSweeperObjectiveDefinition Objective;
    Objective.ObjectiveId = TEXT("supply");
    Objective.Type = ETunaSweeperObjectiveType::ItemSubmitted;
    Objective.ItemId = 3004;
    Objective.RequiredCount = 1;
    Objective.TargetProviderId = Quest.ProviderId;
    Quest.Objectives.Add(Objective);
    Quests->QuestDefinitions.Add(Quest.QuestId, Quest);
    Quests->SetQuestState(Quest.QuestId, ETunaSweeperQuestState::Accepted);
    int32 Submitted = 0;
    TestTrue(TEXT("Storage-only item can be submitted"), Quests->TrySubmitItemsToProvider(Quest.ProviderId, Submitted, false));
    TestTrue(TEXT("Submitted storage slot clears"), Game->StorageSlots[0].IsEmpty());
    Game->PlayerInventorySlots.Reset();
    FTunaSweeperItemStack Reward;
    Reward.ItemId = 3004; Reward.Quantity = 1;
    TestTrue(TEXT("Reward is accepted even when inventory is full"), Game->GrantQuestItemRewards({Reward}));
    TestEqual(TEXT("Full bag puts one stack in escrow"), Game->GetPendingQuestRewards().Num(), 1);
    TestEqual(TEXT("Escrow does not qualify as possession"), Game->CountQuestSubmissionItemById(3004), 0);
    TestFalse(TEXT("Receive cannot lose reward when bag full"), Game->ReceivePendingQuestReward(0));
    TestEqual(TEXT("Failed receive preserves escrow"), Game->GetPendingQuestRewards().Num(), 1);
    Game->PlayerInventorySlots.SetNum(2);
    TestTrue(TEXT("Receive succeeds after space is freed"), Game->ReceivePendingQuestReward(0));
    TestEqual(TEXT("Receive removes escrow entry"), Game->GetPendingQuestRewards().Num(), 0);
    TestEqual(TEXT("Receive transfers exactly one"), Game->CountInventoryItemById(3004), 1);
    TestFalse(TEXT("Repeated receive cannot duplicate item"), Game->ReceivePendingQuestReward(0));

    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset();
    Quest.QuestId = 99002;
    Quest.Objectives[0].Type = ETunaSweeperObjectiveType::ItemPossessed;
    Quest.Rewards.Items = {Reward};
    Quests->QuestDefinitions.Add(Quest.QuestId, Quest);
    TestTrue(TEXT("Quest can be accepted with an already owned item"), Quests->AcceptQuest(Quest.QuestId));
    TestTrue(TEXT("Existing ownership makes quest ready"), Quests->CanClaimQuestReward(Quest.QuestId));
    Game->ConsumeInventoryItemById(3004, 1);
    TestFalse(TEXT("Spending item revokes readiness"), Quests->CanClaimQuestReward(Quest.QuestId));
    TestFalse(TEXT("Missing ownership blocks claim"), Quests->ClaimQuestReward(Quest.QuestId));
    Game->StorageSlots[0].ItemUid = Game->CreateItemInstance(3004, 1);
    Game->NotifyQuestItemMutation();
    TestTrue(TEXT("Storage ownership makes quest ready"), Quests->CanClaimQuestReward(Quest.QuestId));
    TestFalse(TEXT("Wrong provider cannot submit possession quest"), Quests->TrySubmitItemsToProvider(TEXT("provider.wrong"), Submitted, false));
    TestTrue(TEXT("Correct provider finds possession quest"), Quests->TrySubmitItemsToProvider(Quest.ProviderId, Submitted, false));
    bool Reentered = false;
    const auto Handle = Game->OnInventoryStateChanged.AddLambda([&] { Reentered |= Quests->ClaimQuestReward(Quest.QuestId); });
    TestTrue(TEXT("Submission and return reward commit"), Quests->ClaimQuestReward(Quest.QuestId));
    Game->OnInventoryStateChanged.Remove(Handle);
    TestFalse(TEXT("Inventory observer cannot double claim"), Reentered);
    TestTrue(TEXT("Submission consumes from storage"), Game->StorageSlots[0].IsEmpty());
    TestEqual(TEXT("Reward returns one item to inventory"), Game->CountInventoryItemById(3004), 1);
    TestFalse(TEXT("Completed quest cannot claim again"), Quests->ClaimQuestReward(Quest.QuestId));

    // Same item across two requirements must not be counted twice.
    Quest.QuestId = 99003;
    const auto SecondObjective = Quest.Objectives[0];
    Quest.Objectives.Add(SecondObjective); Quest.Objectives[1].ObjectiveId = TEXT("second");
    Quests->QuestDefinitions.Add(Quest.QuestId, Quest);
    Quests->AcceptQuest(Quest.QuestId);
    TestFalse(TEXT("One item cannot fulfill two owned objectives"), Quests->CanClaimQuestReward(Quest.QuestId));
    Game->StorageSlots[0].ItemUid = Game->CreateItemInstance(3004, 1);
    Game->NotifyQuestItemMutation();
    TestTrue(TEXT("Inventory and storage quantities combine"), Quests->CanClaimQuestReward(Quest.QuestId));
    Quest.Rewards.Items[0].ItemId = MAX_int32;
    Quests->QuestDefinitions[Quest.QuestId] = Quest;
    TestFalse(TEXT("Invalid reward prevents consumption"), Quests->ClaimQuestReward(Quest.QuestId));
    TestEqual(TEXT("Invalid reward leaves both sources intact"), Game->CountQuestSubmissionItemById(3004), 2);

    // Mixed objectives remain submittable; owned quantities are consumed at reward claim only.
    Quest.QuestId = 99004;
    Quest.Rewards.Items = {Reward};
    Quest.Objectives[1].Type = ETunaSweeperObjectiveType::ItemSubmitted;
    Quest.Objectives[1].ItemId = 6020;
    Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset();
    Quests->QuestDefinitions.Add(Quest.QuestId, Quest);
    Quests->AcceptQuest(Quest.QuestId);
    Game->StorageSlots[1].ItemUid = Game->CreateItemInstance(6020, 1);
    Game->NotifyQuestItemMutation();
    TestTrue(TEXT("Mixed ownership and submission can submit"), Quests->TrySubmitItemsToProvider(Quest.ProviderId, Submitted, false));
    TestTrue(TEXT("Mixed quest becomes ready"), Quests->CanClaimQuestReward(Quest.QuestId));
    TestEqual(TEXT("Owned requirement is not prematurely consumed"), Game->CountQuestSubmissionItemById(3004), 2);
    TestTrue(TEXT("Mixed quest can claim"), Quests->ClaimQuestReward(Quest.QuestId));

    // Stale pre-change acquisition progress is reconciled against the final restored inventory.
    Quest.QuestId = 99005; Quest.Objectives.SetNum(1);
    Quests->QuestDefinitions.Add(Quest.QuestId, Quest);
    FTunaSweeperNumericQuestProgressSaveData Legacy;
    Legacy.QuestId = Quest.QuestId; Legacy.State = ETunaSweeperQuestState::RewardAvailable;
    FTunaSweeperObjectiveProgressSaveData LegacyCount;
    LegacyCount.ObjectiveId = Quest.Objectives[0].ObjectiveId; LegacyCount.CurrentCount = 1;
    Legacy.ObjectiveProgress.Add(LegacyCount);
    Quests->LoadQuestProgressFromSave({Legacy}, 0, 0);
    Game->ItemInstancesByUid.Reset(); Game->ResetPlayerSlotArrays(); Game->StorageSlots.Reset();
    Game->CompleteInventoryStateInitialization();
    TestFalse(TEXT("Loaded stale acquisition completion is not ownership"), Quests->CanClaimQuestReward(Quest.QuestId));

    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Save->PendingQuestRewards = {Reward};
    TArray<uint8> Bytes;
    TestTrue(TEXT("Escrow serializes with save"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Restored = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Escrow save roundtrip"), Restored))
    {
        TestEqual(TEXT("Pending reward persists"), Restored->PendingQuestRewards.Num(), 1);
        if (Restored->PendingQuestRewards.Num() == 1)
        { TestEqual(TEXT("Item ID persists"), Restored->PendingQuestRewards[0].ItemId, 3004); TestEqual(TEXT("Quantity persists"), Restored->PendingQuestRewards[0].Quantity, 1); }
    }
    Game->BeginCombatTestSession();
    TestFalse(TEXT("Combat test cannot grant persistent reward"), Game->GrantQuestItemRewards({Reward}));
    TestFalse(TEXT("Combat test cannot receive persistent reward"), Game->ReceivePendingQuestReward(0));
    Game->EndCombatTestSession();
    return true;
}
#endif
