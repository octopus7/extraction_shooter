#include "Game/TunaSweeperGameInstance.h"

int32 UTunaSweeperGameInstance::CountQuestSubmissionItemById(int32 ItemId)
{
    EnsureInventoryStateInitialized();
    int64 Count = 0;
    TSet<FGuid> Seen;
    for (const auto* Slots : { &PlayerInventorySlots, &AuxiliaryBagSlots, &StorageSlots })
        for (int32 Index = 0; Index < Slots->Num(); ++Index)
        {
            if (Slots == &StorageSlots && Index >= GetAccessibleStorageSlotCount()) break;
            const auto& Slot = (*Slots)[Index];
            if (Seen.Contains(Slot.ItemUid)) continue;
            Seen.Add(Slot.ItemUid);
            const auto* Item = ItemInstancesByUid.Find(Slot.ItemUid);
            if (Item && Item->ItemId == ItemId) Count += FMath::Max(0, Item->Quantity);
        }
    return static_cast<int32>(FMath::Min<int64>(Count, MAX_int32));
}

bool UTunaSweeperGameInstance::TryConsumeQuestItems(
    const TArray<FTunaSweeperItemStack>& Requirements, TFunctionRef<void()> OnConsumed, bool bNotifyAndSave)
{
    EnsureInventoryStateInitialized();
    if (IsCombatTestSession() || Requirements.IsEmpty() || !ValidateQuestItemRewards(Requirements)) return false;
    TMap<int32, int32> Totals;
    for (const auto& Entry : Requirements)
    {
        int32& Total = Totals.FindOrAdd(Entry.ItemId);
        if (Entry.Quantity > MAX_int32 - Total) return false;
        Total += Entry.Quantity;
    }
    for (const auto& Entry : Totals)
        if (CountQuestSubmissionItemById(Entry.Key) < Entry.Value) return false;

    // All requirements validated together. No delegates or saves until the batch is committed.
    for (const auto& Entry : Totals)
    {
        int32 Remaining = Entry.Value;
        for (auto* Slots : { &PlayerInventorySlots, &AuxiliaryBagSlots, &StorageSlots })
            for (int32 Index = 0; Index < Slots->Num(); ++Index)
            {
                if (Slots == &StorageSlots && Index >= GetAccessibleStorageSlotCount()) break;
                auto& Slot = (*Slots)[Index];
                auto* Item = ItemInstancesByUid.Find(Slot.ItemUid);
                if (!Item || Item->ItemId != Entry.Key || Remaining <= 0) continue;
                const int32 Taken = FMath::Min(Remaining, FMath::Max(0, Item->Quantity));
                Item->Quantity -= Taken;
                Remaining -= Taken;
                if (Item->Quantity == 0)
                {
                    const FGuid Removed = Slot.ItemUid;
                    ItemInstancesByUid.Remove(Removed);
                    // Clear duplicate references as well; a UID is never counted/consumed twice.
                    for (auto* AllSlots : { &PlayerInventorySlots, &AuxiliaryBagSlots, &StorageSlots })
                        for (auto& Ref : *AllSlots) if (Ref.ItemUid == Removed) Ref.ItemUid.Invalidate();
                }
            }
    }
    OnConsumed();
    if (bNotifyAndSave)
    {
        NotifyQuestItemMutation();
        MarkItemStateMutationForSave();
    }
    return true;
}

bool UTunaSweeperGameInstance::ValidateQuestItemRewards(const TArray<FTunaSweeperItemStack>& Rewards) const
{
    auto* Data = GetSubsystem<UTunaSweeperItemDataSubsystem>();
    for (const auto& Entry : Rewards)
    {
        FTunaSweeperItemDefinition Definition;
        if (Entry.ItemId <= 0 || Entry.Quantity <= 0 || !Data || !Data->TryGetItemDefinition(Entry.ItemId, Definition)) return false;
    }
    return true;
}

void UTunaSweeperGameInstance::NotifyQuestItemMutation()
{
    BroadcastInventoryStateChanged();
}

const TArray<FTunaSweeperItemStack>& UTunaSweeperGameInstance::GetPendingQuestRewards()
{
    EnsureInventoryStateInitialized();
    return PendingQuestRewards;
}

bool UTunaSweeperGameInstance::GrantQuestItemRewards(const TArray<FTunaSweeperItemStack>& Rewards, bool bNotifyAndSave)
{
    EnsureInventoryStateInitialized();
    if (IsCombatTestSession() || bQuestRewardTransferInProgress || !ValidateQuestItemRewards(Rewards)) return false;
    TGuardValue<bool> Guard(bQuestRewardTransferInProgress, true);
    for (const auto& Entry : Rewards)
    {
        int32 Remaining = Entry.Quantity;
        TryAddItemQuantityToExistingStacks(Entry.ItemId, Remaining, PlayerInventorySlots);
        TryAddItemQuantityToFirstEmptySlots(Entry.ItemId, Remaining, PlayerInventorySlots);
        if (Remaining < Entry.Quantity) MarkItemEverAcquired(Entry.ItemId);
        if (Remaining > 0)
        {
            // Stack-sized entries allow each receive button to request one normal item stack.
            FTunaSweeperItemDefinition Definition;
            GetSubsystem<UTunaSweeperItemDataSubsystem>()->TryGetItemDefinition(Entry.ItemId, Definition);
            const int32 MaxStack = FMath::Max(1, GetSubsystem<UTunaSweeperItemDataSubsystem>()->ResolveItemMaxStackQuantity(Definition));
            while (Remaining > 0)
            {
                FTunaSweeperItemStack Pending;
                Pending.ItemId = Entry.ItemId;
                Pending.Quantity = FMath::Min(Remaining, MaxStack);
                PendingQuestRewards.Add(Pending);
                Remaining -= Pending.Quantity;
            }
        }
    }
    if (bNotifyAndSave && !Rewards.IsEmpty())
    {
        NotifyQuestItemMutation();
        MarkItemStateMutationForSave();
    }
    return true;
}

bool UTunaSweeperGameInstance::ReceivePendingQuestReward(int32 Index)
{
    EnsureInventoryStateInitialized();
    if (IsCombatTestSession() || bQuestRewardTransferInProgress || !PendingQuestRewards.IsValidIndex(Index)) return false;
    TGuardValue<bool> Guard(bQuestRewardTransferInProgress, true);
    const FTunaSweeperItemStack Reward = PendingQuestRewards[Index];
    if (!ValidateQuestItemRewards({Reward}) || !CanGrantQuestItemRewards({Reward})) return false;
    const auto PreviousItems = ItemInstancesByUid;
    const auto PreviousSlots = PlayerInventorySlots;
    int32 Remaining = Reward.Quantity;
    TryAddItemQuantityToExistingStacks(Reward.ItemId, Remaining, PlayerInventorySlots);
    TryAddItemQuantityToFirstEmptySlots(Reward.ItemId, Remaining, PlayerInventorySlots);
    if (Remaining > 0)
    {
        ItemInstancesByUid = PreviousItems;
        PlayerInventorySlots = PreviousSlots;
        return false;
    }
    PendingQuestRewards.RemoveAt(Index);
    MarkItemEverAcquired(Reward.ItemId);
    NotifyQuestItemMutation();
    // Receiving and removal from escrow belong to the same save, including in the base.
    SaveGameStateInternal();
    return true;
}
