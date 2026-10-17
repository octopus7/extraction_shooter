#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperQuestData
{
	bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

namespace TunaSweeperQuestCraftTests
{
	bool Parse(const FString& Fields, FTunaSweeperObjectiveDefinition& Out)
	{
		TSharedPtr<FJsonObject> Json;
		return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(
			TEXT("{\"objective_id\":\"craft\",\"type\":\"workbench_recipe_crafted\"") + Fields + TEXT("}")), Json)
			&& TunaSweeperQuestData::ParseObjective(Json, Out);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestCraftDataTest,
	"TunaSweeper.Quest.Crafting.JsonValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestCraftDataTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperQuestCraftTests;
	FTunaSweeperObjectiveDefinition Objective;
	TestTrue(TEXT("Exact recipe objective loads"), Parse(TEXT(",\"recipe_id\":\"recipe.test.craft\",\"required_count\":2"), Objective));
	TestTrue(TEXT("Omitted count defaults to one"), Parse(TEXT(",\"recipe_id\":\"recipe.test.craft\""), Objective));
	TestEqual(TEXT("Default count"), Objective.RequiredCount, 1);
	TestFalse(TEXT("Missing recipe cannot match every craft"), Parse(TEXT(""), Objective));
	for (const FString Invalid : { TEXT("null"), TEXT("true"), TEXT("42"), TEXT("[]"), TEXT("\"\""), TEXT("\"  \""), TEXT("\"None\"") })
	{
		TestFalse(TEXT("Invalid recipe rejected"), Parse(TEXT(",\"recipe_id\":") + Invalid, Objective));
	}
	for (const FString Invalid : { TEXT("null"), TEXT("true"), TEXT("\"2\""), TEXT("0"), TEXT("-1"), TEXT("1.5"), TEXT("2147483648") })
	{
		TestFalse(TEXT("Invalid craft count rejected"), Parse(TEXT(",\"recipe_id\":\"recipe.test.craft\",\"required_count\":") + Invalid, Objective));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestCraftTest,
	"TunaSweeper.Quest.Crafting.SuccessfulTransactionOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestCraftTest::RunTest(const FString& Parameters)
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	// Prevent fixture setup and quest notifications from reading/writing player saves.
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
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	Quests->QuestDefinitions.Reset();
	Quests->QuestProgressById.Reset();
	Quests->bQuestDataLoaded = true;
	Items->bItemDataLoaded = true;
	constexpr int32 IngredientId = 990701, OutputId = 990702, QuestId = 990703, LateQuestId = 990704;
	for (int32 Id : { IngredientId, OutputId })
	{
		FTunaSweeperItemDefinition Item;
		Item.Id = Id;
		Items->ItemDefinitionsById.Add(Id, Item);
	}
	FTunaSweeperWorkbenchIngredient Ingredient;
	Ingredient.ItemId = IngredientId;
	FTunaSweeperWorkbenchRecipeDefinition Recipe;
	Recipe.RecipeId = TEXT("recipe.test.craft");
	Recipe.WorkbenchId = 990701;
	Recipe.OutputItemId = OutputId;
	Recipe.OutputQuantity = 2;
	Recipe.Ingredients.Add(Ingredient);
	Items->WorkbenchRecipeDefinitionsById.Add(Recipe.RecipeId, Recipe);
	Items->WorkbenchRecipeIdsInLoadOrder = { Recipe.RecipeId };
	FTunaSweeperWorkbenchRecipeDefinition OtherRecipe = Recipe;
	OtherRecipe.RecipeId = TEXT("recipe.test.other");
	Items->WorkbenchRecipeDefinitionsById.Add(OtherRecipe.RecipeId, OtherRecipe);
	Items->WorkbenchRecipeIdsInLoadOrder.Add(OtherRecipe.RecipeId);
	FTunaSweeperObjectiveDefinition Objective;
	if (!TestTrue(TEXT("Parse craft objective"), TunaSweeperQuestCraftTests::Parse(
		TEXT(",\"recipe_id\":\"recipe.test.craft\",\"required_count\":2"), Objective))) return false;
	for (int32 Id : { QuestId, LateQuestId })
	{
		FTunaSweeperQuestDefinition Definition;
		Definition.QuestId = Id;
		Definition.Objectives = { Objective };
		Quests->QuestDefinitions.Add(Id, Definition);
	}
	TestTrue(TEXT("Accept craft quest"), Quests->AcceptQuest(QuestId));
	Game->ResetPlayerSlotArrays();
	Game->SetActiveWorkbench(Recipe.WorkbenchId, ETunaSweeperWorkbenchMode::Craft);
	auto Count = [&]() { return Quests->GetObjectiveProgressCount(QuestId, TEXT("craft")); };
	auto ResetItems = [&]()
	{
		Game->ItemInstancesByUid.Reset();
		Game->ResetPlayerSlotArrays();
		Game->StorageSlots.SetNum(1);
		Game->StorageSlots[0].ItemUid.Invalidate();
	};
	auto AddIngredient = [&]()
	{ Game->PlayerInventorySlots[0].ItemUid = Game->CreateItemInstance(IngredientId, 1); };
	Game->PlayerInventorySlots[0].ItemUid = Game->CreateItemInstance(OutputId, 1);
	Game->BroadcastInventoryStateChanged();
	Quests->NotifyItemAcquired(OutputId, 1, false);
	TestEqual(TEXT("Possession/acquisition never credits crafting"), Count(), 0);
	ResetItems();
	TestFalse(TEXT("Missing ingredients reject craft"), Game->TryCraftActiveWorkbenchRecipe(0, false));
	TestEqual(TEXT("Rejected craft gives no credit"), Count(), 0);
	AddIngredient();
	TestTrue(TEXT("Other recipe with same output succeeds"), Game->TryCraftActiveWorkbenchRecipe(1, false));
	TestEqual(TEXT("Other recipe gives no credit"), Count(), 0);
	ResetItems();
	const FGuid StoredIngredient = Game->CreateItemInstance(IngredientId, 1);
	Game->StorageSlots[0].ItemUid = StoredIngredient;
	for (auto& Slot : Game->PlayerInventorySlots) Slot.ItemUid = Game->CreateItemInstance(IngredientId, 1);
	// Inventory ingredients are consumed first: require two output slots so consuming one cannot make room.
	TestFalse(TEXT("Insufficient output space rolls back craft"), Game->TryCraftActiveWorkbenchRecipe(0, false));
	TestEqual(TEXT("Output failure gives no credit"), Count(), 0);
	TestEqual(TEXT("Rollback retains storage ingredient"), Game->StorageSlots[0].ItemUid, StoredIngredient);
	ResetItems();
	AddIngredient();
	TestTrue(TEXT("Successful transaction crafts recipe"), Game->TryCraftActiveWorkbenchRecipe(0, false));
	TestEqual(TEXT("Two output items count as one crafting operation"), Count(), 1);
	TestEqual(TEXT("Output was actually received"), Game->CountInventoryItemById(OutputId), 2);
	TestTrue(TEXT("Accept late quest"), Quests->AcceptQuest(LateQuestId));
	TestEqual(TEXT("Earlier crafting is not retroactive"), Quests->GetObjectiveProgressCount(LateQuestId, TEXT("craft")), 0);
	TArray<FTunaSweeperNumericQuestProgressSaveData> Saved;
	int32 Tracked = 0, Coins = 0;
	Quests->ExportQuestProgressForSave(Saved, Tracked, Coins);
	Quests->QuestProgressById.Reset();
	Quests->LoadQuestProgressFromSave(Saved, Tracked, Coins);
	TestEqual(TEXT("Existing objective save round trip preserves craft count"), Count(), 1);
	ResetItems();
	AddIngredient();
	TestTrue(TEXT("Second successful transaction"), Game->TryCraftActiveWorkbenchRecipe(0, false));
	TestEqual(TEXT("Required crafts complete quest"), Quests->GetQuestState(QuestId), ETunaSweeperQuestState::RewardAvailable);
	TestEqual(TEXT("Only post-accept craft advances late quest"), Quests->GetObjectiveProgressCount(LateQuestId, TEXT("craft")), 1);
	return true;
}

#endif
