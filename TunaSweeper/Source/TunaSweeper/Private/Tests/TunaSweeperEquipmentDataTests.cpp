#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperEquipmentDataTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperEquipmentDataTest,
	"TunaSweeper.Inventory.EquipmentData.LaserAndStartingLoadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperEquipmentDataTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperEquipmentDataTests;
	UTunaSweeperGameInstance* UninitializedGame = NewObject<UTunaSweeperGameInstance>();
	TestFalse(TEXT("A missing item subsystem is not reported as invalid JSON"), UninitializedGame->InitializeDemoStartingLoadout());
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient world"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	Game->bInventoryStateInitialized = true;
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
	UTunaSweeperItemDataSubsystem* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!TestTrue(TEXT("Production item definitions load"), Items && Items->LoadItemData())) return false;
	if (!TestTrue(TEXT("Starting equipment initializes"), Game->InitializeDemoStartingLoadout())) return false;
	ATunaSweeperTopDownCharacter* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>();
	if (!TestNotNull(TEXT("Character"), Character)) return false;
	Character->SelectedWeaponSlotNumber = 1;
	Character->bMeleeWeaponSelected = false;
	TestTrue(TEXT("Starting laser works on rifle"), Character->IsSelectedWeaponLaserSightEquipped());
	FTunaSweeperItemInstance& Rifle = Game->ItemInstancesByUid.FindChecked(Game->EquipmentSlots[0].ItemUid);
	FTunaSweeperItemInstance& Attachment = Game->ItemInstancesByUid.FindChecked(
		Rifle.AttachmentSlots.FindChecked(FName(TEXT("attachment.slot.tactical"))));
	FTunaSweeperItemDefinition AlternativeLaser = Items->ItemDefinitionsById.FindChecked(Attachment.ItemId);
	AlternativeLaser.Id = 92006;
	Items->ItemDefinitionsById.Add(AlternativeLaser.Id, AlternativeLaser);
	Attachment.ItemId = AlternativeLaser.Id;
	TestTrue(TEXT("A laser with a different item ID still works"), Character->IsSelectedWeaponLaserSightEquipped());
	Rifle.ItemId = 1006;
	TestTrue(TEXT("A compatible SMG also uses the laser"), Character->IsSelectedWeaponLaserSightEquipped());
	Items->ItemDefinitionsById.FindChecked(AlternativeLaser.Id).bProvidesLaserSight = false;
	TestFalse(TEXT("A tactical attachment without the laser capability stays off"), Character->IsSelectedWeaponLaserSightEquipped());
	Items->ItemDefinitionsById.FindChecked(AlternativeLaser.Id).bProvidesLaserSight = true;
	Items->ItemDefinitionsById.FindChecked(AlternativeLaser.Id).CompatibleWeaponTypeTags = {FName(TEXT("weapon.type.rifle"))};
	TestFalse(TEXT("An incompatible laser cannot activate on an SMG"), Character->IsSelectedWeaponLaserSightEquipped());
	Items->ItemDefinitionsById.FindChecked(AlternativeLaser.Id).CompatibleWeaponTypeTags.Add(FName(TEXT("weapon.type.smg")));
	Character->bMeleeWeaponSelected = true;
	TestFalse(TEXT("Selecting melee switches off the gun laser"), Character->IsSelectedWeaponLaserSightEquipped());
	Character->bMeleeWeaponSelected = false;

	// A different weapon, attachment ID, slot, ammunition and armor must all come from JSON.
	Game->ItemInstancesByUid.Reset();
	Game->ResetPlayerSlotArrays();
	const FString CustomLoadout = TEXT(R"({
		"selected_weapon_slot":2,
		"equipment":[
			{"slot_index":1,"item_id":1006,"attachments":[92006],"loaded_ammo":{"item_id":2001,"quantity":7}},
			{"slot_index":3,"item_id":5006},
			{"slot_index":7,"item_id":5002}],
		"inventory":[{"slot_index":49,"item_id":2001,"quantity":11}]
	})");
	if (!TestTrue(TEXT("Custom loadout applies"), Game->ApplyStartingLoadoutJson(CustomLoadout))) return false;
	TestTrue(TEXT("Slot one remains empty"), Game->EquipmentSlots[0].IsEmpty());
	FTunaSweeperItemInstance CustomWeapon;
	FTunaSweeperItemDefinition CustomWeaponDefinition;
	TestTrue(TEXT("SMG goes into slot two"), Game->TryGetEquipmentWeaponSlotItem(2, CustomWeapon, CustomWeaponDefinition));
	TestEqual(TEXT("Configured weapon ID"), CustomWeapon.ItemId, 1006);
	TestEqual(TEXT("Configured loaded ammo ID"), CustomWeapon.LoadedAmmoItemId, 2001);
	TestEqual(TEXT("Configured loaded quantity"), CustomWeapon.LoadedAmmoCount, 7);
	TestEqual(TEXT("Backpack extends capacity before inventory is placed"), Game->PlayerInventorySlots.Num(), 50);
	TestEqual(TEXT("Configured inventory quantity"), Game->ItemInstancesByUid.FindChecked(Game->PlayerInventorySlots[49].ItemUid).Quantity, 11);
	TestEqual(TEXT("Configured armor"), Game->ItemInstancesByUid.FindChecked(Game->EquipmentSlots[3].ItemUid).ItemId, 5006);
	TestEqual(TEXT("Configured weapon selection"), Game->RuntimeSelectedWeaponSlotNumber, 2);
	TestTrue(TEXT("Nested attachment is included in acquisition history"), Game->EverAcquiredItemIds.Contains(92006));
	TestFalse(TEXT("Applying over occupied starting slots is rejected"), Game->ApplyStartingLoadoutJson(CustomLoadout));
	TestEqual(TEXT("Existing loadout survives rejection"), Game->GetWeaponLoadedAmmoCount(2), 7);
	Game->GrantCombatTestReserveAmmo();
	TestEqual(TEXT("Lab reserve follows the configured SMG ammo"), Game->CountInventoryItemById(2001), 311);
	TestEqual(TEXT("Lab does not supply unrelated rifle ammo"), Game->CountInventoryItemById(2002), 0);

	const TArray<FString> InvalidLoadouts = {
		TEXT("{broken"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":999999}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":5006}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002,"attachments":[3001]}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002,"attachments":[2006,2006]}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002,"loaded_ammo":{"item_id":2001,"quantity":2}}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002,"loaded_ammo":{"item_id":2002,"quantity":999}}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002},{"slot_index":0,"item_id":1002}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":2,"equipment":[{"slot_index":0,"item_id":1002}],"inventory":[]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002}],"inventory":[{"slot_index":0,"item_id":2002,"quantity":1.5}]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002}],"inventory":[{"slot_index":40,"item_id":2002,"quantity":1}]})"),
		TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002}],"inventory":[{"slot_index":0,"item_id":2002,"quantity":999999}]})")
	};
	for (const FString& Invalid : InvalidLoadouts)
	{
		Game->ItemInstancesByUid.Reset();
		Game->ResetPlayerSlotArrays();
		Game->EverAcquiredItemIds.Reset();
		Game->SetRuntimeSelectedWeaponSlotNumber(2);
		TestFalse(TEXT("Invalid loadout is rejected"), Game->ApplyStartingLoadoutJson(Invalid));
		TestEqual(TEXT("Rejected loadout leaves no items"), Game->ItemInstancesByUid.Num(), 0);
		TestTrue(TEXT("Rejected loadout leaves no equipment"), Game->EquipmentSlots[0].IsEmpty());
		TestEqual(TEXT("Rejected loadout preserves acquisition history"), Game->EverAcquiredItemIds.Num(), 0);
		TestEqual(TEXT("Rejected loadout preserves selection"), Game->RuntimeSelectedWeaponSlotNumber, 2);
	}
	TestTrue(TEXT("Default file can initialize after failed attempts"), Game->InitializeDemoStartingLoadout());
	TestEqual(TEXT("Default loaded rounds"), Game->GetWeaponLoadedAmmoCount(1), 30);
	TestEqual(TEXT("Default reserve rounds"), Game->GetWeaponInventoryAmmoCount(1), 30);

	// The fifth backpack must override legacy 100-slot defaults and retain the
	// final slot through the real save serializer, without writing player files.
	Game->ItemInstancesByUid.Reset();
	Game->ResetPlayerSlotArrays();
	Game->GameplaySettings.MaxInventorySlots = 100;
	const FString CarbonLoadout = TEXT(R"({"selected_weapon_slot":1,
		"equipment":[{"slot_index":0,"item_id":1002},{"slot_index":7,"item_id":5011}],
		"inventory":[{"slot_index":119,"item_id":2001,"quantity":11}]})");
	if (!TestTrue(TEXT("Carbon frame backpack accepts the 120th slot"), Game->ApplyStartingLoadoutJson(CarbonLoadout))) return false;
	TestEqual(TEXT("Carbon frame capacity is 120 despite legacy settings"), Game->PlayerInventorySlots.Num(), 120);
	const FGuid LastSlotUid = Game->PlayerInventorySlots[119].ItemUid;
	TStrongObjectPtr<UTunaSweeperSaveGame> Snapshot(NewObject<UTunaSweeperSaveGame>());
	Snapshot->InventorySlots = Game->PlayerInventorySlots;
	Snapshot->EquipmentSlots = Game->EquipmentSlots;
	Game->ItemInstancesByUid.GenerateValueArray(Snapshot->ItemInstances);
	TArray<uint8> SaveBytes;
	if (!TestTrue(TEXT("120-slot inventory serializes"), UGameplayStatics::SaveGameToMemory(Snapshot.Get(), SaveBytes))) return false;
	TStrongObjectPtr<UTunaSweeperSaveGame> Restored(Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes)));
	if (!TestNotNull(TEXT("120-slot inventory deserializes"), Restored.Get())) return false;
	TestEqual(TEXT("Save retains all 120 slots"), Restored->InventorySlots.Num(), 120);
	TestEqual(TEXT("Save retains the last-slot item UID"), Restored->InventorySlots[119].ItemUid, LastSlotUid);
	const FTunaSweeperItemInstance* SavedItem = Restored->ItemInstances.FindByPredicate(
		[&LastSlotUid](const FTunaSweeperItemInstance& Item) { return Item.Uid == LastSlotUid; });
	if (!TestNotNull(TEXT("Last-slot item exists in serialized instances"), SavedItem)) return false;
	TestEqual(TEXT("Last-slot quantity survives serialization"), SavedItem->Quantity, 11);
	TestEqual(TEXT("Loaded backpack still resolves 120 slots"), Game->CalculateInventoryCapacityForEquipmentSlots(Restored->EquipmentSlots), 120);
	return true;
}

#endif
