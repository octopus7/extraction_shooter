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
	// Submission behavior is tested against a synthetic quest, independent of authored content.
	Quests->LoadQuestData(false);
	Quests->QuestDefinitions.Reset();
	FTunaSweeperQuestDefinition Delivery;
	Delivery.QuestId = 101;
	Delivery.ProviderId = TEXT("provider.mole");
	FTunaSweeperObjectiveDefinition DeliveryObjective;
	DeliveryObjective.ObjectiveId = TEXT("deliver_canned_tuna");
	DeliveryObjective.Type = ETunaSweeperObjectiveType::ItemSubmitted;
	DeliveryObjective.ItemId = 3004;
	DeliveryObjective.RequiredCount = 1;
	DeliveryObjective.TargetProviderId = TEXT("provider.mole");
	Delivery.Objectives.Add(DeliveryObjective);
	Quests->QuestDefinitions.Add(Delivery.QuestId, Delivery);
	FTunaSweeperNumericQuestProgressSaveData Progress;
	Progress.QuestId = Delivery.QuestId;
	Progress.State = ETunaSweeperQuestState::Accepted;
	Quests->LoadQuestProgressFromSave({Progress}, 0, 0);
	Quests->NotifyInteractionCompleted(TEXT("demo.canned_tuna.deliver"), TEXT("world_progress"));
	TestEqual(TEXT("An interaction event without submitted items cannot finish delivery"),
		Quests->GetQuestState(Progress.QuestId), ETunaSweeperQuestState::Accepted);
	int32 Submitted = 0;
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

	// Reuse the achieved synthetic quest to exercise both UI entry paths without claiming rewards.
	auto* Panel = CreateWidget<UTunaSweeperQuestWidget>(Game.Get());
	if (!TestNotNull(TEXT("Quest panel"), Panel)) return false;
	Panel->InitializeQuest(Submitted);
	TestFalse(TEXT("Inventory entry cannot claim rewards"), Panel->IsPrimaryButtonEnabled(Submitted));
	Panel->HandlePrimaryButtonClicked();
	TestEqual(TEXT("Direct inventory click cannot bypass the disabled button"), Quests->GetQuestState(Submitted), ETunaSweeperQuestState::RewardAvailable);
	Panel->InitializeQuest(Submitted, TEXT("provider.other"));
	TestFalse(TEXT("Wrong NPC cannot claim rewards"), Panel->IsPrimaryButtonEnabled(Submitted));
	Panel->HandlePrimaryButtonClicked();
	TestEqual(TEXT("Wrong NPC click preserves progress"), Quests->GetQuestState(Submitted), ETunaSweeperQuestState::RewardAvailable);
	Panel->InitializeQuest(Submitted, TEXT("provider.mole"));
	TestTrue(TEXT("Assigned NPC enables reward claim"), Panel->IsPrimaryButtonEnabled(Submitted));
	int32 ResolvedQuest = 0;
	TestTrue(TEXT("Omitted completion provider resolves reward at giver"),
		Quests->TryResolveQuestForProvider(TEXT("provider.mole"), 0, ResolvedQuest));
	Quests->QuestDefinitions[Submitted].CompletionProviderId = TEXT("provider.recipient");
	TestFalse(TEXT("Giver cannot claim a reward assigned to a different NPC"), Panel->IsPrimaryButtonEnabled(Submitted));
	Panel->HandlePrimaryButtonClicked();
	TestEqual(TEXT("Giver click cannot bypass completion NPC"), Quests->GetQuestState(Submitted), ETunaSweeperQuestState::RewardAvailable);
	TestFalse(TEXT("Giver fallback cannot route a reward assigned to another NPC"),
		Quests->TryResolveQuestForProvider(TEXT("provider.mole"), Submitted, ResolvedQuest));
	TestTrue(TEXT("Completion NPC resolves the achieved quest"),
		Quests->TryResolveQuestForProvider(TEXT("provider.recipient"), 0, ResolvedQuest));
	TestEqual(TEXT("Completion NPC resolves correct reward"), ResolvedQuest, Submitted);
	auto* FacilityNpc = World->SpawnActor<ATunaSweeperFacilityNpcActor>();
	if (!TestNotNull(TEXT("Facility provider actor"), FacilityNpc)) return false;
	FacilityNpc->QuestFallbackId = Submitted;
	FacilityNpc->QuestProviderId = TEXT("provider.mole");
	TestEqual(TEXT("Facility giver fallback cannot bypass separate reward NPC"), FacilityNpc->ResolveQuestId(), 0);
	FacilityNpc->QuestProviderId = TEXT("provider.recipient");
	TestEqual(TEXT("Facility completion NPC resolves achieved quest"), FacilityNpc->ResolveQuestId(), Submitted);
	TestFalse(TEXT("Unrelated NPC fallback cannot route the reward"),
		Quests->TryResolveQuestForProvider(TEXT("provider.other"), Submitted, ResolvedQuest));
	Panel->InitializeQuest(Submitted, TEXT("provider.recipient"));
	TestTrue(TEXT("Completion NPC enables reward claim"), Panel->IsPrimaryButtonEnabled(Submitted));
	Quests->QuestDefinitions[Submitted].CompletionProviderId = NAME_None;
	TestFalse(TEXT("Empty completion provider removes recipient reward access"), Panel->IsPrimaryButtonEnabled(Submitted));
	Panel->InitializeQuest(Submitted, TEXT("provider.mole"));
	TestTrue(TEXT("Empty completion provider restores giver reward access"), Panel->IsPrimaryButtonEnabled(Submitted));
	Panel->ResetQuestSelection();
	TestFalse(TEXT("Closing clears NPC reward access"), Panel->IsPrimaryButtonEnabled(Submitted));
	Panel->InitializeQuest(Submitted, TEXT("provider.mole"));
	Panel->InitializeQuest(Submitted);
	TestFalse(TEXT("Reopening from inventory clears old NPC access"), Panel->IsPrimaryButtonEnabled(Submitted));
	Quests->QuestDefinitions[Submitted].ProviderId = NAME_None;
	Panel->InitializeQuest(Submitted, TEXT("provider.mole"));
	TestFalse(TEXT("Unassigned quest is not claimable through any NPC"), Panel->IsPrimaryButtonEnabled(Submitted));
	Panel->ResetQuestSelection();

	// A different giver, target, item and count exercise the same production submission path.
	FTunaSweeperQuestDefinition Offered = Delivery;
	Offered.QuestId = 201;
	Offered.CompletionProviderId = TEXT("provider.recipient");
	Quests->QuestDefinitions.Add(201, Offered);
	FacilityNpc->QuestFallbackId = 201;
	TestEqual(TEXT("Facility completion fallback cannot offer giver's available quest"), FacilityNpc->ResolveQuestId(), 0);
	FacilityNpc->Destroy();
	TestFalse(TEXT("Completion NPC does not offer unaccepted quest"),
		Quests->TryResolveQuestForProvider(TEXT("provider.recipient"), 201, ResolvedQuest));
	Panel->InitializeQuest(201, TEXT("provider.recipient"));
	TestEqual(TEXT("Completion NPC has no bulk acceptance candidates"), Panel->GetAcceptAllQuestIds().Num(), 0);
	TestFalse(TEXT("Completion NPC cannot accept giver quest"), Panel->IsPrimaryButtonEnabled(201));
	TArray<FTunaSweeperQuestDefinition> VisibleOffers;
	Panel->BuildFilteredQuestDefinitions(*Quests, VisibleOffers);
	TestTrue(TEXT("Completion NPC hides another giver's unaccepted quest"), VisibleOffers.IsEmpty());
	TestEqual(TEXT("Completion NPC available badge excludes another giver"),
		Panel->CountVisibleQuestsForFilter(UTunaSweeperQuestWidget::EQuestListFilter::Available), 0);

	Panel->InitializeQuest(201, TEXT("provider.mole"));
	TestTrue(TEXT("Giver still offers quest with separate completion NPC"),
		Quests->TryResolveQuestForProvider(TEXT("provider.mole"), 0, ResolvedQuest));
	TestTrue(TEXT("Giver can accept quest with separate completion NPC"), Panel->IsPrimaryButtonEnabled(201));
	TestEqual(TEXT("One available quest is counted"), Panel->GetAcceptAllQuestIds().Num(), 1);
	TestEqual(TEXT("Single available quest hides bulk action"), Panel->AcceptAllButton->GetVisibility(), ESlateVisibility::Collapsed);
	Panel->HandleAcceptAllClicked();
	TestEqual(TEXT("Bulk click with one candidate does nothing"), Quests->GetQuestState(201), ETunaSweeperQuestState::Available);
	Offered.QuestId = 202;
	Quests->QuestDefinitions.Add(202, Offered);
	Offered.QuestId = 203;
	Offered.ProviderId = TEXT("provider.other");
	Quests->QuestDefinitions.Add(203, Offered);
	Offered.QuestId = 204;
	Offered.ProviderId = TEXT("provider.mole");
	Offered.RequiredCompletedQuestIds = { 203 };
	Quests->QuestDefinitions.Add(204, Offered);
	TestEqual(TEXT("Only eligible quests for this NPC count"), Panel->GetAcceptAllQuestIds().Num(), 2);
	Panel->BuildFilteredQuestDefinitions(*Quests, VisibleOffers);
	TestEqual(TEXT("NPC offer list contains only its two eligible quests"), VisibleOffers.Num(), 2);
	TestEqual(TEXT("NPC available badge matches offer list"),
		Panel->CountVisibleQuestsForFilter(UTunaSweeperQuestWidget::EQuestListFilter::Available), 2);

	Panel->RefreshQuestView();
	TestEqual(TEXT("Two available quests show bulk action"), Panel->AcceptAllButton->GetVisibility(), ESlateVisibility::Visible);
	Panel->InitializeQuest(201);
	TestEqual(TEXT("Inventory has no bulk candidates"), Panel->GetAcceptAllQuestIds().Num(), 0);
	TestEqual(TEXT("Inventory hides bulk action"), Panel->AcceptAllButton->GetVisibility(), ESlateVisibility::Collapsed);
	Panel->HandleAcceptAllClicked();
	TestEqual(TEXT("Inventory bulk click cannot accept"), Quests->GetQuestState(201), ETunaSweeperQuestState::Available);
	Panel->ResetQuestSelection();

	Quests->QuestDefinitions.Reset();
	Quests->QuestProgressById.Reset();
	FTunaSweeperQuestDefinition Custom;
	Custom.QuestId = 102;
	Custom.ProviderId = TEXT("provider.giver");
	Custom.CompletionProviderId = TEXT("provider.completion");
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
	Quests->LoadQuestProgressFromSave({Progress}, 0, 0);
	TestTrue(TEXT("Accepted quest remains reachable at giver"),
		Quests->TryResolveQuestForProvider(TEXT("provider.giver"), 0, ResolvedQuest));
	TestFalse(TEXT("Completion NPC does not expose a reward before objectives complete"),
		Quests->TryResolveQuestForProvider(TEXT("provider.completion"), Custom.QuestId, ResolvedQuest));
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
		TestTrue(TEXT("Recipient has no giver-owned quest"), Mole->ResolveQuestId() == 0);
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
		int32 ReentrantQuest = 0;
		bReentrantSubmission = Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), ReentrantQuest, false);
	});
	TestTrue(TEXT("Exact combined requirements submit"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));
	Game->OnInventoryStateChanged.Remove(Observer);
	TestTrue(TEXT("Inventory observers see committed items and quest state together"), bObserverSawCommittedState);
	TestFalse(TEXT("Notification cannot re-enter submission"), bReentrantSubmission);
	TestEqual(TEXT("Only requested amount consumed"), Game->CountInventoryItemById(6002), 1);
	TestEqual(TEXT("First objective fulfilled"), Quests->GetObjectiveProgressCount(Custom.QuestId, TEXT("first")), 2);
	TestEqual(TEXT("Second objective fulfilled"), Quests->GetObjectiveProgressCount(Custom.QuestId, TEXT("second")), 3);
	TestFalse(TEXT("Item recipient cannot claim a reward assigned to a third NPC"),
		Quests->TryResolveQuestForProvider(TEXT("provider.recipient"), Custom.QuestId, ResolvedQuest));
	TestTrue(TEXT("Separate completion NPC resolves reward after item submission"),
		Quests->TryResolveQuestForProvider(TEXT("provider.completion"), 0, ResolvedQuest));
	Quests->QuestDefinitions.FindChecked(Custom.QuestId).CompletionProviderId = TEXT("provider.mole");
	TestEqual(TEXT("Actor resolves completion quest for reward marker and interaction"), Mole->ResolveQuestId(), Custom.QuestId);
	Quests->QuestDefinitions.FindChecked(Custom.QuestId).CompletionProviderId = Custom.CompletionProviderId;

	TStrongObjectPtr<UTunaSweeperSaveGame> Save(NewObject<UTunaSweeperSaveGame>());
	Quests->ExportQuestProgressForSave(Save->NumericQuestProgressStates, Save->NumericTrackedQuestId, Save->QuestCoinBalance);
	Save->InventorySlots = Game->PlayerInventorySlots;
	Game->ItemInstancesByUid.GenerateValueArray(Save->ItemInstances);
	TArray<uint8> Bytes;
	TestTrue(TEXT("Submission serializes in existing save format"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bytes));
	TStrongObjectPtr<UTunaSweeperSaveGame> Loaded(Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
	if (!TestNotNull(TEXT("Submission save reloads"), Loaded.Get())) return false;
	Quests->LoadQuestProgressFromSave(Loaded->NumericQuestProgressStates, Loaded->NumericTrackedQuestId, Loaded->QuestCoinBalance);
	Game->PlayerInventorySlots = Loaded->InventorySlots;
	Game->ItemInstancesByUid.Reset();
	for (const auto& Item : Loaded->ItemInstances) Game->ItemInstancesByUid.Add(Item.Uid, Item);
	TestTrue(TEXT("Reloaded submission retries reward"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));
	TestEqual(TEXT("Reload does not consume again"), Game->CountInventoryItemById(6002), 1);
	Quests->QuestProgressById.FindChecked(Custom.QuestId).State = ETunaSweeperQuestState::RewardCompleted;
	TestFalse(TEXT("Reward completed quest never submits again"), Quests->TrySubmitItemsToProvider(TEXT("provider.recipient"), Submitted, false));

	// Bind real actor lifecycle listeners, then restore progress as a save load does.
	// No manual refresh follows the loads: missing subscriptions must fail these checks.
	Quests->LoadQuestProgressFromSave({}, 0, 0);
	FActorSpawnParameters PresenceSpawnParameters;
	PresenceSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* FieldNpc = World->SpawnActor<ATunaSweeperQuestPresenceNpcActor>(PresenceSpawnParameters);
	auto* BaseNpc = World->SpawnActor<ATunaSweeperQuestPresenceNpcActor>(PresenceSpawnParameters);
	if (!TestNotNull(TEXT("Field presence actor"), FieldNpc) || !TestNotNull(TEXT("Base presence actor"), BaseNpc)) return false;
	FieldNpc->PresenceQuestId = Custom.QuestId;
	FieldNpc->PresenceRole = ETunaSweeperQuestNpcPresenceRole::FieldOffer;
	BaseNpc->PresenceQuestId = Custom.QuestId;
	BaseNpc->PresenceRole = ETunaSweeperQuestNpcPresenceRole::BaseResident;
	FieldNpc->DispatchBeginPlay();
	BaseNpc->DispatchBeginPlay();
	TestFalse(TEXT("Available field NPC appears on begin play"), FieldNpc->IsHidden());
	TestTrue(TEXT("Available field NPC collision is enabled"), FieldNpc->GetActorEnableCollision());
	TestTrue(TEXT("Available quest does not spawn base resident"), BaseNpc->IsHidden());
	TestFalse(TEXT("Absent base resident cannot collide"), BaseNpc->GetActorEnableCollision());
	Progress.QuestId = Custom.QuestId;
	Progress.ObjectiveProgress.Reset();
	for (const auto State : { ETunaSweeperQuestState::Accepted, ETunaSweeperQuestState::RewardAvailable })
	{
		Progress.State = State;
		Quests->LoadQuestProgressFromSave({Progress}, 0, 0);
		TestTrue(TEXT("Accepted or unclaimed quest hides field NPC after load notification"), FieldNpc->IsHidden());
		TestFalse(TEXT("Accepted or unclaimed quest disables field collision"), FieldNpc->GetActorEnableCollision());
		TestTrue(TEXT("Accepted or unclaimed quest keeps base resident hidden"), BaseNpc->IsHidden());
		TestFalse(TEXT("Accepted or unclaimed quest keeps base collision disabled"), BaseNpc->GetActorEnableCollision());
	}
	Progress.State = ETunaSweeperQuestState::RewardCompleted;
	Quests->LoadQuestProgressFromSave({Progress}, 0, 0);
	TestTrue(TEXT("Reward completion keeps field NPC hidden"), FieldNpc->IsHidden());
	TestFalse(TEXT("Reward completion keeps field collision disabled"), FieldNpc->GetActorEnableCollision());
	TestFalse(TEXT("Reward completion reveals base resident through load notification"), BaseNpc->IsHidden());
	TestTrue(TEXT("Reward completion enables base resident collision"), BaseNpc->GetActorEnableCollision());
	Quests->LoadQuestProgressFromSave({}, 0, 0);
	TestFalse(TEXT("Loading pre-accept progress restores field NPC"), FieldNpc->IsHidden());
	TestTrue(TEXT("Loading pre-accept progress restores field collision"), FieldNpc->GetActorEnableCollision());
	TestTrue(TEXT("Loading pre-accept progress removes base resident"), BaseNpc->IsHidden());
	TestFalse(TEXT("Loading pre-accept progress removes base collision"), BaseNpc->GetActorEnableCollision());
	Quests->QuestDefinitions.FindChecked(Custom.QuestId).RequiredCompletedQuestIds = { 999999 };
	Quests->LoadQuestProgressFromSave({}, 0, 0);
	TestTrue(TEXT("Locked available quest does not expose field NPC"), FieldNpc->IsHidden());
	TestFalse(TEXT("Locked available quest disables field collision"), FieldNpc->GetActorEnableCollision());
	Quests->QuestDefinitions.Remove(Custom.QuestId);
	Quests->LoadQuestProgressFromSave({}, 0, 0);
	TestTrue(TEXT("Unknown quest keeps field NPC hidden"), FieldNpc->IsHidden());
	TestTrue(TEXT("Unknown quest keeps base resident hidden"), BaseNpc->IsHidden());
	FieldNpc->Destroy();
	BaseNpc->Destroy();

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
