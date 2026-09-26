#include "Game/TunaSweeperGameInstance.h"

bool UTunaSweeperGameInstance::TryConsumeInventoryItems(
	const TArray<FTunaSweeperItemStack>& Requirements, TFunctionRef<void()> OnConsumed)
{
	EnsureInventoryStateInitialized();
	if (Requirements.IsEmpty()) return false;
	TMap<int32, int32> Totals;
	auto* Items = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	for (const auto& Requirement : Requirements)
	{
		FTunaSweeperItemDefinition Definition;
		if (Requirement.ItemId <= 0 || Requirement.Quantity <= 0 || !Items ||
			!Items->TryGetItemDefinition(Requirement.ItemId, Definition)) return false;
		int32& Total = Totals.FindOrAdd(Requirement.ItemId);
		if (Requirement.Quantity > MAX_int32 - Total) return false;
		Total += Requirement.Quantity;
	}
	for (const auto& Total : Totals)
	{
		if (CountInventoryItemById(Total.Key) < Total.Value) return false;
	}

	// No delegates run between validation and consumption. Keep a rollback for inconsistent slot data.
	const auto PreviousItems = ItemInstancesByUid;
	const auto PreviousInventory = PlayerInventorySlots;
	const auto PreviousBags = AuxiliaryBagSlots;
	for (const auto& Total : Totals)
	{
		if (ConsumeInventoryAmmoByItemId(Total.Key, Total.Value) != Total.Value)
		{
			ItemInstancesByUid = PreviousItems;
			PlayerInventorySlots = PreviousInventory;
			AuxiliaryBagSlots = PreviousBags;
			return false;
		}
	}
	OnConsumed();
	ClearSelectedItemIfInvalid();
	MarkItemStateMutationForSave();
	BroadcastInventoryStateChanged();
	return true;
}
