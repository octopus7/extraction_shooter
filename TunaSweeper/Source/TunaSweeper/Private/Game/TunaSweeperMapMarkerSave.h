#pragma once

#include "Inventory/TunaSweeperSaveGame.h"

namespace TunaSweeperMapMarkerSave
{
	FString GetFilePath(const FString& GameplayPath, int32 SlotIndex);

	bool LoadOrMigrate(
		const FString& GameplayPath, int32 SlotIndex, FName BuildFlavor,
		const TArray<FTunaSweeperMapMarkerSaveData>& LegacyMarkers,
		TArray<FTunaSweeperMapMarkerSaveData>* OutMarkers = nullptr);

	bool Save(
		const FString& GameplayPath, int32 SlotIndex, FName BuildFlavor,
		const TArray<FTunaSweeperMapMarkerSaveData>& Markers);
}
