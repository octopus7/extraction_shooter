#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Character/TunaSweeperMoleCompanionActor.h"
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

namespace TunaSweeperQuestData
{
	bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

namespace TunaSweeperQuestSubmissionTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestSubmissionTest,
	"TunaSweeper.Quest.ItemSubmission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestSubmissionTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperQuestSubmissionTests;
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
	FTunaSweeperQuestProgressSaveData Progress;
	Progress.QuestId = TEXT("demo_q4_todays_reward");
	Progress.State = ETunaSweeperQuestState::Accepted;
	Quests->LoadQuestProgressFromSave({Progress}, NAME_None, 0);
	Quests->NotifyInteractionCompleted(TEXT("demo.canned_tuna.deliver"), TEXT("world_progress"));
	TestEqual(TEXT("An interaction event without submitted items cannot finish delivery"),
		Quests->GetQuestState(Progress.QuestId), ETunaSweeperQuestState::Accepted);
	FTunaSweeperQuestDefinition Demo;
	TestTrue(TEXT("Demo definition loads"), Quests->TryGetQuestDefinition(Progress.QuestId, Demo));
	TestEqual(TEXT("Demo retains existing objective ID"), Demo.Objectives[0].ObjectiveId, FName(TEXT("deliver_canned_tuna")));
	TestEqual(TEXT("Demo requires submitted can"), Demo.Objectives[0].ItemId, 3004);
	TestEqual(TEXT("Demo targets mole provider"), Demo.Objectives[0].TargetProviderId, FName(TEXT("provider.mole")));
	FName Submitted;
	TestFalse(TEXT("Missing can cannot submit"), Quests->TrySubmitItemsToProvider(TEXT("provider.mole"), Submitted, false));
	TestTrue(TEXT("Test inventory receives can"), Game->AddItemToFirstAvailableInventorySlot(3004, 1));
	TestFalse(TEXT("Wrong NPC cannot take the can"), Quests->TrySubmitItemsToProvider(TEXT("provider.other"), Submitted, false));
	TestEqual(TEXT("Wrong NPC keeps inventory"), Game->CountInventoryItemById(3004), 1);
	TestTrue(TEXT("Mole consumes authored can"), Quests->TrySubmitItemsToProvider(TEXT("provider.mole"), Submitted, false));
	TestEqual(TEXT("Correct quest selected"), Submitted, Progress.QuestId);
	TestEqual(TEXT("Submitted can is consumed"), Game->CountInventoryItemById(3004), 0);
	TestEqual(TEXT("Submission unlocks reward"), Quests->GetQuestState(Submitted), ETunaSweeperQuestState::RewardAvailable);
	TestTrue(TEXT("Reward retry does not need another can"), Quests->TrySubmitItemsToProvider(TEXT("provider.mole"), Submitted, false));
	TestEqual(TEXT("Retry consumes nothing"), Game->CountInventoryItemById(3004), 0);

	// A different giver, target, item and count exercise the same production submission path.
	Quests->QuestDefinitions.Reset();
	Quests->QuestProgressById.Reset();
	FTunaSweeperQuestDefinition Custom;
	Custom.QuestId = TEXT("test.delivery");
	Custom.ProviderId = TEXT("provider.giver");
	FTunaSweeperObjectiveDefinition First;
	First.ObjectiveId = TEXT("first");
	First.Type = ETunaSweeperObjectiveType::ItemSubmitted;
	First.ItemId = 6002;
	First.RequiredCount = 2;
	First.TargetProviderId = TEXT("provider.recipient");
	Custom.Objectives.Add(First);
	First.ObjectiveId = TEXT("second");
	First.RequiredCount = 3;
	Custom.Objectives.Add(First);
	Quests->QuestDefinitions.Add(Custom.QuestId, Custom);
	Game->AddItemToFirstAvailableInventorySlot(6002, 3);
	TestFalse(TEXT("Unaccepted quest cannot consume"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));
	Progress.QuestId = Custom.QuestId;
	Progress.State = ETunaSweeperQuestState::Accepted;
	Progress.ObjectiveProgress.Reset();
	Quests->LoadQuestProgressFromSave({Progress}, NAME_None, 0);
	TestFalse(TEXT("Two objectives require total five, not three"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));
	// The recipient need not own the quest; its interaction must remain reachable.
	for (auto& Objective : Quests->QuestDefinitions.FindChecked(Custom.QuestId).Objectives) Objective.TargetProviderId = TEXT("provider.mole");
	auto* Mole = World->SpawnActor<ATunaSweeperMoleCompanionActor>();
	TArray<UTunaSweeperInteractableComponent*> Interactables;
	Mole->GetComponents(Interactables);
	UTunaSweeperInteractableComponent* QuestInteractable = nullptr;
	for (auto* Interactable : Interactables)
		if (Interactable->GetInteractionType() == ETunaSweeperInteractionType::Quest) QuestInteractable = Interactable;
	auto* Interactions = World->GetSubsystem<UTunaSweeperInteractionSubsystem>();
	if (TestNotNull(TEXT("Recipient quest interaction"), QuestInteractable) && TestNotNull(TEXT("Interaction subsystem"), Interactions))
	{
		TestTrue(TEXT("Recipient has no giver-owned quest"), Mole->ResolveQuestId().IsNone());
		TestTrue(TEXT("Cross-provider delivery still offers recipient interaction"), Interactions->CanOfferInteraction(QuestInteractable));
	}
	Quests->QuestDefinitions.FindChecked(Custom.QuestId) = Custom;
	TestEqual(TEXT("Insufficient batch consumes nothing"), Game->CountInventoryItemById(6002), 3);
	TestEqual(TEXT("Insufficient batch advances nothing"), Quests->GetObjectiveProgressCount(Custom.QuestId, TEXT("first")), 0);
	Game->AddItemToFirstAvailableInventorySlot(6002, 3);
	bool bObserverSawCommittedState = false;
	bool bReentrantSubmission = false;
	const FDelegateHandle Observer = Game->OnInventoryStateChanged.AddLambda([&]()
	{
		bObserverSawCommittedState = Game->CountInventoryItemById(6002) == 1 &&
			Quests->GetQuestState(Custom.QuestId) == ETunaSweeperQuestState::RewardAvailable;
		FName ReentrantQuest;
		bReentrantSubmission = Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), ReentrantQuest, false);
	});
	TestTrue(TEXT("Exact combined requirements submit"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));
	Game->OnInventoryStateChanged.Remove(Observer);
	TestTrue(TEXT("Inventory observers see committed items and quest state together"), bObserverSawCommittedState);
	TestFalse(TEXT("Notification cannot re-enter submission"), bReentrantSubmission);
	TestEqual(TEXT("Only requested amount consumed"), Game->CountInventoryItemById(6002), 1);
	TestEqual(TEXT("First objective fulfilled"), Quests->GetObjectiveProgressCount(Custom.QuestId, TEXT("first")), 2);
	TestEqual(TEXT("Second objective fulfilled"), Quests->GetObjectiveProgressCount(Custom.QuestId, TEXT("second")), 3);

	TStrongObjectPtr<UTunaSweeperSaveGame> Save(NewObject<UTunaSweeperSaveGame>());
	Quests->ExportQuestProgressForSave(Save->QuestProgressStates, Save->TrackedQuestId, Save->QuestCoinBalance);
	Save->InventorySlots = Game->PlayerInventorySlots;
	Game->ItemInstancesByUid.GenerateValueArray(Save->ItemInstances);
	TArray<uint8> Bytes;
	TestTrue(TEXT("Submission serializes in existing save format"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bytes));
	TStrongObjectPtr<UTunaSweeperSaveGame> Loaded(Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
	if (!TestNotNull(TEXT("Submission save reloads"), Loaded.Get())) return false;
	Quests->LoadQuestProgressFromSave(Loaded->QuestProgressStates, Loaded->TrackedQuestId, Loaded->QuestCoinBalance);
	Game->PlayerInventorySlots = Loaded->InventorySlots;
	Game->ItemInstancesByUid.Reset();
	for (const auto& Item : Loaded->ItemInstances) Game->ItemInstancesByUid.Add(Item.Uid, Item);
	TestTrue(TEXT("Reloaded submission retries reward"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));
	TestEqual(TEXT("Reload does not consume again"), Game->CountInventoryItemById(6002), 1);
	Quests->QuestProgressById.FindChecked(Custom.QuestId).State = ETunaSweeperQuestState::RewardCompleted;
	TestFalse(TEXT("Reward completed quest never submits again"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));

	const FString ValidObjective = TEXT(R"({"objective_id":"test","type":"item_submitted","item_id":6002,"required_count":2,"target_provider_id":"provider.recipient"})");
	for (const FString& Json : { ValidObjective.Replace(TEXT("6002"), TEXT("-1")),
		ValidObjective.Replace(TEXT(":2,"), TEXT(":2.5,")), ValidObjective.Replace(TEXT(":2,"), TEXT(":0,")),
		ValidObjective.Replace(TEXT("provider.recipient"), TEXT("")) })
	{
		TSharedPtr<FJsonObject> Object;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object);
		FTunaSweeperObjectiveDefinition Parsed;
		TestFalse(TEXT("Invalid submission condition is rejected"), TunaSweeperQuestData::ParseObjective(Object, Parsed));
	}
	return true;
}

#endif
