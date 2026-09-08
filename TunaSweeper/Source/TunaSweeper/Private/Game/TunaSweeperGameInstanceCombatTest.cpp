#include "TunaSweeperGameInstanceShared.h"

struct UTunaSweeperGameInstance::FCombatTestInventoryBackup
{
	TMap<FGuid, FTunaSweeperItemInstance> Items;
	TArray<FTunaSweeperInventorySlot> Inventory, Equipment, Auxiliary, Quick, Storage;
	TSet<int32> AcquiredItems;
	int32 WeaponSlot = 1;
	bool bMeleeSelected = false;
	bool bHasSelection = false;
	bool bPendingSave = false;
	int64 TotalExperience = 0;
	int64 RaidStartExperience = 0;
	int64 PendingExperience = 0;
	FTunaSweeperExperienceAnimationState ExperienceAnimation;
	bool bRaidSession = false;
	bool bPendingAnimation = false;
};

void UTunaSweeperGameInstance::BeginCombatTestSession()
{
	if (IsCombatTestSession()) return;
	EnsureInventoryStateInitialized();
	auto Backup = MakeShared<FCombatTestInventoryBackup>();
	Backup->Items = ItemInstancesByUid;
	Backup->Inventory = PlayerInventorySlots;
	Backup->Equipment = EquipmentSlots;
	Backup->Auxiliary = AuxiliaryBagSlots;
	Backup->Quick = UsableQuickSlots;
	Backup->Storage = StorageSlots;
	Backup->AcquiredItems = EverAcquiredItemIds;
	Backup->WeaponSlot = RuntimeSelectedWeaponSlotNumber;
	Backup->bMeleeSelected = bRuntimeSelectedMeleeWeapon;
	Backup->bHasSelection = bHasRuntimeSelectedWeaponSelection;
	Backup->bPendingSave = bPendingBunkerItemStateSave;
	Backup->TotalExperience = TotalExperiencePoints;
	Backup->RaidStartExperience = RaidStartExperiencePoints;
	Backup->PendingExperience = PendingRaidExperiencePoints;
	Backup->ExperienceAnimation = PendingRaidExperienceAnimationState;
	Backup->bRaidSession = bRaidExperienceSessionActive;
	Backup->bPendingAnimation = bHasPendingRaidExperienceAnimationState;
	CombatTestInventoryBackup = Backup;
	ResetCombatTestLoadout();
}

void UTunaSweeperGameInstance::ResetCombatTestLoadout()
{
	if (!IsCombatTestSession()) return;
	ClearSelectedItemSelection();
	ClearHoveredItemSlot();
	ItemInstancesByUid.Reset();
	ResetPlayerSlotArrays();
	StorageSlots.Reset();
	EnsureSlotArraySize(StorageSlots, StorageSlotCapacity);
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	bHasActiveLootContainer = false;
	bPendingBunkerItemStateSave = false;
	if (!InitializeDemoStartingLoadout())
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Boss lab could not initialize rifle and ammunition."));
	}
	AddItemToFirstAvailableInventorySlot(2002, 300);
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::EndCombatTestSession()
{
	if (!IsCombatTestSession()) return;
	const auto Backup = CombatTestInventoryBackup;
	ItemInstancesByUid = Backup->Items;
	PlayerInventorySlots = Backup->Inventory;
	EquipmentSlots = Backup->Equipment;
	AuxiliaryBagSlots = Backup->Auxiliary;
	UsableQuickSlots = Backup->Quick;
	StorageSlots = Backup->Storage;
	EverAcquiredItemIds = Backup->AcquiredItems;
	RuntimeSelectedWeaponSlotNumber = Backup->WeaponSlot;
	bRuntimeSelectedMeleeWeapon = Backup->bMeleeSelected;
	bHasRuntimeSelectedWeaponSelection = Backup->bHasSelection;
	bPendingBunkerItemStateSave = Backup->bPendingSave;
	TotalExperiencePoints = Backup->TotalExperience;
	RaidStartExperiencePoints = Backup->RaidStartExperience;
	PendingRaidExperiencePoints = Backup->PendingExperience;
	PendingRaidExperienceAnimationState = Backup->ExperienceAnimation;
	bRaidExperienceSessionActive = Backup->bRaidSession;
	bHasPendingRaidExperienceAnimationState = Backup->bPendingAnimation;
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	bHasActiveLootContainer = false;
	ClearSelectedItemSelection();
	ClearHoveredItemSlot();
	// Keep the save guard up while listeners refresh the restored equipment.
	BroadcastInventoryStateChanged();
	CombatTestInventoryBackup.Reset();
}
