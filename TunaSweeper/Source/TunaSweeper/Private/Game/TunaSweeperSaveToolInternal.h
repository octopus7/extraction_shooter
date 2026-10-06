#pragma once
#if WITH_EDITOR
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

namespace TunaSweeperSaveTool::Internal
{
	using FObject = TSharedPtr<FJsonObject>;
	FObject Result(const FString& Code = TEXT("ok"));
	bool Fields(const FObject& Object, std::initializer_list<const TCHAR*> Allowed);
	bool Integer(const FObject& Object, const TCHAR* Key, int32& Out, int32 Min, int32 Max);
	FTunaSweeperItemInstance* Find(UTunaSweeperSaveGame& Save, const FGuid& Uid);
	bool AllowedItem(const FTunaSweeperItemDefinition& Definition);
	bool FitsEquipment(int32 Slot, const FTunaSweeperItemDefinition& Definition);
	int32 Capacity(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data);
	FString Validate(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data);
	FString Add(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data, const FObject& Request);
	FString ApplyPreset(UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data, const FObject& Preset);
	FObject ExportPreset(UTunaSweeperSaveGame& Save);
}
#endif
