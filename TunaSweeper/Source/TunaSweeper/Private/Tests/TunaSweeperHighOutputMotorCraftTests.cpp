#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperMotorCraftTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperHighOutputMotorCraftTest,
	"TunaSweeper.Inventory.Crafting.HighOutputMotor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperHighOutputMotorCraftTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperMotorCraftTests;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	// Initialize subsystem fixtures without loading or writing the user's save slots.
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
	Game->SetActiveWorkbench(1, ETunaSweeperWorkbenchMode::Craft);
	TArray<FTunaSweeperWorkbenchRecipeView> Recipes;
	Game->GetActiveWorkbenchRecipes(Recipes);
	TestFalse(TEXT("Recipe starts locked"), Recipes.ContainsByPredicate([](const auto& Recipe)
	{ return Recipe.RecipeId == FName(TEXT("recipe.high_output_motor")); }));
	TestTrue(TEXT("Recipe unlock succeeds"), Game->UnlockWorkbenchRecipe(TEXT("recipe.high_output_motor"), false));
	Game->GetActiveWorkbenchRecipes(Recipes);
	const int32 RecipeIndex = Recipes.IndexOfByPredicate([](const FTunaSweeperWorkbenchRecipeView& Recipe)
	{
		return Recipe.RecipeId == FName(TEXT("recipe.high_output_motor"));
	});
	if (!TestTrue(TEXT("Recipe is available after unlock"), RecipeIndex != INDEX_NONE)) return false;
	TestEqual(TEXT("One motor is produced"), Recipes[RecipeIndex].OutputQuantity, 1);
	TestEqual(TEXT("Motor item ID"), Recipes[RecipeIndex].OutputItemId, 6034);

	auto ResetItems = [&]
	{
		Game->ItemInstancesByUid.Reset();
		Game->ResetPlayerSlotArrays();
		Game->StorageSlots.Reset();
		Game->StorageSlots.SetNum(100);
	};
	auto PlaceItem = [&](TArray<FTunaSweeperInventorySlot>& Slots, int32 SlotIndex, int32 ItemId)
	{
		Slots[SlotIndex].ItemUid = Game->CreateItemInstance(ItemId, 1);
		return Slots[SlotIndex].ItemUid;
	};
	ResetItems();
	PlaceItem(Game->PlayerInventorySlots, 0, 6026);
	TestFalse(TEXT("Missing motor prevents crafting"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));
	TestEqual(TEXT("Missing ingredient leaves electromagnet untouched"), Game->CountInventoryItemById(6026), 1);
	TestEqual(TEXT("Failed craft produces no motor"), Game->CountInventoryItemById(6034), 0);

	PlaceItem(Game->PlayerInventorySlots, 1, 6024);
	TestTrue(TEXT("Electromagnet and motor craft successfully"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));
	TestEqual(TEXT("Electromagnet consumed"), Game->CountInventoryItemById(6026), 0);
	TestEqual(TEXT("Motor consumed"), Game->CountInventoryItemById(6024), 0);
	TestEqual(TEXT("One motor received"), Game->CountInventoryItemById(6034), 1);
	TestFalse(TEXT("Repeated craft requires new materials"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));

	ResetItems();
	// Put ingredients in accessible storage, so consumption cannot free an inventory slot.
	const FGuid MagnetUid = PlaceItem(Game->StorageSlots, 0, 6026);
	const FGuid MotorUid = PlaceItem(Game->StorageSlots, 1, 6024);
	for (int32 SlotIndex = 0; SlotIndex < Game->PlayerInventorySlots.Num(); ++SlotIndex)
	{
		PlaceItem(Game->PlayerInventorySlots, SlotIndex, 6020);
	}
	const int32 BeforeInstanceCount = Game->ItemInstancesByUid.Num();
	TestFalse(TEXT("Full inventory rejects output"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));
	TestEqual(TEXT("Failed output restores original item instances"), Game->ItemInstancesByUid.Num(), BeforeInstanceCount);
	TestEqual(TEXT("Electromagnet storage identity restored"), Game->StorageSlots[0].ItemUid, MagnetUid);
	TestEqual(TEXT("Motor storage identity restored"), Game->StorageSlots[1].ItemUid, MotorUid);
	TestEqual(TEXT("Electromagnet remains available"), Game->CountWorkbenchIngredientItemById(6026), 1);
	TestEqual(TEXT("Motor remains available"), Game->CountWorkbenchIngredientItemById(6024), 1);
	TestEqual(TEXT("Failed output leaves no motor"), Game->CountInventoryItemById(6034), 0);
	return true;
}

#endif
