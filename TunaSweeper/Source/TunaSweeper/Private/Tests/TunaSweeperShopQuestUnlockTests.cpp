#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/UnrealType.h"
#include "Components/TileView.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/ItemContainerWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"

namespace TunaSweeperShopQuestConditions
{
	bool Parse(const FJsonObject& Json, TArray<int32>& OutIds);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperShopQuestUnlockContractTest,
	"TunaSweeper.Shop.QuestUnlock.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperShopQuestUnlockContractTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Shop products can specify completed quest requirements"),
		FindFProperty<FArrayProperty>(FTunaSweeperShopItemDefinition::StaticStruct(), TEXT("RequiredCompletedQuestIds")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperShopQuestUnlockDataTest,
	"TunaSweeper.Shop.QuestUnlock.JsonValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperShopQuestUnlockDataTest::RunTest(const FString& Parameters)
{
	auto Parse = [](const FString& Text, TArray<int32>& Out)
	{
		TSharedPtr<FJsonObject> Json;
		return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) &&
			TunaSweeperShopQuestConditions::Parse(*Json, Out);
	};
	TArray<int32> Ids = {99};
	TestTrue(TEXT("Legacy product without requirements loads"), Parse(TEXT("{}"), Ids));
	TestTrue(TEXT("Omitted requirements reset previous output"), Ids.IsEmpty());
	TestTrue(TEXT("Explicit empty requirements remain unrestricted"), Parse(TEXT("{\"required_completed_quest_ids\":[]}"), Ids));
	TestTrue(TEXT("Positive int32 IDs load"), Parse(TEXT("{\"required_completed_quest_ids\":[1,2147483647,1]}"), Ids));
	TestEqual(TEXT("Duplicate requirements deduplicated"), Ids.Num(), 2);
	for (const FString Invalid : {TEXT("null"), TEXT("true"), TEXT("1"), TEXT("\"1\""), TEXT("{}"),
		TEXT("[0]"), TEXT("[-1]"), TEXT("[1.5]"), TEXT("[2147483648]"), TEXT("[\"1\"]"), TEXT("[true]"), TEXT("[1,null]")})
	{
		Ids = {99};
		TestFalse(*FString::Printf(TEXT("Reject malformed requirements %s"), *Invalid),
			Parse(TEXT("{\"required_completed_quest_ids\":") + Invalid + TEXT("}"), Ids));
		TestTrue(TEXT("Invalid parse never leaks partial or previous requirements"), Ids.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperShopQuestUnlockTest,
	"TunaSweeper.Shop.QuestUnlock.PurchaseUiAndPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperShopQuestUnlockTest::RunTest(const FString& Parameters)
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
	auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
	FContextAccess::Attach(Game, &Context); Context.OwningGameInstance = Game; World->SetGameInstance(Game);
	Game->bInventoryStateInitialized = true; Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->UGameInstance::Init(); Game->ResetPlayerSlotArrays();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game, nullptr); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
	};
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition Item;
	if (!TestTrue(TEXT("Fixture item exists"), Items->TryGetItemDefinition(3004, Item))) return false;
	Quests->QuestDefinitions.Reset(); Quests->QuestProgressById.Reset(); Quests->bQuestDataLoaded = true;
	FTunaSweeperQuestDefinition First; First.QuestId = 920001;
	FTunaSweeperObjectiveDefinition Objective; Objective.ObjectiveId = TEXT("unlock");
	Objective.Type = ETunaSweeperObjectiveType::InteractionCompleted; Objective.InteractionEventId = TEXT("test.shop.unlock");
	First.Objectives.Add(Objective); Quests->QuestDefinitions.Add(First.QuestId, First);
	FTunaSweeperQuestDefinition Second; Second.QuestId = 920002; Second.bCompleteOnAccept = true;
	Quests->QuestDefinitions.Add(Second.QuestId, Second);
	Quests->AddCoins(100, false);
	FTunaSweeperShopItemDefinition Locked; Locked.ItemId = 3004; Locked.StockQuantity = 4; Locked.PriceOverride = 10;
	Locked.RequiredCompletedQuestIds = {First.QuestId};
	FTunaSweeperShopItemDefinition Normal = Locked; Normal.RequiredCompletedQuestIds.Reset();
	FTunaSweeperShopItemDefinition Both = Locked; Both.RequiredCompletedQuestIds.Add(Second.QuestId);
	FTunaSweeperShopDefinition Shop; Shop.ShopId = 990101; Shop.Items = {Locked, Normal, Both};
	Items->ShopDefinitionsById.Add(Shop.ShopId, Shop); Game->SetActiveShop(Shop.ShopId);
	Game->SetShopStockQuantity(Shop.ShopId, 1, Normal, 2);
	TArray<FTunaSweeperShopItemView> Views;
	TestTrue(TEXT("Unrestricted legacy product remains visible"), Game->GetActiveShopItems(Views));
	TestEqual(TEXT("Only one unlocked row"), Views.Num(), 1);
	if (Views.Num() != 1) return false;
	TestEqual(TEXT("Filtering keeps original slot identity"), Views[0].SlotIndex, 1);
	TestEqual(TEXT("Filtering preserves saved stock at original index"), Views[0].StockQuantity, 2);
	FTunaSweeperShopItemView View;
	TestFalse(TEXT("Locked product detail cannot be fetched"), Game->TryGetActiveShopItemView(0, View));
	TestFalse(TEXT("Direct slot purchase cannot bypass quest lock"), Game->TryBuyActiveShopSlot(0));
	TestEqual(TEXT("Rejected purchase preserves coins"), Quests->GetCoinBalance(), 100);
	TestEqual(TEXT("Rejected purchase gives no item"), Game->CountInventoryItemById(3004), 0);
	TestEqual(TEXT("Rejected purchase preserves stock"), Game->GetShopStockQuantity(Shop.ShopId, 0, Locked), 4);

	auto* Panel = CreateWidget<UShopContainerWidget>(Game);
	if (!TestNotNull(TEXT("Shop UI fixture"), Panel)) return false;
	Panel->ContainerTileView = NewObject<UTileView>(Panel);
	Panel->SetShopView(Shop.ShopId); Panel->UItemContainerWidget::NativeConstruct();
	ON_SCOPE_EXIT { Panel->UItemContainerWidget::NativeDestruct(); };
	auto VisibleTiles = [Panel]()
	{
		int32 Count = 0;
		for (UObject* Object : Panel->ContainerTileView->GetListItems())
			if (const auto* Tile = Cast<UTunaSweeperItemStackTileItemObject>(Object); Tile && !Tile->GetTileData().bIsEmpty) ++Count;
		return Count;
	};
	TestEqual(TEXT("Open UI contains only available product"), VisibleTiles(), 1);
	Quests->AcceptQuest(First.QuestId);
	TestFalse(TEXT("Accepted quest does not unlock product"), Game->TryBuyActiveShopSlot(0));
	Quests->NotifyInteractionCompleted(Objective.InteractionEventId, TEXT("test"));
	TestFalse(TEXT("Unclaimed reward does not unlock product"), Game->TryBuyActiveShopSlot(0));
	Quests->ClaimQuestReward(First.QuestId);
	TestEqual(TEXT("Open UI updates immediately on quest reward"), VisibleTiles(), 2);
	TestFalse(TEXT("All required quests are needed"), Game->TryBuyActiveShopSlot(2));
	TestTrue(TEXT("Unlocked product can be purchased"), Game->TryBuyActiveShopSlot(0));
	TestEqual(TEXT("Successful purchase grants one item"), Game->CountInventoryItemById(3004), 1);
	TestEqual(TEXT("Successful purchase spends exact price"), Quests->GetCoinBalance(), 90);
	TestEqual(TEXT("Successful purchase decrements only its original stock"), Game->GetShopStockQuantity(Shop.ShopId, 0, Locked), 3);
	TestEqual(TEXT("Other stock unchanged"), Game->GetShopStockQuantity(Shop.ShopId, 1, Normal), 2);
	Quests->AcceptQuest(Second.QuestId); Quests->ClaimQuestReward(Second.QuestId);
	TestEqual(TEXT("Both requirements unlock final product in open UI"), VisibleTiles(), 3);
	for (int32 InvalidId : {0, -1, 929999})
	{
		Items->ShopDefinitionsById[Shop.ShopId].Items[2].RequiredCompletedQuestIds = {InvalidId};
		TestFalse(TEXT("Unknown or invalid required quest fails closed"), Game->TryGetActiveShopItemView(2, View));
	}
	Items->ShopDefinitionsById[Shop.ShopId].Items[2] = Both;

	auto* Save = NewObject<UTunaSweeperSaveGame>();
	Quests->ExportQuestProgressForSave(Save->NumericQuestProgressStates, Save->NumericTrackedQuestId, Save->QuestCoinBalance);
	Game->ShopStockStatesByKey.GenerateValueArray(Save->ShopStockStates);
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Existing save schema serializes quests and stock"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
	auto* Loaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Save roundtrip"), Loaded)) return false;
	Quests->ResetQuestProgressForNewGame();
	TestEqual(TEXT("New game immediately hides gated UI rows"), VisibleTiles(), 1);
	TestFalse(TEXT("Cached slot cannot buy after progress reset"), Game->TryBuyActiveShopSlot(0));
	Game->ShopStockStatesByKey.Reset();
	for (const auto& Stock : Loaded->ShopStockStates)
	{
		FTunaSweeperShopItemDefinition Definition;
		if (Items->TryGetShopItemDefinition(Stock.ShopId, Stock.SlotIndex, Definition))
			Game->SetShopStockQuantity(Stock.ShopId, Stock.SlotIndex, Definition, Stock.StockQuantity);
	}
	Quests->LoadQuestProgressFromSave(Loaded->NumericQuestProgressStates, Loaded->NumericTrackedQuestId, Loaded->QuestCoinBalance);
	TestEqual(TEXT("Save load immediately restores visible products"), VisibleTiles(), 3);
	TestTrue(TEXT("Previously unlocked slot can be fetched after load"), Game->TryGetActiveShopItemView(0, View));
	TestEqual(TEXT("Unlock refresh never restocks purchased product"), View.StockQuantity, 3);
	Game->ClearActiveShop(); Game->SetActiveShop(Shop.ShopId);
	Game->TryGetActiveShopItemView(0, View); TestEqual(TEXT("Reopening retains stock"), View.StockQuantity, 3);
	Panel->UItemContainerWidget::NativeDestruct();
	Quests->ResetQuestProgressForNewGame();
	TestEqual(TEXT("Closed UI unsubscribes from quest events"), VisibleTiles(), 3);
	return true;
}
#endif
