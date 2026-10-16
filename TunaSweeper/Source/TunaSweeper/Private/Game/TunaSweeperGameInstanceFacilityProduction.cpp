#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

bool UTunaSweeperGameInstance::ProduceFacilityBatch(FName ObjectId, int32 InputItemId, int32 InputQuantity,
    int32 OutputItemId, int32 OutputQuantity, bool bSaveImmediately)
{
    EnsureInventoryStateInitialized();
    const auto* Progress = WorldProgressStatesById.Find(ObjectId);
    auto* Items = GetSubsystem<UTunaSweeperItemDataSubsystem>();
    FTunaSweeperItemDefinition InputDefinition;
    FTunaSweeperItemDefinition OutputDefinition;
    if (ObjectId.IsNone() || !Progress || Progress->State != ETunaSweeperWorldProgressState::Completed ||
        InputItemId <= 0 || OutputItemId <= 0 || InputQuantity <= 0 || OutputQuantity <= 0 ||
        !Items || !Items->TryGetItemDefinition(InputItemId, InputDefinition) ||
        !Items->TryGetItemDefinition(OutputItemId, OutputDefinition) ||
        CountInventoryItemById(InputItemId) < InputQuantity) return false;

    const auto PreviousItems = ItemInstancesByUid;
    const auto PreviousInventory = PlayerInventorySlots;
    const auto PreviousBags = AuxiliaryBagSlots;
    const auto PreviousAcquiredIds = EverAcquiredItemIds;
    const auto Rollback = [&]
    {
        ItemInstancesByUid = PreviousItems;
        PlayerInventorySlots = PreviousInventory;
        AuxiliaryBagSlots = PreviousBags;
        EverAcquiredItemIds = PreviousAcquiredIds;
    };
    // These helpers mutate carried slots only, without broadcasting or saving partial payment.
    if (ConsumeInventoryAmmoByItemId(InputItemId, InputQuantity) != InputQuantity)
    {
        Rollback();
        return false;
    }
    int32 RemainingOutput = OutputQuantity;
    TryAddItemQuantityToExistingStacks(OutputItemId, RemainingOutput, PlayerInventorySlots);
    TryAddItemQuantityToFirstEmptySlots(OutputItemId, RemainingOutput, PlayerInventorySlots);
    if (RemainingOutput > 0)
    {
        Rollback();
        return false;
    }
    MarkItemEverAcquired(OutputItemId);
    if (bSaveImmediately && !SaveGameStateInternal())
    {
        Rollback();
        return false;
    }
    // Publish only after the inventory exchange has been committed durably.
    BroadcastInventoryStateChanged();
    if (auto* Quests = GetSubsystem<UTunaSweeperQuestSubsystem>())
        Quests->NotifyItemAcquired(OutputItemId, OutputQuantity, bSaveImmediately);
    return true;
}
