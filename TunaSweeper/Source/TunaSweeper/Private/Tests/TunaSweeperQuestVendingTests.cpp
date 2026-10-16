#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperQuestVendingActor.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestVendingTest,
	"TunaSweeper.Quest.Vending.ProgressionAndPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestVendingTest::RunTest(const FString& Parameters)
{
	// Catch premature relocation, repeated item consumption/rewards, missing provider routing,
	// and missing progress subscriptions by driving the real subsystem and actor lifecycle.
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
	};
	// GetMapName uses the outermost package, not the UWorld object name.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("VendingTestBunkerMap"),
		CreatePackage(TEXT("/Temp/VendingTestBunkerMap")));
	TestTrue(TEXT("Fixture enables the base-map shop interaction gate"), World->GetMapName().EndsWith(TEXT("BunkerMap")));
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
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
	auto* Interactions = World->GetSubsystem<UTunaSweeperInteractionSubsystem>();
	if (!TestNotNull(TEXT("Quest subsystem"), Quests) || !TestNotNull(TEXT("Interaction subsystem"), Interactions)) return false;
	Quests->LoadQuestData(false);
	Quests->QuestDefinitions.Reset();
	Quests->QuestProgressById.Reset();
	FTunaSweeperQuestDefinition First;
	First.QuestId = 923001;
	First.ProviderId = TEXT("provider.vending_test");
	First.Rewards.Coins = 7;
	FTunaSweeperObjectiveDefinition Coin;
	Coin.ObjectiveId = TEXT("submit_coin");
	Coin.Type = ETunaSweeperObjectiveType::ItemSubmitted;
	Coin.ItemId = 6002;
	Coin.RequiredCount = 1;
	Coin.TargetProviderId = First.ProviderId;
	First.Objectives.Add(Coin);
	Quests->QuestDefinitions.Add(First.QuestId, First);
	FTunaSweeperQuestDefinition Relocation;
	Relocation.QuestId = 923002;
	Relocation.ProviderId = First.ProviderId;
	Relocation.RequiredCompletedQuestIds = {First.QuestId};
	FTunaSweeperObjectiveDefinition Module;
	Module.ObjectiveId = TEXT("show_module");
	Module.Type = ETunaSweeperObjectiveType::ItemPossessed;
	Module.ItemId = 3004;
	Module.RequiredCount = 1;
	Module.TargetProviderId = First.ProviderId;
	Relocation.Objectives.Add(Module);
	FTunaSweeperItemStack ReturnedModule;
	ReturnedModule.ItemId = Module.ItemId;
	ReturnedModule.Quantity = 1;
	Relocation.Rewards.Items.Add(ReturnedModule);
	Quests->QuestDefinitions.Add(Relocation.QuestId, Relocation);
	Quests->LoadQuestProgressFromSave({}, 0, 0);

	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Field = World->SpawnActor<ATunaSweeperQuestVendingActor>(Spawn);
	auto* Base = World->SpawnActor<ATunaSweeperQuestVendingActor>(Spawn);
	if (!TestNotNull(TEXT("Field vending actor"), Field) || !TestNotNull(TEXT("Base vending actor"), Base)) return false;
	for (auto* Actor : {Field, Base})
	{
		Actor->FirstMeetingQuestId = First.QuestId;
		Actor->RelocationQuestId = Relocation.QuestId;
		Actor->QuestProviderId = First.ProviderId;
	}
	Base->PresenceRole = ETunaSweeperQuestVendingRole::BaseResident;
	Field->DispatchBeginPlay();
	Base->DispatchBeginPlay();
	TestFalse(TEXT("First meeting exposes field vending"), Field->IsHidden());
	TestTrue(TEXT("Base vending waits for relocation"), Base->IsHidden());
	TestFalse(TEXT("Field vending never exposes a shop even on base map"), Interactions->CanOfferInteraction(Field->GetInteractableComponent()));
	TestFalse(TEXT("Unrelocated base shop unavailable"), Base->IsShopAvailable());
	TestTrue(TEXT("Vending routes first meeting to normal quest interaction"), Interactions->CanOfferInteraction(Field->GetQuestInteractableComponent()));
	TestEqual(TEXT("Provider resolves configurable first quest"), Field->ResolveQuestId(), First.QuestId);
	TestFalse(TEXT("Relocation requires claimed first reward"), Quests->CanAcceptQuest(Relocation.QuestId));
	TestTrue(TEXT("First meeting accepts normally"), Quests->AcceptQuest(First.QuestId));
	TestFalse(TEXT("Accepted first meeting keeps vending reachable"), Field->IsHidden());
	int32 Submitted = 0;
	TestFalse(TEXT("Missing coin cannot complete first meeting"), Quests->TrySubmitItemsToProvider(First.ProviderId, Submitted, false));
	TestTrue(TEXT("Fixture coin enters inventory"), Game->AddItemToFirstAvailableInventorySlot(Coin.ItemId, 2));
	TestTrue(TEXT("Explicit submission consumes required coin"), Quests->TrySubmitItemsToProvider(First.ProviderId, Submitted, false));
	TestEqual(TEXT("Exactly one coin consumed"), Game->CountInventoryItemById(Coin.ItemId), 1);
	TestEqual(TEXT("Submission awaits explicit claim"), Quests->GetQuestState(First.QuestId), ETunaSweeperQuestState::RewardAvailable);
	TestEqual(TEXT("Submission does not grant reward"), Quests->GetCoinBalance(), 0);
	TestTrue(TEXT("Retry can reopen pending reward"), Quests->TrySubmitItemsToProvider(First.ProviderId, Submitted, false));
	TestEqual(TEXT("Retry does not consume another coin"), Game->CountInventoryItemById(Coin.ItemId), 1);
	TestTrue(TEXT("Explicit claim completes first meeting"), Quests->ClaimQuestReward(First.QuestId));
	TestFalse(TEXT("First reward cannot be claimed twice"), Quests->ClaimQuestReward(First.QuestId));
	TestEqual(TEXT("First reward granted once"), Quests->GetCoinBalance(), 7);
	TestFalse(TEXT("First meeting completion retains field machine"), Field->IsHidden());
	TestEqual(TEXT("Next provider quest becomes relocation"), Field->ResolveQuestId(), Relocation.QuestId);
	TestTrue(TEXT("Relocation accepts without the module"), Quests->AcceptQuest(Relocation.QuestId));
	TestEqual(TEXT("Missing module leaves relocation accepted"), Quests->GetQuestState(Relocation.QuestId), ETunaSweeperQuestState::Accepted);
	TestFalse(TEXT("Accepting relocation retains field machine"), Field->IsHidden());
	TestTrue(TEXT("Accepted relocation still offers field interaction"), Interactions->CanOfferInteraction(Field->GetQuestInteractableComponent()));
	TestFalse(TEXT("Missing module prevents relocation claim"), Quests->ClaimQuestReward(Relocation.QuestId));
	TestTrue(TEXT("Pending relocation keeps base machine hidden"), Base->IsHidden());
	TestFalse(TEXT("Pending relocation cannot unlock shop"), Base->IsShopAvailable());
	TArray<FTunaSweeperNumericQuestProgressSaveData> PendingProgress;
	int32 PendingTrackedQuest = 0;
	int32 PendingCoins = 0;
	Quests->ExportQuestProgressForSave(PendingProgress, PendingTrackedQuest, PendingCoins);
	Quests->LoadQuestProgressFromSave(PendingProgress, PendingTrackedQuest, PendingCoins);
	TestFalse(TEXT("Accepted relocation save retains field machine"), Field->IsHidden());
	TestTrue(TEXT("Accepted relocation save keeps base machine absent"), Base->IsHidden());
	TestTrue(TEXT("Fixture module enters inventory"), Game->AddItemToFirstAvailableInventorySlot(Module.ItemId, 1));
	TestEqual(TEXT("Module possession makes relocation ready"), Quests->GetQuestState(Relocation.QuestId), ETunaSweeperQuestState::RewardAvailable);
	TestFalse(TEXT("Possession readiness retains field reward access"), Field->IsHidden());
	TestTrue(TEXT("Vending identifies ready relocation"), Quests->TrySubmitItemsToProvider(First.ProviderId, Submitted, false));
	TestEqual(TEXT("Vending identifies correct relocation"), Submitted, Relocation.QuestId);
	TestEqual(TEXT("Looking up ready relocation does not consume module"), Game->CountInventoryItemById(Module.ItemId), 1);
	int32 HandoffQuest = 0;
	TestTrue(TEXT("Vending resolves its relocation reward"), Quests->TryResolveQuestForProvider(First.ProviderId, 0, HandoffQuest));
	TestEqual(TEXT("Vending receives the relocation completion"), HandoffQuest, Relocation.QuestId);
	TestFalse(TEXT("Mole cannot route vending relocation reward"), Quests->TryResolveQuestForProvider(TEXT("provider.mole"), Relocation.QuestId, HandoffQuest));
	TestEqual(TEXT("Fixture module can be spent before claiming"), Game->ConsumeInventoryItemById(Module.ItemId, 1), 1);
	TestEqual(TEXT("Losing module revokes relocation readiness"), Quests->GetQuestState(Relocation.QuestId), ETunaSweeperQuestState::Accepted);
	TestFalse(TEXT("Losing module retains field machine"), Field->IsHidden());
	TestFalse(TEXT("Lost module blocks explicit claim"), Quests->ClaimQuestReward(Relocation.QuestId));
	TestTrue(TEXT("Reacquiring module restores readiness"), Game->AddItemToFirstAvailableInventorySlot(Module.ItemId, 1));
	Quests->ExportQuestProgressForSave(PendingProgress, PendingTrackedQuest, PendingCoins);
	Quests->LoadQuestProgressFromSave(PendingProgress, PendingTrackedQuest, PendingCoins);
	TestFalse(TEXT("Loading unclaimed relocation retains field machine"), Field->IsHidden());
	TestEqual(TEXT("Loading unclaimed relocation routes field reward"), Field->ResolveQuestId(), Relocation.QuestId);
	TestTrue(TEXT("Loading unclaimed relocation keeps base hidden"), Base->IsHidden());
	TestFalse(TEXT("Loading unclaimed relocation keeps shop locked"), Base->IsShopAvailable());
	TestTrue(TEXT("Explicit relocation claim succeeds"), Quests->ClaimQuestReward(Relocation.QuestId));
	TestEqual(TEXT("Claim returns exactly one module as reward"), Game->CountInventoryItemById(Module.ItemId), 1);
	TestFalse(TEXT("Relocation cannot be claimed twice"), Quests->ClaimQuestReward(Relocation.QuestId));
	TestEqual(TEXT("Repeated claim cannot duplicate returned module"), Game->CountInventoryItemById(Module.ItemId), 1);
	TestFalse(TEXT("Claim retains field machine for the current visit"), Field->IsHidden());
	TestTrue(TEXT("Remaining field machine keeps collision"), Field->GetActorEnableCollision());
	TestFalse(TEXT("Claim immediately reveals base machine"), Base->IsHidden());
	TestTrue(TEXT("Base machine has collision"), Base->GetActorEnableCollision());
	TestTrue(TEXT("Base shop unlocks after claim"), Base->IsShopAvailable());
	TestTrue(TEXT("Normal interaction subsystem offers unlocked shop"), Interactions->CanOfferInteraction(Base->GetInteractableComponent()));
	TestTrue(TEXT("Remaining field machine offers farewell interaction"), Interactions->CanOfferInteraction(Field->GetQuestInteractableComponent()));
	TestTrue(TEXT("Remaining field machine enters departure dialogue mode"), Field->IsAwaitingDeparture());
	TestFalse(TEXT("Resident never enters departure dialogue mode"), Base->IsAwaitingDeparture());
	FTunaSweeperQuestTextString Farewell;
	Farewell.StringKey = Field->DepartureStringKey;
	Farewell.Korean = FText::FromString(TEXT("Test farewell KO"));
	Farewell.English = FText::FromString(TEXT("Test farewell EN"));
	Farewell.Japanese = FText::FromString(TEXT("Test farewell JA"));
	Quests->QuestTextStringsByKey.Add(Farewell.StringKey, Farewell);
	Game->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::Korean, false);
	TestEqual(TEXT("Departure dialogue resolves current language"), Field->GetDepartureText().ToString(), Farewell.Korean.ToString());
	Game->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::English, false);
	TestEqual(TEXT("Departure dialogue follows language changes"), Field->GetDepartureText().ToString(), Farewell.English.ToString());
	TestEqual(TEXT("Departure uses localized talk prompt"), Field->GetQuestInteractableComponent()->GetInteractionDisplayName().ToString(),
		Game->ResolveLocalizedText(TEXT("ui.interaction.mole_dialogue"), FText::GetEmpty()).ToString());

	// Future quests remain usable beside the base shop.
	FTunaSweeperQuestDefinition FollowUp = Relocation;
	FollowUp.QuestId = 923003;
	FollowUp.CompletionProviderId = NAME_None;
	FollowUp.RequiredCompletedQuestIds = {Relocation.QuestId};
	Quests->QuestDefinitions.Add(FollowUp.QuestId, FollowUp);
	Quests->OnQuestProgressChanged.Broadcast();
	TestTrue(TEXT("Resident exposes subsequent quest interaction"), Interactions->CanOfferInteraction(Base->GetQuestInteractableComponent()));
	TestEqual(TEXT("Resident routes next provider quest"), Base->ResolveQuestId(), FollowUp.QuestId);
	TestEqual(TEXT("Departing field machine cannot route subsequent quests"), Field->ResolveQuestId(), 0);
	TestTrue(TEXT("Resident retains shop beside quest interaction"), Interactions->CanOfferInteraction(Base->GetInteractableComponent()));

	TStrongObjectPtr<UTunaSweeperSaveGame> Save(NewObject<UTunaSweeperSaveGame>());
	Quests->ExportQuestProgressForSave(Save->NumericQuestProgressStates, Save->NumericTrackedQuestId, Save->QuestCoinBalance);
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Vending progress uses existing save schema"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bytes))) return false;
	TStrongObjectPtr<UTunaSweeperSaveGame> Loaded(Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));
	if (!TestNotNull(TEXT("Vending save roundtrip"), Loaded.Get())) return false;
	Quests->LoadQuestProgressFromSave({}, 0, 0);
	TestFalse(TEXT("Loading fresh progress restores field machine"), Field->IsHidden());
	TestTrue(TEXT("Loading fresh progress removes resident"), Base->IsHidden());
	TestFalse(TEXT("Loading fresh progress removes shop availability"), Base->IsShopAvailable());
	Quests->LoadQuestProgressFromSave(Loaded->NumericQuestProgressStates, Loaded->NumericTrackedQuestId, Loaded->QuestCoinBalance);
	TestFalse(TEXT("Progress refresh does not end the current field visit"), Field->IsHidden());
	auto* RevisitedField = World->SpawnActor<ATunaSweeperQuestVendingActor>(Spawn);
	if (!TestNotNull(TEXT("New visit recreates field actor"), RevisitedField)) return false;
	RevisitedField->FirstMeetingQuestId = First.QuestId;
	RevisitedField->RelocationQuestId = Relocation.QuestId;
	RevisitedField->QuestProviderId = First.ProviderId;
	RevisitedField->DispatchBeginPlay();
	TestTrue(TEXT("Next visit hides relocated field machine"), RevisitedField->IsHidden());
	TestFalse(TEXT("Next visit removes field collision"), RevisitedField->GetActorEnableCollision());
	TestFalse(TEXT("Next visit cannot offer farewell or quests"), Interactions->CanOfferInteraction(RevisitedField->GetQuestInteractableComponent()));
	RevisitedField->Destroy();
	TestFalse(TEXT("Loading relocated save restores resident"), Base->IsHidden());
	TestTrue(TEXT("Loading relocated save restores shop"), Base->IsShopAvailable());
	TestEqual(TEXT("Load does not grant rewards again"), Quests->GetCoinBalance(), 7);
	TestEqual(TEXT("Load does not duplicate returned module"), Game->CountInventoryItemById(Module.ItemId), 1);
	Quests->QuestDefinitions.FindChecked(First.QuestId).RequiredCompletedQuestIds = {929999};
	Quests->LoadQuestProgressFromSave({}, 0, 0);
	TestTrue(TEXT("Locked first meeting hides field machine"), Field->IsHidden());
	TestFalse(TEXT("Locked first meeting does not offer quests"), Interactions->CanOfferInteraction(Field->GetQuestInteractableComponent()));
	Quests->QuestDefinitions.Remove(Relocation.QuestId);
	Quests->OnQuestProgressChanged.Broadcast();
	TestTrue(TEXT("Missing relocation definition hides field machine"), Field->IsHidden());
	TestTrue(TEXT("Missing relocation definition hides resident"), Base->IsHidden());
	TestFalse(TEXT("Missing definition cannot unlock shop"), Base->IsShopAvailable());
	Field->Destroy();
	Base->Destroy();
	return true;
}
#endif
