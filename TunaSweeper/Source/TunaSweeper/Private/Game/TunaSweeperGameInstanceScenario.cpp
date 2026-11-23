#include "TunaSweeperGameInstanceShared.h"
#include "Memo/TunaSweeperMemoTypes.h"
#include "Settings/TunaSweeperBuildFlavor.h"

bool UTunaSweeperGameInstance::IsScenarioProgressFlagSet(FName ScenarioFlag) const
{
	return !ScenarioFlag.IsNone() && CompletedScenarioFlags.Contains(ScenarioFlag);
}

void UTunaSweeperGameInstance::MarkScenarioProgressFlag(FName ScenarioFlag, bool bSaveImmediately)
{
	if (ScenarioFlag.IsNone())
	{
		return;
	}

	EnsureInventoryStateInitialized();
	CompletedScenarioFlags.Add(ScenarioFlag);
	if (UTunaSweeperQuestSubsystem* Quests = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		Quests->ReconcileStoryQuestAcceptance(CompletedScenarioFlags);
	}

	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
}

bool UTunaSweeperGameInstance::TrySaveGateUnlock(FName UnlockFlag)
{
	if (UnlockFlag.IsNone() || IsCombatTestSession())
	{
		return false;
	}
	EnsureInventoryStateInitialized();
	if (CompletedScenarioFlags.Contains(UnlockFlag))
	{
		return true;
	}

	CompletedScenarioFlags.Add(UnlockFlag);
	if (!SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime))
	{
		CompletedScenarioFlags.Remove(UnlockFlag);
		return false;
	}
	return true;
}

bool UTunaSweeperGameInstance::TrySaveGateReset(FName UnlockFlag)
{
	// Never initialize a user save merely because an external helper is polling/resetting.
	if (UnlockFlag.IsNone() || !IsGateCheatSaveReady() || IsCombatTestSession()) return false;
	const bool bHadFlag = CompletedScenarioFlags.Remove(UnlockFlag) > 0;
	if (!SaveGameStateInternal(EUsableQuickSlotSaveMode::PersistRuntime))
	{
		if (bHadFlag) CompletedScenarioFlags.Add(UnlockFlag);
		return false;
	}
	return true;
}

FName UTunaSweeperGameInstance::ResolveInitialGameplayLevelName()
{
	EnsureInventoryStateInitialized();
	if (!bActiveSaveSlotDifficultySelected)
	{
		return FName(TEXT("IntroMap"));
	}

	const FName DemoLevel = TunaSweeperScenario::BunkerMapName;
	if (!TunaSweeperBuildFlavor::IsDemo() && IsScenarioProgressFlagSet(TunaSweeperScenario::OpeningScenarioFlag))
	{
		return TunaSweeperScenario::BunkerMapName;
	}
	return TunaSweeperBuildFlavor::ResolveInitialGameplayLevel(
		DemoLevel,
		TunaSweeperScenario::BunkerMapName);
}

void UTunaSweeperGameInstance::BeginScenarioBunkerEntry(FName ScenarioFlag)
{
	PendingScenarioCompletionFlag = ScenarioFlag;
	bPendingScenarioBunkerEntryPresentation = true;
}

bool UTunaSweeperGameInstance::CompletePendingScenarioBunkerEntryIfNeeded()
{
	if (!bPendingScenarioBunkerEntryPresentation)
	{
		return false;
	}

	const FName ScenarioFlag = PendingScenarioCompletionFlag;
	PendingScenarioCompletionFlag = NAME_None;
	bPendingScenarioBunkerEntryPresentation = false;
	if (!ScenarioFlag.IsNone())
	{
		MarkScenarioProgressFlag(ScenarioFlag, true);
	}
	return true;
}

bool UTunaSweeperGameInstance::IsMemoAcquired(int32 MemoId)
{
	EnsureInventoryStateInitialized();
	return TunaSweeperMemo::IsAvailable(MemoId) && AcquiredMemoIds.Contains(MemoId);
}

bool UTunaSweeperGameInstance::MarkMemoAcquired(int32 MemoId, bool bSaveImmediately)
{
	if (!TunaSweeperMemo::IsAvailable(MemoId))
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	const int32 PreviousCount = AcquiredMemoIds.Num();
	AcquiredMemoIds.Add(MemoId);
	if (AcquiredMemoIds.Num() == PreviousCount)
	{
		return false;
	}

	if (bSaveImmediately && !SaveGameStateInternal())
	{
		AcquiredMemoIds.Remove(MemoId);
		return false;
	}
	OnMemoStateChanged.Broadcast();
	return true;
}

void UTunaSweeperGameInstance::GetAcquiredMemoIds(TArray<int32>& OutMemoIds)
{
	EnsureInventoryStateInitialized();
	OutMemoIds = AcquiredMemoIds.Array();
	OutMemoIds.Sort();
}

bool UTunaSweeperGameInstance::HasEverAcquiredItem(int32 ItemId)
{
	EnsureInventoryStateInitialized();
	return ItemId != INDEX_NONE && EverAcquiredItemIds.Contains(ItemId);
}

void UTunaSweeperGameInstance::GetMapMarkers(TArray<FTunaSweeperMapMarkerSaveData>& OutMapMarkers)
{
	EnsureInventoryStateInitialized();
	OutMapMarkers = MapMarkers;
	OutMapMarkers.Sort([](
		const FTunaSweeperMapMarkerSaveData& Left,
		const FTunaSweeperMapMarkerSaveData& Right)
	{
		return Left.MarkerId < Right.MarkerId;
	});
}

int32 UTunaSweeperGameInstance::AddMapMarker(
	const FVector2D& MapPosition,
	int32 MarkerIconIndex,
	int32 MarkerColorIndex,
	bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	FTunaSweeperMapMarkerSaveData NewMarker;
	while (MapMarkers.ContainsByPredicate([this](const FTunaSweeperMapMarkerSaveData& Marker)
	{
		return Marker.MarkerId == NextMapMarkerId;
	}))
	{
		NextMapMarkerId = NextMapMarkerId == MAX_int32 ? 1 : NextMapMarkerId + 1;
	}
	NewMarker.MarkerId = NextMapMarkerId;
	NewMarker.MapPosition = MapPosition;
	NewMarker.MarkerIconIndex = MarkerIconIndex;
	NewMarker.MarkerColorIndex = MarkerColorIndex;
	NewMarker = TunaSweeperMapMarkers::SanitizeMarker(NewMarker);

	MapMarkers.Add(NewMarker);
	if (bSaveImmediately && !SaveMapMarkerState())
	{
		MapMarkers.Pop();
		return INDEX_NONE;
	}
	NextMapMarkerId = NextMapMarkerId == MAX_int32 ? 1 : NextMapMarkerId + 1;
	bMapMarkersDirty = !bSaveImmediately;
	OnMapMarkersChanged.Broadcast();

	return NewMarker.MarkerId;
}

bool UTunaSweeperGameInstance::RemoveMapMarker(int32 MarkerId, bool bSaveImmediately)
{
	if (MarkerId <= 0)
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	const int32 Index = MapMarkers.IndexOfByPredicate([MarkerId](const FTunaSweeperMapMarkerSaveData& Marker)
	{
		return Marker.MarkerId == MarkerId;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}

	const FTunaSweeperMapMarkerSaveData RemovedMarker = MapMarkers[Index];
	MapMarkers.RemoveAt(Index);
	if (bSaveImmediately && !SaveMapMarkerState())
	{
		MapMarkers.Insert(RemovedMarker, Index);
		return false;
	}
	bMapMarkersDirty = !bSaveImmediately;
	OnMapMarkersChanged.Broadcast();
	return true;
}

