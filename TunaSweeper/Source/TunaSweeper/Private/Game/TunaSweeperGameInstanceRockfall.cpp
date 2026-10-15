#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

bool UTunaSweeperGameInstance::CommitRockfallProgress(FName ObjectId, int32 ExplosiveItemId, bool bComplete, bool bSaveImmediately)
{
    EnsureInventoryStateInitialized();
    if (ObjectId.IsNone()) return false;
    const auto* Existing = WorldProgressStatesById.Find(ObjectId);
    if (Existing && Existing->State == ETunaSweeperWorldProgressState::Completed) return false;
    if (bComplete && (!Existing || Existing->ProgressQuantity < 1)) return false;
    if (!bComplete)
    {
        if (Existing && Existing->ProgressQuantity >= 1) return false;
        auto* Items = GetSubsystem<UTunaSweeperItemDataSubsystem>();
        FTunaSweeperItemDefinition Definition;
        if (ExplosiveItemId <= 0 || !Items || !Items->TryGetItemDefinition(ExplosiveItemId,Definition) ||
            CountInventoryItemById(ExplosiveItemId) < 1) return false;
    }
    const auto PreviousItems = ItemInstancesByUid;
    const auto PreviousInventory = PlayerInventorySlots;
    const auto PreviousBags = AuxiliaryBagSlots;
    const auto PreviousProgress = WorldProgressStatesById;
    const auto Rollback = [&]()
    {
        ItemInstancesByUid = PreviousItems; PlayerInventorySlots = PreviousInventory;
        AuxiliaryBagSlots = PreviousBags; WorldProgressStatesById = PreviousProgress;
    };
    if (!bComplete && ConsumeInventoryAmmoByItemId(ExplosiveItemId,1) != 1) { Rollback(); return false; }
    FTunaSweeperWorldProgressSaveData State;
    State.ObjectId = ObjectId; State.InfoId = TEXT("world_progress.rockfall");
    State.State = bComplete ? ETunaSweeperWorldProgressState::Completed : ETunaSweeperWorldProgressState::InProgress;
    State.ProgressQuantity = 1;
    WorldProgressStatesById.Add(ObjectId,State);
    if (bSaveImmediately && !SaveGameStateInternal()) { Rollback(); return false; }
    ClearSelectedItemIfInvalid();
    // Publish after the durable commit so failed saves cannot complete quests or open paired passages.
    // Quest load reconciliation derives any not-yet-exported objective progress from this saved world state.
    OnWorldProgressChanged.Broadcast();
    if (!bComplete) BroadcastInventoryStateChanged();
    return true;
}
