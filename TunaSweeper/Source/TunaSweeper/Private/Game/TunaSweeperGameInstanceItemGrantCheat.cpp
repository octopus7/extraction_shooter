#include "TunaSweeperGameInstanceShared.h"
#include "Debug/TunaSweeperGateCheatSubsystem.h"
#include "Debug/TunaSweeperMovementCheatSubsystem.h"
#include "Engine/World.h"

uint32 UTunaSweeperGameInstance::TryGrantItemForHelper(int32 ItemId, int32 Quantity)
{
    using EResult = ETunaSweeperGateCheatResult;
    const auto* Movement = GetSubsystem<UTunaSweeperMovementCheatSubsystem>();
    if (!Movement || !Movement->IsEnabled()) return uint32(EResult::Disabled);
    const UWorld* World = GetWorld();
    if (!IsGateCheatSaveReady() || IsCombatTestSession() || !World || !World->IsGameWorld()
        || World->GetNetMode() != NM_Standalone || ActiveSaveSlotIndex != SanitizeSaveSlotIndex(ActiveSaveSlotIndex)) return uint32(EResult::NoContext);
    FTunaSweeperItemDefinition Definition;
    auto* Data = GetSubsystem<UTunaSweeperItemDataSubsystem>();
    if (ItemId <= 0 || Quantity <= 0 || Quantity > 1000000 || !Data || !Data->TryGetItemDefinition(ItemId, Definition)) return uint32(EResult::InvalidRequest);

    const auto PreviousItems = ItemInstancesByUid;
    const auto PreviousSlots = PlayerInventorySlots;
    const auto PreviousAcquired = EverAcquiredItemIds;
    const bool bPreviousWarehouseDirty = bPendingRaidStorageSave;
    const bool bPreviousBunkerDirty = bPendingBunkerItemStateSave;
    const bool bPreviousMarkersDirty = bMapMarkersDirty;
    auto Restore = [&]
    {
        ItemInstancesByUid = PreviousItems;
        PlayerInventorySlots = PreviousSlots;
        EverAcquiredItemIds = PreviousAcquired;
        bPendingRaidStorageSave = bPreviousWarehouseDirty;
        bPendingBunkerItemStateSave = bPreviousBunkerDirty;
        bMapMarkersDirty = bPreviousMarkersDirty;
    };
    int32 Remaining = Quantity;
    TryAddItemQuantityToExistingStacks(ItemId, Remaining, PlayerInventorySlots);
    TryAddItemQuantityToFirstEmptySlots(ItemId, Remaining, PlayerInventorySlots);
    if (Remaining > 0) { Restore(); return uint32(EResult::InventoryFull); }
    EverAcquiredItemIds.Add(ItemId);
    if (!SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime)) { Restore(); return uint32(EResult::SaveFailed); }
    bPendingRaidStorageSave = false;
    bPendingBunkerItemStateSave = false;
    BroadcastInventoryStateChanged();
    return uint32(EResult::ItemGranted);
}
