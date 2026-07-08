#include "TunaSweeperGameInstanceShared.h"

void UTunaSweeperGameInstance::SetActiveShop(int32 ShopId)
{
	EnsureInventoryStateInitialized();

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopDefinition ShopDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetShopDefinition(ShopId, ShopDefinition))
	{
		ClearActiveShop();
		return;
	}

	ActiveShopId = ShopId;
	bHasActiveShop = true;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::ClearActiveShop()
{
	const bool bHadActiveShop = bHasActiveShop || ActiveShopId != INDEX_NONE;
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;

	if (bHadActiveShop)
	{
		BroadcastInventoryStateChanged();
	}
}

bool UTunaSweeperGameInstance::GetActiveShopItems(TArray<FTunaSweeperShopItemView>& OutShopItems)
{
	EnsureInventoryStateInitialized();
	OutShopItems.Reset();

	if (!bHasActiveShop || ActiveShopId == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopDefinition ShopDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetShopDefinition(ActiveShopId, ShopDefinition))
	{
		return false;
	}

	OutShopItems.Reserve(ShopDefinition.Items.Num());
	for (int32 SlotIndex = 0; SlotIndex < ShopDefinition.Items.Num(); ++SlotIndex)
	{
		const FTunaSweeperShopItemDefinition& ShopItemDefinition = ShopDefinition.Items[SlotIndex];
		FTunaSweeperShopItemView ShopItemView;
		ShopItemView.ShopId = ActiveShopId;
		ShopItemView.SlotIndex = SlotIndex;
		ShopItemView.ItemId = ShopItemDefinition.ItemId;
		ShopItemView.StockQuantity = GetShopStockQuantity(ActiveShopId, SlotIndex, ShopItemDefinition);
		ShopItemView.TotalStockQuantity = FMath::Max(0, ShopItemDefinition.StockQuantity);
		ShopItemView.Price = ItemDataSubsystem->ResolveShopItemBuyPrice(ShopItemDefinition);
		OutShopItems.Add(ShopItemView);
	}

	return OutShopItems.Num() > 0;
}

bool UTunaSweeperGameInstance::TryGetActiveShopItemView(
	int32 ShopSlotIndex,
	FTunaSweeperShopItemView& OutShopItem)
{
	EnsureInventoryStateInitialized();
	OutShopItem = FTunaSweeperShopItemView();

	if (!bHasActiveShop || ActiveShopId == INDEX_NONE || ShopSlotIndex == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopItemDefinition ShopItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetShopItemDefinition(ActiveShopId, ShopSlotIndex, ShopItemDefinition))
	{
		return false;
	}

	OutShopItem.ShopId = ActiveShopId;
	OutShopItem.SlotIndex = ShopSlotIndex;
	OutShopItem.ItemId = ShopItemDefinition.ItemId;
	OutShopItem.StockQuantity = GetShopStockQuantity(ActiveShopId, ShopSlotIndex, ShopItemDefinition);
	OutShopItem.TotalStockQuantity = FMath::Max(0, ShopItemDefinition.StockQuantity);
	OutShopItem.Price = ItemDataSubsystem->ResolveShopItemBuyPrice(ShopItemDefinition);
	return true;
}

bool UTunaSweeperGameInstance::TryBuyActiveShopSlot(int32 ShopSlotIndex)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperShopItemView ShopItemView;
	if (!TryGetActiveShopItemView(ShopSlotIndex, ShopItemView) ||
		ShopItemView.ItemId == INDEX_NONE ||
		ShopItemView.StockQuantity <= 0)
	{
		return false;
	}

	UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>();
	if (!QuestSubsystem || QuestSubsystem->GetCoinBalance() < ShopItemView.Price)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopItemDefinition ShopItemDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetShopItemDefinition(ShopItemView.ShopId, ShopItemView.SlotIndex, ShopItemDefinition))
	{
		return false;
	}

	if (!AddItemToFirstAvailableInventorySlot(ShopItemView.ItemId, 1))
	{
		return false;
	}

	SetShopStockQuantity(
		ShopItemView.ShopId,
		ShopItemView.SlotIndex,
		ShopItemDefinition,
		ShopItemView.StockQuantity - 1);

	if (ShopItemView.Price > 0)
	{
		QuestSubsystem->TrySpendCoins(ShopItemView.Price, false);
	}
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave(true);
	return true;
}

bool UTunaSweeperGameInstance::DebugRestockActiveShop(bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	if (!bHasActiveShop || ActiveShopId == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperShopDefinition ShopDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetShopDefinition(ActiveShopId, ShopDefinition))
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < ShopDefinition.Items.Num(); ++SlotIndex)
	{
		const FTunaSweeperShopItemDefinition& ShopItemDefinition = ShopDefinition.Items[SlotIndex];
		SetShopStockQuantity(
			ActiveShopId,
			SlotIndex,
			ShopItemDefinition,
			ShopItemDefinition.StockQuantity);
	}

	if (bSaveImmediately)
	{
		MarkItemStateMutationForSave(true);
	}
	BroadcastInventoryStateChanged();
	return true;
}

bool UTunaSweeperGameInstance::TryGetSlotSellPrice(
	const FTunaSweeperItemSlotReference& SlotReference,
	int32& OutSalePrice)
{
	EnsureInventoryStateInitialized();
	OutSalePrice = 0;

	if (!SlotReference.IsValid() || !IsSellableItemSlotSource(SlotReference.Source))
	{
		return false;
	}

	FTunaSweeperItemInstance ItemInstance;
	FTunaSweeperItemDefinition ItemDefinition;
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem ||
		!TryGetSlotItemInstance(SlotReference, ItemInstance) ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
	{
		return false;
	}

	OutSalePrice = FMath::Max(0, (FMath::Max(0, ItemDefinition.ShopSellPrice) * FMath::Max(1, ItemInstance.Quantity)) / 2);
	return true;
}

bool UTunaSweeperGameInstance::TrySellItemInSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	int32& OutSalePrice)
{
	EnsureInventoryStateInitialized();
	OutSalePrice = 0;

	if (!bHasActiveShop || !TryGetSlotSellPrice(SlotReference, OutSalePrice))
	{
		return false;
	}

	FTunaSweeperItemInstance RemovedItemInstance;
	if (!RemoveItemFromSlot(SlotReference, RemovedItemInstance))
	{
		return false;
	}

	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		if (OutSalePrice > 0)
		{
			QuestSubsystem->AddCoins(OutSalePrice, false);
		}
	}

	MarkItemStateMutationForSave(true);
	ClearSelectedItemSelection();
	ClearHoveredItemSlot(SlotReference);
	return true;
}

int32 UTunaSweeperGameInstance::GetShopStockQuantity(
	int32 ShopId,
	int32 SlotIndex,
	const FTunaSweeperShopItemDefinition& ShopItemDefinition) const
{
	if (!TunaSweeperShop::IsValidShopSlotKey(ShopId, SlotIndex, ShopItemDefinition.ItemId))
	{
		return 0;
	}

	const FName StockKey = TunaSweeperShop::MakeStockKey(ShopId, SlotIndex, ShopItemDefinition.ItemId);
	if (const FTunaSweeperShopStockSaveData* SavedStockState = ShopStockStatesByKey.Find(StockKey))
	{
		return FMath::Clamp(SavedStockState->StockQuantity, 0, FMath::Max(0, ShopItemDefinition.StockQuantity));
	}

	return FMath::Max(0, ShopItemDefinition.StockQuantity);
}

void UTunaSweeperGameInstance::SetShopStockQuantity(
	int32 ShopId,
	int32 SlotIndex,
	const FTunaSweeperShopItemDefinition& ShopItemDefinition,
	int32 StockQuantity)
{
	if (!TunaSweeperShop::IsValidShopSlotKey(ShopId, SlotIndex, ShopItemDefinition.ItemId))
	{
		return;
	}

	const FName StockKey = TunaSweeperShop::MakeStockKey(ShopId, SlotIndex, ShopItemDefinition.ItemId);
	FTunaSweeperShopStockSaveData StockState;
	StockState.ShopId = ShopId;
	StockState.SlotIndex = SlotIndex;
	StockState.ItemId = ShopItemDefinition.ItemId;
	StockState.StockQuantity = FMath::Clamp(StockQuantity, 0, FMath::Max(0, ShopItemDefinition.StockQuantity));
	ShopStockStatesByKey.Add(StockKey, StockState);
}

