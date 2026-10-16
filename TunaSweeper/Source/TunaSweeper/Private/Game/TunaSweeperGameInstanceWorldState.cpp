#include "TunaSweeperGameInstanceShared.h"

bool UTunaSweeperGameInstance::CommitPowerFacilityRepair(FName ObjectId,
	const TArray<FTunaSweeperItemStack>& Requirements, bool bConsumeItems, bool bSaveImmediately)
{
	return CommitPowerFacilityRepairs({ObjectId}, Requirements, bConsumeItems, bSaveImmediately);
}

bool UTunaSweeperGameInstance::CommitPowerFacilityRepairs(const TArray<FName>& ObjectIds,
	const TArray<FTunaSweeperItemStack>& Requirements, bool bConsumeItems, bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	TSet<FName> PendingIds;
	for (const FName ObjectId : ObjectIds)
	{
		if (ObjectId.IsNone()) return false;
		const auto* Existing = WorldProgressStatesById.Find(ObjectId);
		if (!Existing || Existing->State != ETunaSweeperWorldProgressState::Completed) PendingIds.Add(ObjectId);
	}
	if (PendingIds.IsEmpty()) return false;
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
		const int32 Owned = bConsumeItems ? CountInventoryItemById(Total.Key) : CountCarriedItemById(Total.Key);
		if (Owned < Total.Value) return false;
	}
	const auto PreviousItems = ItemInstancesByUid;
	const auto PreviousInventory = PlayerInventorySlots;
	const auto PreviousBags = AuxiliaryBagSlots;
	const auto PreviousProgress = WorldProgressStatesById;
	const auto Rollback = [&]()
	{
		ItemInstancesByUid = PreviousItems;
		PlayerInventorySlots = PreviousInventory;
		AuxiliaryBagSlots = PreviousBags;
		WorldProgressStatesById = PreviousProgress;
	};
	if (bConsumeItems)
	{
		for (const auto& Total : Totals)
		{
			if (ConsumeInventoryAmmoByItemId(Total.Key, Total.Value) != Total.Value)
			{
				Rollback();
				return false;
			}
		}
	}
	for (const FName ObjectId : PendingIds)
	{
		FTunaSweeperWorldProgressSaveData State;
		State.ObjectId = ObjectId;
		State.InfoId = TEXT("world_progress.power_facility");
		State.State = ETunaSweeperWorldProgressState::Completed;
		State.ProgressQuantity = 1;
		WorldProgressStatesById.Add(ObjectId, State);
	}
	if (bSaveImmediately && !SaveGameStateInternal())
	{
		Rollback();
		return false;
	}
	ClearSelectedItemIfInvalid();
	// Quest reconciliation and observers only see a completion after the durable transaction succeeds.
	OnWorldProgressChanged.Broadcast();
	if (bConsumeItems && !Totals.IsEmpty()) BroadcastInventoryStateChanged();
	return true;
}

FTunaSweeperWorldProgressSaveData UTunaSweeperGameInstance::GetOrCreateWorldProgressState(
	FName ObjectId,
	FName InfoId,
	int32 InitialProgressQuantity,
	int32 RequiredQuantity)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperWorldProgressSaveData EmptyState;
	if (ObjectId.IsNone())
	{
		return EmptyState;
	}

	FTunaSweeperWorldProgressSaveData* ExistingState = WorldProgressStatesById.Find(ObjectId);
	if (!ExistingState)
	{
		FTunaSweeperWorldProgressSaveData NewState;
		NewState.ObjectId = ObjectId;
		NewState.InfoId = InfoId;
		NewState.State = ETunaSweeperWorldProgressState::InProgress;
		NewState.ProgressQuantity = FMath::Clamp(
			InitialProgressQuantity,
			0,
			FMath::Max(0, RequiredQuantity));
		ExistingState = &WorldProgressStatesById.Add(ObjectId, NewState);
	}

	if (!InfoId.IsNone())
	{
		ExistingState->InfoId = InfoId;
	}

	ExistingState->ProgressQuantity = FMath::Clamp(
		ExistingState->ProgressQuantity,
		0,
		FMath::Max(0, RequiredQuantity));
	if (ExistingState->State == ETunaSweeperWorldProgressState::Completed)
	{
		ExistingState->ProgressQuantity = FMath::Max(ExistingState->ProgressQuantity, FMath::Max(0, RequiredQuantity));
	}

	return *ExistingState;
}

bool UTunaSweeperGameInstance::TryGetWorldProgressState(
	FName ObjectId,
	FTunaSweeperWorldProgressSaveData& OutState) const
{
	if (const FTunaSweeperWorldProgressSaveData* FoundState = WorldProgressStatesById.Find(ObjectId))
	{
		OutState = *FoundState;
		return true;
	}

	OutState = FTunaSweeperWorldProgressSaveData();
	return false;
}

bool UTunaSweeperGameInstance::UpdateWorldProgressState(
	FName ObjectId,
	FName InfoId,
	ETunaSweeperWorldProgressState State,
	int32 ProgressQuantity,
	int32 RequiredQuantity,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	if (ObjectId.IsNone())
	{
		return false;
	}

	FTunaSweeperWorldProgressSaveData NewState;
	NewState.ObjectId = ObjectId;
	NewState.InfoId = InfoId;
	NewState.State = State;
	NewState.ProgressQuantity = FMath::Clamp(
		ProgressQuantity,
		0,
		FMath::Max(0, RequiredQuantity));
	if (NewState.State == ETunaSweeperWorldProgressState::Completed)
	{
		NewState.ProgressQuantity = FMath::Max(NewState.ProgressQuantity, FMath::Max(0, RequiredQuantity));
	}

	WorldProgressStatesById.Add(ObjectId, NewState);
	OnWorldProgressChanged.Broadcast();
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return true;
}

int32 UTunaSweeperGameInstance::GetPiggyBankStoredAncientCoinValue(FName PiggyBankId) const
{
	if (PiggyBankId.IsNone())
	{
		return 0;
	}

	const FTunaSweeperPiggyBankSaveData* FoundState = PiggyBankStatesById.Find(PiggyBankId);
	return FoundState ? FMath::Max(0, FoundState->StoredAncientCoinValue) : 0;
}

bool UTunaSweeperGameInstance::AddPiggyBankStoredAncientCoinValue(
	FName PiggyBankId,
	int32 CoinValueDelta,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	if (PiggyBankId.IsNone() || CoinValueDelta <= 0)
	{
		return false;
	}

	FTunaSweeperPiggyBankSaveData& State = PiggyBankStatesById.FindOrAdd(PiggyBankId);
	State.PiggyBankId = PiggyBankId;
	State.StoredAncientCoinValue = FMath::Max(0, State.StoredAncientCoinValue) + CoinValueDelta;

	if (bSaveImmediately)
	{
		SaveGameState();
	}
	return true;
}

void UTunaSweeperGameInstance::GetHousingFacilities(
	TArray<FTunaSweeperHousingPlacedFacilitySaveData>& OutFacilities)
{
	EnsureInventoryStateInitialized();
	OutFacilities = HousingFacilities;
}

void UTunaSweeperGameInstance::SetHousingFacilities(
	const TArray<FTunaSweeperHousingPlacedFacilitySaveData>& InFacilities,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();
	HousingFacilities.Reset();
	TSet<FGuid> SeenInstanceIds;
	for (FTunaSweeperHousingPlacedFacilitySaveData HousingFacility : InFacilities)
	{
		if (!HousingFacility.IsValid() || SeenInstanceIds.Contains(HousingFacility.InstanceId))
		{
			continue;
		}

		HousingFacility.RotationQuarterTurns = FMath::Clamp(HousingFacility.RotationQuarterTurns, 0, 3);
		HousingFacilities.Add(HousingFacility);
		SeenInstanceIds.Add(HousingFacility.InstanceId);
	}

	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
}

bool UTunaSweeperGameInstance::IsHousingFacilityUnlocked(FName FacilityId)
{
	EnsureInventoryStateInitialized();
	return !FacilityId.IsNone() && UnlockedHousingFacilityIds.Contains(FacilityId);
}

bool UTunaSweeperGameInstance::UnlockHousingFacility(FName FacilityId, bool bSaveImmediately)
{
	if (FacilityId.IsNone())
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	const int32 PreviousCount = UnlockedHousingFacilityIds.Num();
	UnlockedHousingFacilityIds.Add(FacilityId);
	const bool bChanged = UnlockedHousingFacilityIds.Num() != PreviousCount;
	if (bChanged && bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return bChanged;
}

void UTunaSweeperGameInstance::GetUnlockedHousingFacilityIds(TArray<FName>& OutFacilityIds)
{
	EnsureInventoryStateInitialized();
	OutFacilityIds = UnlockedHousingFacilityIds.Array();
	OutFacilityIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
}
