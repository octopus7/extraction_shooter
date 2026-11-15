#include "TunaSweeperGameInstanceShared.h"
#include "Engine/World.h"

bool UTunaSweeperGameInstance::TryFillRaidStorageForTrueEndingTest()
{
    // Explicit helper action only: never initialize a slot or invoke starter grants here.
    const UWorld* World = GetWorld();
    if (!IsGateCheatSaveReady() || IsCombatTestSession() || !World || !World->IsGameWorld()
        || World->GetNetMode() != NM_Standalone || ActiveSaveSlotIndex != SanitizeSaveSlotIndex(ActiveSaveSlotIndex)) return false;

    const auto PreviousItems = ItemInstancesByUid;
    const auto PreviousWarehouse = RaidStorageSlots;
    const auto PreviousActiveSlots = ActiveLootContainerSlots;
    const bool bPreviousInitialized = bRaidStorageInitialized;
    const bool bPreviousWarehouseDirty = bPendingRaidStorageSave;
    const bool bPreviousBunkerDirty = bPendingBunkerItemStateSave;
    const bool bPreviousMarkersDirty = bMapMarkersDirty;

    TSet<FGuid> ReplacedGraph;
    CollectItemUidsFromSlots(RaidStorageSlots, ReplacedGraph);
    if (bActiveLootContainerIsRaidStorage && bHasActiveLootContainer)
        CollectItemUidsFromSlots(ActiveLootContainerSlots, ReplacedGraph);

    TSet<FGuid> ProtectedGraph;
    CollectItemUidsFromSlots(PlayerInventorySlots, ProtectedGraph);
    CollectItemUidsFromSlots(EquipmentSlots, ProtectedGraph);
    CollectItemUidsFromSlots(AuxiliaryBagSlots, ProtectedGraph);
    CollectItemUidsFromSlots(UsableQuickSlots, ProtectedGraph);
    CollectItemUidsFromSlots(StorageSlots, ProtectedGraph);
    if (!bActiveLootContainerIsRaidStorage)
        CollectItemUidsFromSlots(ActiveLootContainerSlots, ProtectedGraph);
    // Other live containers may own item instances outside the current UI. Preserve
    // references from every graph outside the warehouse, including shared descendants.
    TArray<FTunaSweeperInventorySlot> OtherRoots;
    for (const auto& Pair : ItemInstancesByUid)
    {
        if (!ReplacedGraph.Contains(Pair.Key))
        {
            FTunaSweeperInventorySlot& Root = OtherRoots.AddDefaulted_GetRef();
            Root.ItemUid = Pair.Key;
        }
    }
    CollectItemUidsFromSlots(OtherRoots, ProtectedGraph);
    for (const FGuid& Uid : ReplacedGraph)
        if (!ProtectedGraph.Contains(Uid)) ItemInstancesByUid.Remove(Uid);

    RaidStorageSlots.Reset();
    RaidStorageSlots.SetNum(RaidStorageCapacity);
    for (auto& Slot : RaidStorageSlots) Slot.ItemUid = CreateItemInstance(3004, 10);
    if (bActiveLootContainerIsRaidStorage && bHasActiveLootContainer)
        ActiveLootContainerSlots = RaidStorageSlots;
    bRaidStorageInitialized = true;
    if (!SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime))
    {
        ItemInstancesByUid = PreviousItems;
        RaidStorageSlots = PreviousWarehouse;
        ActiveLootContainerSlots = PreviousActiveSlots;
        bRaidStorageInitialized = bPreviousInitialized;
        bPendingRaidStorageSave = bPreviousWarehouseDirty;
        bPendingBunkerItemStateSave = bPreviousBunkerDirty;
        bMapMarkersDirty = bPreviousMarkersDirty;
        return false;
    }
    bPendingRaidStorageSave = false;
    bPendingBunkerItemStateSave = false;
    BroadcastInventoryStateChanged();
    return true;
}
