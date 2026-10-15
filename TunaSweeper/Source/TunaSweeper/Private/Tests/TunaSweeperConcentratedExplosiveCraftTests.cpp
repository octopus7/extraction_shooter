#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperExplosiveCraftTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperConcentratedExplosiveCraftTest,
	"TunaSweeper.Inventory.Crafting.ConcentratedExplosive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperConcentratedExplosiveCraftTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperExplosiveCraftTests;
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
	const int32 RecipeIndex = Recipes.IndexOfByPredicate([](const FTunaSweeperWorkbenchRecipeView& Recipe)
	{
		return Recipe.RecipeId == FName(TEXT("recipe.concentrated_explosive"));
	});
	if (!TestTrue(TEXT("Recipe is available without a blueprint"), RecipeIndex != INDEX_NONE)) return false;
	TestEqual(TEXT("One explosive is produced"), Recipes[RecipeIndex].OutputQuantity, 1);
	TestEqual(TEXT("Explosive item ID"), Recipes[RecipeIndex].OutputItemId, 6031);

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
	PlaceItem(Game->PlayerInventorySlots, 0, 6028);
	TestFalse(TEXT("Missing clock prevents crafting"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));
	TestEqual(TEXT("Missing ingredient leaves pink puff untouched"), Game->CountInventoryItemById(6028), 1);
	TestEqual(TEXT("Failed craft produces no explosive"), Game->CountInventoryItemById(6031), 0);

	PlaceItem(Game->PlayerInventorySlots, 1, 6032);
	TestTrue(TEXT("Pink puff and clock craft successfully"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));
	TestEqual(TEXT("Pink puff consumed"), Game->CountInventoryItemById(6028), 0);
	TestEqual(TEXT("Clock consumed"), Game->CountInventoryItemById(6032), 0);
	TestEqual(TEXT("One explosive received"), Game->CountInventoryItemById(6031), 1);
	TestFalse(TEXT("Repeated craft requires new materials"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));

	ResetItems();
	// Put ingredients in accessible storage, so consumption cannot free an inventory slot.
	const FGuid PuffUid = PlaceItem(Game->StorageSlots, 0, 6028);
	const FGuid ClockUid = PlaceItem(Game->StorageSlots, 1, 6032);
	for (int32 SlotIndex = 0; SlotIndex < Game->PlayerInventorySlots.Num(); ++SlotIndex)
	{
		PlaceItem(Game->PlayerInventorySlots, SlotIndex, 6020);
	}
	const int32 BeforeInstanceCount = Game->ItemInstancesByUid.Num();
	TestFalse(TEXT("Full inventory rejects output"), Game->TryCraftActiveWorkbenchRecipe(RecipeIndex, false));
	TestEqual(TEXT("Failed output restores original item instances"), Game->ItemInstancesByUid.Num(), BeforeInstanceCount);
	TestEqual(TEXT("Pink puff storage identity restored"), Game->StorageSlots[0].ItemUid, PuffUid);
	TestEqual(TEXT("Clock storage identity restored"), Game->StorageSlots[1].ItemUid, ClockUid);
	TestEqual(TEXT("Pink puff remains available"), Game->CountWorkbenchIngredientItemById(6028), 1);
	TestEqual(TEXT("Clock remains available"), Game->CountWorkbenchIngredientItemById(6032), 1);
	TestEqual(TEXT("Failed output leaves no explosive"), Game->CountInventoryItemById(6031), 0);
	return true;
}

#endif
