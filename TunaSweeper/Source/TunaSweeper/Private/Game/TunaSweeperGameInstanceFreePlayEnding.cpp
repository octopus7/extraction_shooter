#include "TunaSweeperGameInstanceShared.h"

bool UTunaSweeperGameInstance::IsRaidStorageReadyForTrueEnding()
{
    EnsureInventoryStateInitialized();
    SynchronizeRaidStorage();
    if (RaidStorageSlots.Num() != RaidStorageCapacity) return false;
    TSet<FGuid> Seen;
    for (const auto& Slot : RaidStorageSlots)
    {
        const auto* Item = ItemInstancesByUid.Find(Slot.ItemUid);
        if (!Item || Item->ItemId != 3004 || Item->Quantity != 10 || Seen.Contains(Slot.ItemUid)) return false;
        Seen.Add(Slot.ItemUid);
    }
    return true;
}

bool UTunaSweeperGameInstance::SaveFreePlayEnding(bool bTrueEnding)
{
    if (IsCombatTestSession()) return false;
    EnsureInventoryStateInitialized();
    SynchronizeRaidStorage();
    if (bTrueEnding && !IsRaidStorageReadyForTrueEnding()) return false;
    const FName Flag(bTrueEnding ? TEXT("ending.freeplay.true_seen") : TEXT("ending.freeplay.normal_seen"));
    const bool bAlreadySeen = CompletedScenarioFlags.Contains(Flag);
    const int64 PreviousExperience = TotalExperiencePoints;
    // Settle earned raid XP once, together with the ending flag and both inventories.
    TotalExperiencePoints = FMath::Max(TotalExperiencePoints,
        bRaidExperienceSessionActive ? RaidStartExperiencePoints : TotalExperiencePoints)
        + FMath::Max<int64>(0, PendingRaidExperiencePoints);
    CompletedScenarioFlags.Add(Flag);
    if (!SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime))
    {
        TotalExperiencePoints = PreviousExperience;
        if (!bAlreadySeen) CompletedScenarioFlags.Remove(Flag);
        return false;
    }
    ClearRaidExperienceGain();
    bPendingBunkerItemStateSave = false;
    bPendingRaidStorageSave = false;
    RefreshCarryWeightState();
    OnExperienceChanged.Broadcast();
    return true;
}
