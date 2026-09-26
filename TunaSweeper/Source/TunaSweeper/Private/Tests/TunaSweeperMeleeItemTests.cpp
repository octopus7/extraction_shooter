#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMeleeItemDefinitionsTest,
	"TunaSweeper.Inventory.MeleeReplacement.Definitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperMeleeItemDefinitionsTest::RunTest(const FString& Parameters)
{
	UTunaSweeperGameInstance* Game = NewObject<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* Items = NewObject<UTunaSweeperItemDataSubsystem>(Game);
	if (!TestTrue(TEXT("Item data loads"), Items->LoadItemData())) return false;
	FTunaSweeperItemDefinition Definition;
	for (int32 ItemId : {1004, 1005, 6003})
	{
		if (!TestTrue(TEXT("Replacement exists"), Items->TryGetItemDefinition(ItemId, Definition))) return false;
		TestEqual(TEXT("Replacement is melee"), Definition.CategoryTag, FName(TEXT("item.category.weapon.melee")));
		TestEqual(TEXT("Replacement is equippable"), Definition.EquipmentSlotTag, FName(TEXT("equipment.slot.melee")));
	}
	TestTrue(TEXT("Spiked club keeps the legacy bat ID"), Items->TryGetItemDefinition(1005, Definition));
	TestEqual(TEXT("Spiked club has its own localized identity"), Definition.NameStringKey, FName(TEXT("item.spiked_club")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCarriedToolTest,
	"TunaSweeper.Inventory.MeleeReplacement.EquippedTool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperCarriedToolTest::RunTest(const FString& Parameters)
{
	UTunaSweeperGameInstance* Game = NewObject<UTunaSweeperGameInstance>();
	Game->bInventoryStateInitialized = true;
	FTunaSweeperItemInstance Tool;
	Tool.Uid = FGuid::NewGuid();
	Tool.ItemId = 6003;
	Game->ItemInstancesByUid.Add(Tool.Uid, Tool);
	FTunaSweeperInventorySlot Slot;
	Slot.ItemUid = Tool.Uid;
	Game->EquipmentSlots.Add(Slot);
	TestEqual(TEXT("Equipped crowbar satisfies reusable tool requirement"), Game->CountCarriedItemById(6003), 1);
	TestEqual(TEXT("Equipped tool is not consumable inventory"), Game->CountInventoryItemById(6003), 0);
	Game->EquipmentSlots.Reset();
	Game->StorageSlots.Add(Slot);
	TestEqual(TEXT("Stored tool cannot clear the screen"), Game->CountCarriedItemById(6003), 0);
	Game->StorageSlots.Reset();
	Game->AuxiliaryBagSlots.Add(Slot);
	TestEqual(TEXT("Auxiliary bag tool remains usable"), Game->CountCarriedItemById(6003), 1);
	return true;
}

#endif
