#include "TunaSweeperGameInstanceShared.h"
#include "Engine/Engine.h"

const TArray<FTunaSweeperInventorySlot>& UTunaSweeperGameInstance::GetRaidStorageSlots()
{
    EnsureInventoryStateInitialized();
    EnsureSlotArraySize(RaidStorageSlots, RaidStorageCapacity);
    return RaidStorageSlots;
}

void UTunaSweeperGameInstance::OpenRaidStorage(UObject* Owner, const FText& DisplayName)
{
    EnsureInventoryStateInitialized();
    EnsureSlotArraySize(RaidStorageSlots, RaidStorageCapacity);
    FTunaSweeperLootContainerInstance Container;
    Container.Capacity = RaidStorageCapacity;
    Container.DisplayName = DisplayName;
    SetActiveLootContainerRuntimeSlots(Container, RaidStorageSlots, Owner);
    bActiveLootContainerIsRaidStorage = true;
}

void UTunaSweeperGameInstance::SynchronizeRaidStorage()
{
    if (bActiveLootContainerIsRaidStorage && bHasActiveLootContainer)
    {
        RaidStorageSlots = ActiveLootContainerSlots;
        EnsureSlotArraySize(RaidStorageSlots, RaidStorageCapacity);
    }
}

bool UTunaSweeperGameInstance::FlushRaidStorageSave()
{
    if (!bPendingRaidStorageSave) return true;
    if (IsCombatTestSession()) return false;
    // Commit BOTH ends of transfers, including quick-slot references, in one file.
    if (SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime))
    {
        bPendingRaidStorageSave = false;
        bPendingBunkerItemStateSave = false;
        return true;
    }
    // Keep current state and the retry flag; never claim a failed disk write succeeded.
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(0x52414944, 8.0f, FColor::Red,
            ResolveLocalizedText(TEXT("ui.raid_storage.save_failed"), FText::GetEmpty()).ToString());
    UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Raid storage save failed; retained runtime state for retry."));
    return false;
}

void UTunaSweeperGameInstance::CloseRaidStorage(UObject* Owner)
{
    if (!bActiveLootContainerIsRaidStorage || ActiveLootContainerOwner.Get() != Owner) return;
    SynchronizeRaidStorage();
    FlushRaidStorageSave();
    bActiveLootContainerIsRaidStorage = false;
    bHasActiveLootContainer = false;
    ActiveLootContainerSlots.Reset();
    ActiveLootContainerOwner.Reset();
    ActiveLootContainerCapacity = 0;
    ActiveLootContainerDisplayName = FText::GetEmpty();
}
