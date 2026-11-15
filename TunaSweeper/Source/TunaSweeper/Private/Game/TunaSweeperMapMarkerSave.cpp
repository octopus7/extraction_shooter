#include "Game/TunaSweeperMapMarkerSave.h"

#include "Game/TunaSweeperSafeSave.h"
#include "Misc/Paths.h"

namespace TunaSweeperMapMarkerSave
{
	namespace
	{
		TunaSweeperSafeSave::FSaveValidator MakeValidator(int32 SlotIndex, FName BuildFlavor)
		{
			return [SlotIndex, BuildFlavor](const USaveGame& Object)
			{
				const auto* SaveGame = Cast<UTunaSweeperMapMarkerSaveGame>(&Object);
				return SaveGame && SaveGame->SaveVersion == 1 &&
					SaveGame->SaveSlotIndex == SlotIndex && SaveGame->BuildFlavor == BuildFlavor;
			};
		}
	}

	FString GetFilePath(const FString& GameplayPath, int32 SlotIndex)
	{
		return FPaths::GetPath(GameplayPath) / FString::Printf(TEXT("MapMarkers_Slot%02d.sav"), SlotIndex);
	}

	bool LoadOrMigrate(
		const FString& GameplayPath, int32 SlotIndex, FName BuildFlavor,
		const TArray<FTunaSweeperMapMarkerSaveData>& LegacyMarkers,
		TArray<FTunaSweeperMapMarkerSaveData>* OutMarkers)
	{
		if (!OutMarkers && LegacyMarkers.IsEmpty())
		{
			return true;
		}

		const FString Path = GetFilePath(GameplayPath, SlotIndex);
		const FString Previous = TunaSweeperSafeSave::GetPreviousFilePath(Path);
		if (FPaths::FileExists(Path) || FPaths::FileExists(Previous))
		{
			const auto Validator = MakeValidator(SlotIndex, BuildFlavor);
			if (!OutMarkers)
			{
				return TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator) ||
					TunaSweeperSafeSave::LoadVerifiedSaveFile(Previous, Validator);
			}

			OutMarkers->Reset();
			const auto* Stored = Cast<UTunaSweeperMapMarkerSaveGame>(
				TunaSweeperSafeSave::LoadSaveFileWithRecovery(Path, {Previous}, Validator));
			if (!Stored)
			{
				return false;
			}
			*OutMarkers = Stored->MapMarkers;
			return true;
		}

		// The legacy markers remain available in memory if the first write fails.
		if (OutMarkers)
		{
			*OutMarkers = LegacyMarkers;
		}
		return LegacyMarkers.IsEmpty() || Save(GameplayPath, SlotIndex, BuildFlavor, LegacyMarkers);
	}

	bool Save(
		const FString& GameplayPath, int32 SlotIndex, FName BuildFlavor,
		const TArray<FTunaSweeperMapMarkerSaveData>& Markers)
	{
		auto* SaveGame = NewObject<UTunaSweeperMapMarkerSaveGame>();
		SaveGame->SaveSlotIndex = SlotIndex;
		SaveGame->BuildFlavor = BuildFlavor;
		SaveGame->MapMarkers = Markers;
		return TunaSweeperSafeSave::SaveGameFileFailClosed(
			SaveGame, GetFilePath(GameplayPath, SlotIndex), MakeValidator(SlotIndex, BuildFlavor));
	}
}
