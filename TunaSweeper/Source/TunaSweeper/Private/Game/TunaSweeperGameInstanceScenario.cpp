#include "TunaSweeperGameInstanceShared.h"
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

	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
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
	return MemoId > 0 && AcquiredMemoIds.Contains(MemoId);
}

bool UTunaSweeperGameInstance::MarkMemoAcquired(int32 MemoId, bool bSaveImmediately)
{
	if (MemoId <= 0)
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

	OnMemoStateChanged.Broadcast();
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
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
	NewMarker.MarkerId = FMath::Max(1, NextMapMarkerId++);
	NewMarker.MapPosition = MapPosition;
	NewMarker.MarkerIconIndex = MarkerIconIndex;
	NewMarker.MarkerColorIndex = MarkerColorIndex;
	NewMarker = TunaSweeperMapMarkers::SanitizeMarker(NewMarker);

	MapMarkers.Add(NewMarker);
	OnMapMarkersChanged.Broadcast();
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}

	return NewMarker.MarkerId;
}

bool UTunaSweeperGameInstance::RemoveMapMarker(int32 MarkerId, bool bSaveImmediately)
{
	if (MarkerId <= 0)
	{
		return false;
	}

	EnsureInventoryStateInitialized();
	const int32 RemovedCount = MapMarkers.RemoveAll([MarkerId](const FTunaSweeperMapMarkerSaveData& Marker)
	{
		return Marker.MarkerId == MarkerId;
	});
	if (RemovedCount <= 0)
	{
		return false;
	}

	OnMapMarkersChanged.Broadcast();
	if (bSaveImmediately)
	{
		SaveGameStateInternal();
	}
	return true;
}

