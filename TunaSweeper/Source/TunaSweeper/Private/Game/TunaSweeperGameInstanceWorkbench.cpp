#include "TunaSweeperGameInstanceShared.h"

void UTunaSweeperGameInstance::SetActiveWorkbench(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	EnsureInventoryStateInitialized();

	if (WorkbenchId <= 0)
	{
		ClearActiveWorkbench();
		return;
	}

	ActiveWorkbenchId = WorkbenchId;
	ActiveWorkbenchMode = WorkbenchMode;
	bHasActiveWorkbench = true;
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::SetActiveWorkbenchMode(ETunaSweeperWorkbenchMode WorkbenchMode)
{
	if (!bHasActiveWorkbench)
	{
		return;
	}

	ActiveWorkbenchMode = WorkbenchMode;
	BroadcastInventoryStateChanged();
}

void UTunaSweeperGameInstance::ClearActiveWorkbench()
{
	const bool bHadActiveWorkbench = bHasActiveWorkbench || ActiveWorkbenchId != INDEX_NONE;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;

	if (bHadActiveWorkbench)
	{
		BroadcastInventoryStateChanged();
	}
}

bool UTunaSweeperGameInstance::GetActiveWorkbenchRecipes(TArray<FTunaSweeperWorkbenchRecipeView>& OutRecipeViews)
{
	EnsureInventoryStateInitialized();
	OutRecipeViews.Reset();

	if (!bHasActiveWorkbench || ActiveWorkbenchId == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperWorkbenchRecipeDefinition> RecipeDefinitions;
	if (!ItemDataSubsystem || !ItemDataSubsystem->GetWorkbenchRecipeDefinitions(ActiveWorkbenchId, RecipeDefinitions))
	{
		return false;
	}

	OutRecipeViews.Reserve(RecipeDefinitions.Num());
	for (const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition : RecipeDefinitions)
	{
		if (IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition))
		{
			OutRecipeViews.Add(BuildWorkbenchRecipeView(RecipeDefinition, OutRecipeViews.Num()));
		}
	}

	return OutRecipeViews.Num() > 0;
}

bool UTunaSweeperGameInstance::TryGetActiveWorkbenchRecipeView(
	int32 RecipeSlotIndex,
	FTunaSweeperWorkbenchRecipeView& OutRecipeView)
{
	EnsureInventoryStateInitialized();
	OutRecipeView = FTunaSweeperWorkbenchRecipeView();

	if (!bHasActiveWorkbench || ActiveWorkbenchId == INDEX_NONE || RecipeSlotIndex == INDEX_NONE)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperWorkbenchRecipeDefinition> RecipeDefinitions;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->GetWorkbenchRecipeDefinitions(ActiveWorkbenchId, RecipeDefinitions))
	{
		return false;
	}

	TArray<FTunaSweeperWorkbenchRecipeDefinition> UnlockedRecipeDefinitions;
	for (const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition : RecipeDefinitions)
	{
		if (IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition))
		{
			UnlockedRecipeDefinitions.Add(RecipeDefinition);
		}
	}
	if (!UnlockedRecipeDefinitions.IsValidIndex(RecipeSlotIndex))
	{
		return false;
	}

	OutRecipeView = BuildWorkbenchRecipeView(UnlockedRecipeDefinitions[RecipeSlotIndex], RecipeSlotIndex);
	return true;
}

bool UTunaSweeperGameInstance::CanCraftActiveWorkbenchRecipe(int32 RecipeSlotIndex)
{
	FTunaSweeperWorkbenchRecipeView RecipeView;
	return TryGetActiveWorkbenchRecipeView(RecipeSlotIndex, RecipeView) && RecipeView.bCanCraft;
}

bool UTunaSweeperGameInstance::TryCraftActiveWorkbenchRecipe(int32 RecipeSlotIndex, bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperWorkbenchRecipeDefinition> RecipeDefinitions;
	if (!bHasActiveWorkbench ||
		ActiveWorkbenchId == INDEX_NONE ||
		RecipeSlotIndex == INDEX_NONE ||
		!ItemDataSubsystem ||
		!ItemDataSubsystem->GetWorkbenchRecipeDefinitions(ActiveWorkbenchId, RecipeDefinitions))
	{
		return false;
	}

	TArray<FTunaSweeperWorkbenchRecipeDefinition> UnlockedRecipeDefinitions;
	for (const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition : RecipeDefinitions)
	{
		if (IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition))
		{
			UnlockedRecipeDefinitions.Add(RecipeDefinition);
		}
	}
	if (!UnlockedRecipeDefinitions.IsValidIndex(RecipeSlotIndex))
	{
		return false;
	}

	const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition = UnlockedRecipeDefinitions[RecipeSlotIndex];
	const FTunaSweeperWorkbenchRecipeView RecipeView = BuildWorkbenchRecipeView(RecipeDefinition, RecipeSlotIndex);
	if (!RecipeView.bCanCraft || RecipeDefinition.OutputItemId == INDEX_NONE || RecipeDefinition.OutputQuantity <= 0)
	{
		return false;
	}

	const TMap<FGuid, FTunaSweeperItemInstance> PreviousItemInstances = ItemInstancesByUid;
	const TArray<FTunaSweeperInventorySlot> PreviousInventorySlots = PlayerInventorySlots;
	const TArray<FTunaSweeperInventorySlot> PreviousAuxiliaryBagSlots = AuxiliaryBagSlots;
	const TArray<FTunaSweeperInventorySlot> PreviousStorageSlots = StorageSlots;

	for (const FTunaSweeperWorkbenchIngredient& Ingredient : RecipeDefinition.Ingredients)
	{
		if (ConsumeWorkbenchIngredientItemById(Ingredient.ItemId, Ingredient.Quantity) < Ingredient.Quantity)
		{
			ItemInstancesByUid = PreviousItemInstances;
			PlayerInventorySlots = PreviousInventorySlots;
			AuxiliaryBagSlots = PreviousAuxiliaryBagSlots;
			StorageSlots = PreviousStorageSlots;
			return false;
		}
	}

	int32 RemainingOutputQuantity = RecipeDefinition.OutputQuantity;
	TryAddItemQuantityToExistingStacks(RecipeDefinition.OutputItemId, RemainingOutputQuantity, PlayerInventorySlots);
	TryAddItemQuantityToFirstEmptySlots(RecipeDefinition.OutputItemId, RemainingOutputQuantity, PlayerInventorySlots);
	if (RemainingOutputQuantity > 0)
	{
		ItemInstancesByUid = PreviousItemInstances;
		PlayerInventorySlots = PreviousInventorySlots;
		AuxiliaryBagSlots = PreviousAuxiliaryBagSlots;
		StorageSlots = PreviousStorageSlots;
		return false;
	}

	ClearSelectedItemIfInvalid();
	BroadcastInventoryStateChanged();
	MarkItemEverAcquired(RecipeDefinition.OutputItemId);
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->NotifyItemAcquired(
			RecipeDefinition.OutputItemId,
			RecipeDefinition.OutputQuantity,
			!IsCurrentWorldBunkerMap());
	}
	AddRaidExperienceForItem(RecipeDefinition.OutputItemId, RecipeDefinition.OutputQuantity);
	MarkItemStateMutationForSave(bSaveImmediately);
	return true;
}

bool UTunaSweeperGameInstance::GetActiveWorkbenchDismantleCandidates(
	TArray<FTunaSweeperWorkbenchDismantleCandidateView>& OutCandidateViews)
{
	EnsureInventoryStateInitialized();
	OutCandidateViews.Reset();
	if (!bHasActiveWorkbench)
	{
		return false;
	}

	auto AddCandidatesFromSlots = [this, &OutCandidateViews](ETunaSweeperItemSlotSource Source, const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			FTunaSweeperItemSlotReference SlotReference;
			SlotReference.Source = Source;
			SlotReference.SlotIndex = SlotIndex;

			FTunaSweeperWorkbenchDismantleCandidateView CandidateView;
			if (TryGetWorkbenchDismantleCandidateFromSlot(SlotReference, CandidateView))
			{
				CandidateView.ListIndex = OutCandidateViews.Num();
				OutCandidateViews.Add(CandidateView);
			}
		}
	};

	AddCandidatesFromSlots(ETunaSweeperItemSlotSource::Inventory, PlayerInventorySlots);
	AddCandidatesFromSlots(ETunaSweeperItemSlotSource::Storage, StorageSlots);
	return OutCandidateViews.Num() > 0;
}

bool UTunaSweeperGameInstance::TryGetWorkbenchDismantleCandidateFromSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	FTunaSweeperWorkbenchDismantleCandidateView& OutCandidateView)
{
	EnsureInventoryStateInitialized();
	OutCandidateView = FTunaSweeperWorkbenchDismantleCandidateView();
	if (!bHasActiveWorkbench ||
		!SlotReference.IsValid() ||
		!IsWorkbenchItemSlotSourceAllowedForDismantle(SlotReference.Source))
	{
		return false;
	}

	FTunaSweeperItemInstance ItemInstance;
	if (!TryGetSlotItemInstance(SlotReference, ItemInstance))
	{
		return false;
	}

	OutCandidateView.SlotReference = SlotReference;
	OutCandidateView.ItemId = ItemInstance.ItemId;
	OutCandidateView.Quantity = FMath::Max(1, ItemInstance.Quantity);

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperWorkbenchDismantleDefinition DismantleDefinition;
	if (ItemDataSubsystem &&
		ItemDataSubsystem->TryGetWorkbenchDismantleDefinition(ItemInstance.ItemId, DismantleDefinition))
	{
		OutCandidateView.Results = DismantleDefinition.Results;
		OutCandidateView.bCanDismantle = OutCandidateView.Results.Num() > 0;
	}

	return true;
}

bool UTunaSweeperGameInstance::TryDismantleWorkbenchItemInSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	TArray<FTunaSweeperItemStack>& OutOverflowItems,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	OutOverflowItems.Reset();

	FTunaSweeperWorkbenchDismantleCandidateView CandidateView;
	if (!TryGetWorkbenchDismantleCandidateFromSlot(SlotReference, CandidateView) || !CandidateView.bCanDismantle)
	{
		return false;
	}

	if (!TryConsumeSingleItemFromSlot(SlotReference))
	{
		return false;
	}

	for (const FTunaSweeperItemStack& ResultStack : CandidateView.Results)
	{
		AddWorkbenchResultToInventoryOrOverflow(ResultStack.ItemId, ResultStack.Quantity, OutOverflowItems);
	}

	ClearSelectedItemIfInvalid();
	ClearHoveredItemSlot(SlotReference);
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave(bSaveImmediately);
	return true;
}

bool UTunaSweeperGameInstance::GetActiveWorkbenchBlueprintItems(
	TArray<FTunaSweeperWorkbenchBlueprintItemView>& OutBlueprintItemViews)
{
	EnsureInventoryStateInitialized();
	OutBlueprintItemViews.Reset();
	if (!bHasActiveWorkbench)
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem)
	{
		return false;
	}

	auto AddBlueprintsFromSlots = [this, ItemDataSubsystem, &OutBlueprintItemViews](ETunaSweeperItemSlotSource Source, const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			FTunaSweeperItemSlotReference SlotReference;
			SlotReference.Source = Source;
			SlotReference.SlotIndex = SlotIndex;

			FTunaSweeperItemInstance ItemInstance;
			FTunaSweeperItemDefinition ItemDefinition;
			if (!TryGetSlotItemInstance(SlotReference, ItemInstance) ||
				!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition) ||
				!IsWorkbenchBlueprintItemDefinition(ItemDefinition))
			{
				continue;
			}

			FTunaSweeperWorkbenchBlueprintItemView BlueprintItemView;
			BlueprintItemView.SlotReference = SlotReference;
			BlueprintItemView.ListIndex = OutBlueprintItemViews.Num();
			BlueprintItemView.ItemId = ItemInstance.ItemId;
			BlueprintItemView.Quantity = FMath::Max(1, ItemInstance.Quantity);
			BlueprintItemView.RecipeId = ItemDefinition.BlueprintRecipeId;
			BlueprintItemView.bAlreadyUnlocked = IsWorkbenchRecipeUnlocked(BlueprintItemView.RecipeId);

			FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
			BlueprintItemView.bRecipeKnown =
				!BlueprintItemView.RecipeId.IsNone() &&
				ItemDataSubsystem->TryGetWorkbenchRecipeDefinition(BlueprintItemView.RecipeId, RecipeDefinition);
			BlueprintItemView.bCanRegister =
				BlueprintItemView.bRecipeKnown &&
				!BlueprintItemView.bAlreadyUnlocked;
			OutBlueprintItemViews.Add(BlueprintItemView);
		}
	};

	AddBlueprintsFromSlots(ETunaSweeperItemSlotSource::Inventory, PlayerInventorySlots);
	AddBlueprintsFromSlots(ETunaSweeperItemSlotSource::Storage, StorageSlots);
	return OutBlueprintItemViews.Num() > 0;
}

bool UTunaSweeperGameInstance::TryRegisterWorkbenchBlueprintFromSlot(
	const FTunaSweeperItemSlotReference& SlotReference,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	if (!bHasActiveWorkbench ||
		!SlotReference.IsValid() ||
		!IsWorkbenchItemSlotSourceAllowedForBlueprintRegister(SlotReference.Source))
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!ItemDataSubsystem)
	{
		return false;
	}

	FTunaSweeperItemInstance ItemInstance;
	FTunaSweeperItemDefinition ItemDefinition;
	if (!TryGetSlotItemInstance(SlotReference, ItemInstance) ||
		!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition) ||
		!IsWorkbenchBlueprintItemDefinition(ItemDefinition) ||
		ItemDefinition.BlueprintRecipeId.IsNone() ||
		IsWorkbenchRecipeUnlocked(ItemDefinition.BlueprintRecipeId))
	{
		return false;
	}

	FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetWorkbenchRecipeDefinition(ItemDefinition.BlueprintRecipeId, RecipeDefinition))
	{
		return false;
	}

	if (!TryConsumeSingleItemFromSlot(SlotReference))
	{
		return false;
	}

	if (!UnlockWorkbenchRecipe(ItemDefinition.BlueprintRecipeId, false))
	{
		return false;
	}

	ClearSelectedItemIfInvalid();
	ClearHoveredItemSlot(SlotReference);
	BroadcastInventoryStateChanged();
	MarkItemStateMutationForSave(bSaveImmediately);
	return true;
}

bool UTunaSweeperGameInstance::IsWorkbenchRecipeUnlocked(FName RecipeId) const
{
	if (RecipeId.IsNone())
	{
		return false;
	}

	const UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
	if (!ItemDataSubsystem || !const_cast<UTunaSweeperItemDataSubsystem*>(ItemDataSubsystem)->TryGetWorkbenchRecipeDefinition(RecipeId, RecipeDefinition))
	{
		return false;
	}

	return IsWorkbenchRecipeDefinitionUnlocked(RecipeDefinition);
}

bool UTunaSweeperGameInstance::UnlockWorkbenchRecipe(FName RecipeId, bool bSaveImmediately)
{
	if (RecipeId.IsNone())
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperWorkbenchRecipeDefinition RecipeDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetWorkbenchRecipeDefinition(RecipeId, RecipeDefinition))
	{
		return false;
	}

	if (RecipeDefinition.bAutoUnlocked)
	{
		return true;
	}

	const int32 PreviousCount = UnlockedWorkbenchRecipeIds.Num();
	UnlockedWorkbenchRecipeIds.Add(RecipeId);
	const bool bChanged = UnlockedWorkbenchRecipeIds.Num() != PreviousCount;
	if (bChanged && bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return true;
}

void UTunaSweeperGameInstance::GetUnlockedWorkbenchRecipeIds(TArray<FName>& OutRecipeIds) const
{
	OutRecipeIds = UnlockedWorkbenchRecipeIds.Array();
	OutRecipeIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
}

FTunaSweeperWorkbenchRecipeView UTunaSweeperGameInstance::BuildWorkbenchRecipeView(
	const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition,
	int32 RecipeSlotIndex) const
{
	FTunaSweeperWorkbenchRecipeView RecipeView;
	RecipeView.RecipeId = RecipeDefinition.RecipeId;
	RecipeView.WorkbenchId = RecipeDefinition.WorkbenchId;
	RecipeView.SlotIndex = RecipeSlotIndex;
	RecipeView.OutputItemId = RecipeDefinition.OutputItemId;
	RecipeView.OutputQuantity = FMath::Max(1, RecipeDefinition.OutputQuantity);

	for (const FTunaSweeperWorkbenchIngredient& Ingredient : RecipeDefinition.Ingredients)
	{
		FTunaSweeperWorkbenchIngredientView IngredientView;
		IngredientView.ItemId = Ingredient.ItemId;
		IngredientView.RequiredQuantity = FMath::Max(1, Ingredient.Quantity);
		IngredientView.AvailableQuantity = CountWorkbenchIngredientItemById(Ingredient.ItemId);
		IngredientView.MissingQuantity = FMath::Max(0, IngredientView.RequiredQuantity - IngredientView.AvailableQuantity);
		if (IngredientView.MissingQuantity > 0)
		{
			++RecipeView.MissingIngredientCount;
		}
		RecipeView.Ingredients.Add(IngredientView);
	}

	RecipeView.bCanCraft =
		RecipeView.OutputItemId != INDEX_NONE &&
		RecipeView.OutputQuantity > 0 &&
		RecipeView.Ingredients.Num() > 0 &&
		RecipeView.MissingIngredientCount <= 0;
	return RecipeView;
}

bool UTunaSweeperGameInstance::IsWorkbenchRecipeDefinitionUnlocked(
	const FTunaSweeperWorkbenchRecipeDefinition& RecipeDefinition) const
{
	return RecipeDefinition.bAutoUnlocked ||
		(!RecipeDefinition.RecipeId.IsNone() && UnlockedWorkbenchRecipeIds.Contains(RecipeDefinition.RecipeId));
}

bool UTunaSweeperGameInstance::IsWorkbenchBlueprintItemDefinition(
	const FTunaSweeperItemDefinition& ItemDefinition) const
{
	static const FName BlueprintCategoryTag(TEXT("item.category.blueprint"));
	return ItemDefinition.CategoryTag == BlueprintCategoryTag || !ItemDefinition.BlueprintRecipeId.IsNone();
}

bool UTunaSweeperGameInstance::IsWorkbenchItemSlotSourceAllowedForDismantle(ETunaSweeperItemSlotSource Source) const
{
	return Source == ETunaSweeperItemSlotSource::Inventory ||
		Source == ETunaSweeperItemSlotSource::Storage;
}

bool UTunaSweeperGameInstance::IsWorkbenchItemSlotSourceAllowedForBlueprintRegister(ETunaSweeperItemSlotSource Source) const
{
	return Source == ETunaSweeperItemSlotSource::Inventory ||
		Source == ETunaSweeperItemSlotSource::Storage;
}

bool UTunaSweeperGameInstance::TryConsumeSingleItemFromSlot(const FTunaSweeperItemSlotReference& SlotReference)
{
	if (!SlotReference.IsValid())
	{
		return false;
	}

	TArray<FTunaSweeperInventorySlot>* Slots = GetMutableSlotsForSource(SlotReference.Source);
	if (!Slots || !Slots->IsValidIndex(SlotReference.SlotIndex))
	{
		return false;
	}

	FTunaSweeperInventorySlot& Slot = (*Slots)[SlotReference.SlotIndex];
	FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
	if (!ItemInstance || !ItemInstance->IsValid())
	{
		return false;
	}

	if (ItemInstance->Quantity > 1)
	{
		--ItemInstance->Quantity;
		return true;
	}

	TFunction<void(const FGuid&)> RemoveItemUid = [this, &RemoveItemUid](const FGuid& Uid)
	{
		if (!Uid.IsValid())
		{
			return;
		}

		TArray<FGuid> AttachmentUids;
		if (const FTunaSweeperItemInstance* RemovedItemInstance = ItemInstancesByUid.Find(Uid))
		{
			for (const TPair<FName, FGuid>& AttachmentSlot : RemovedItemInstance->AttachmentSlots)
			{
				AttachmentUids.Add(AttachmentSlot.Value);
			}
		}

		ItemInstancesByUid.Remove(Uid);
		for (const FGuid& AttachmentUid : AttachmentUids)
		{
			RemoveItemUid(AttachmentUid);
		}
	};

	RemoveItemUid(Slot.ItemUid);
	Slot.Clear();
	return true;
}

bool UTunaSweeperGameInstance::AddWorkbenchResultToInventoryOrOverflow(
	int32 ItemId,
	int32 Quantity,
	TArray<FTunaSweeperItemStack>& InOutOverflowItems)
{
	if (ItemId == INDEX_NONE || Quantity <= 0)
	{
		return false;
	}

	int32 RemainingQuantity = Quantity;
	TryAddItemQuantityToExistingStacks(ItemId, RemainingQuantity, PlayerInventorySlots);
	TryAddItemQuantityToFirstEmptySlots(ItemId, RemainingQuantity, PlayerInventorySlots);
	if (RemainingQuantity > 0)
	{
		FTunaSweeperItemStack* ExistingOverflow = InOutOverflowItems.FindByPredicate(
			[ItemId](const FTunaSweeperItemStack& Candidate)
			{
				return Candidate.ItemId == ItemId;
			});
		if (ExistingOverflow)
		{
			ExistingOverflow->Quantity += RemainingQuantity;
		}
		else
		{
			FTunaSweeperItemStack OverflowStack;
			OverflowStack.ItemId = ItemId;
			OverflowStack.Quantity = RemainingQuantity;
			InOutOverflowItems.Add(OverflowStack);
		}
	}

	const int32 AddedQuantity = Quantity - RemainingQuantity;
	if (AddedQuantity > 0)
	{
		MarkItemEverAcquired(ItemId);
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyItemAcquired(ItemId, AddedQuantity, !IsCurrentWorldBunkerMap());
		}
		AddRaidExperienceForItem(ItemId, AddedQuantity);
	}
	return true;
}

int32 UTunaSweeperGameInstance::CountWorkbenchIngredientItemById(int32 ItemId) const
{
	if (ItemId == INDEX_NONE)
	{
		return 0;
	}

	int32 ItemCount = 0;
	auto CountInSlots = [this, ItemId, &ItemCount](const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (const FTunaSweeperInventorySlot& Slot : Slots)
		{
			const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
			if (ItemInstance && ItemInstance->ItemId == ItemId)
			{
				ItemCount += FMath::Max(0, ItemInstance->Quantity);
			}
		}
	};

	CountInSlots(PlayerInventorySlots);
	CountInSlots(AuxiliaryBagSlots);
	CountInSlots(StorageSlots);
	return ItemCount;
}

int32 UTunaSweeperGameInstance::ConsumeWorkbenchIngredientItemById(int32 ItemId, int32 RequestedAmount)
{
	if (ItemId == INDEX_NONE || RequestedAmount <= 0)
	{
		return 0;
	}

	int32 RemainingAmount = RequestedAmount;
	auto ConsumeInSlots = [this, ItemId, &RemainingAmount](TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (FTunaSweeperInventorySlot& Slot : Slots)
		{
			if (RemainingAmount <= 0)
			{
				break;
			}

			FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(Slot.ItemUid);
			if (!ItemInstance || ItemInstance->ItemId != ItemId)
			{
				continue;
			}

			const int32 ConsumedAmount = FMath::Min(RemainingAmount, FMath::Max(0, ItemInstance->Quantity));
			ItemInstance->Quantity -= ConsumedAmount;
			RemainingAmount -= ConsumedAmount;

			if (ItemInstance->Quantity <= 0)
			{
				ItemInstancesByUid.Remove(Slot.ItemUid);
				Slot.Clear();
			}
		}
	};

	ConsumeInSlots(PlayerInventorySlots);
	ConsumeInSlots(AuxiliaryBagSlots);
	ConsumeInSlots(StorageSlots);
	return RequestedAmount - RemainingAmount;
}

