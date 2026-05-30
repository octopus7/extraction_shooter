#include "Subsystem/TunaSweeperDebuffDataSubsystem.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <initializer_list>

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperDebuffData, Log, All);

namespace TunaSweeperDebuffData
{
	const TCHAR* DebuffDefinitionsJsonRelativePath = TEXT("Data/DebuffDefinitions.json");

	bool TryReadNumberField(
		const TSharedPtr<FJsonObject>& JsonObject,
		const TCHAR* FieldName,
		double& OutValue)
	{
		return JsonObject.IsValid() && FieldName && JsonObject->TryGetNumberField(FieldName, OutValue);
	}

	bool TryReadAnyNumberField(
		const TSharedPtr<FJsonObject>& JsonObject,
		std::initializer_list<const TCHAR*> FieldNames,
		double& OutValue)
	{
		for (const TCHAR* FieldName : FieldNames)
		{
			if (TryReadNumberField(JsonObject, FieldName, OutValue))
			{
				return true;
			}
		}

		return false;
	}

	bool TryReadStringField(
		const TSharedPtr<FJsonObject>& JsonObject,
		std::initializer_list<const TCHAR*> FieldNames,
		FString& OutValue)
	{
		for (const TCHAR* FieldName : FieldNames)
		{
			if (JsonObject.IsValid() && FieldName && JsonObject->TryGetStringField(FieldName, OutValue))
			{
				OutValue = OutValue.TrimStartAndEnd();
				return true;
			}
		}

		return false;
	}

	void ParseCameraReaction(
		const TSharedPtr<FJsonObject>& DebuffObject,
		FTunaSweeperDebuffCameraReactionSettings& OutSettings)
	{
		const TSharedPtr<FJsonObject>* CameraObjectPtr = nullptr;
		if (!DebuffObject.IsValid() ||
			!DebuffObject->TryGetObjectField(TEXT("camera_reaction"), CameraObjectPtr) ||
			!CameraObjectPtr ||
			!CameraObjectPtr->IsValid())
		{
			OutSettings.Normalize();
			return;
		}

		const TSharedPtr<FJsonObject>& CameraObject = *CameraObjectPtr;
		double NumericValue = 0.0;
		if (TryReadAnyNumberField(CameraObject, { TEXT("duration_seconds"), TEXT("duration") }, NumericValue))
		{
			OutSettings.DurationSeconds = static_cast<float>(NumericValue);
		}
		if (TryReadAnyNumberField(CameraObject, { TEXT("location_amplitude"), TEXT("location_amplitude_cm") }, NumericValue))
		{
			OutSettings.LocationAmplitude = static_cast<float>(NumericValue);
		}
		if (TryReadAnyNumberField(CameraObject, { TEXT("roll_amplitude_degrees"), TEXT("roll_degrees") }, NumericValue))
		{
			OutSettings.RollAmplitudeDegrees = static_cast<float>(NumericValue);
		}
		if (TryReadAnyNumberField(CameraObject, { TEXT("fov_amplitude_degrees"), TEXT("fov_degrees") }, NumericValue))
		{
			OutSettings.FOVAmplitudeDegrees = static_cast<float>(NumericValue);
		}
		if (TryReadAnyNumberField(CameraObject, { TEXT("frequency"), TEXT("frequency_hz") }, NumericValue))
		{
			OutSettings.Frequency = static_cast<float>(NumericValue);
		}

		OutSettings.Normalize();
	}
}

bool UTunaSweeperDebuffDataSubsystem::LoadDebuffData(bool bForceReload)
{
	if (bDebuffDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedDebuffData();
	if (!LoadDebuffDefinitionsJson())
	{
		ResetLoadedDebuffData();
		InstallFallbackDefinitions();
	}

	bDebuffDataLoaded = DebuffDefinitionsById.Num() > 0;
	return bDebuffDataLoaded;
}

bool UTunaSweeperDebuffDataSubsystem::TryGetDebuffDefinition(
	FName DebuffId,
	FTunaSweeperDebuffDefinition& OutDefinition)
{
	if (!EnsureDebuffDataLoaded())
	{
		OutDefinition = FTunaSweeperDebuffDefinition();
		return false;
	}

	if (const FTunaSweeperDebuffDefinition* FoundDefinition = DebuffDefinitionsById.Find(DebuffId))
	{
		OutDefinition = *FoundDefinition;
		return true;
	}

	OutDefinition = FTunaSweeperDebuffDefinition();
	return false;
}

FString UTunaSweeperDebuffDataSubsystem::BuildDebuffIconObjectPath(
	const FTunaSweeperDebuffDefinition& Definition) const
{
	const FString IconAssetName = FPaths::GetBaseFilename(Definition.IconFileName);
	if (IconAssetName.IsEmpty())
	{
		return FString();
	}

	return FString::Printf(
		TEXT("/Game/UI/Icons/%s.%s"),
		*IconAssetName,
		*IconAssetName);
}

bool UTunaSweeperDebuffDataSubsystem::EnsureDebuffDataLoaded()
{
	return bDebuffDataLoaded || LoadDebuffData(false);
}

bool UTunaSweeperDebuffDataSubsystem::LoadDebuffDefinitionsJson()
{
	FString JsonContent;
	const FString DebuffDefinitionsJsonPath = GetDebuffDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *DebuffDefinitionsJsonPath))
	{
		UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Failed to read debuff definitions JSON: %s"), *DebuffDefinitionsJsonPath);
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, RootValue) || !RootValue.IsValid())
	{
		UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Failed to parse debuff definitions JSON: %s"), *DebuffDefinitionsJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	if (RootValue->Type == EJson::Array)
	{
		JsonRows = RootValue->AsArray();
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		double NumericGlobalTickIntervalSeconds = GlobalTickIntervalSeconds;
		if (TunaSweeperDebuffData::TryReadAnyNumberField(
			RootObject,
			{
				TEXT("global_tick_interval_seconds"),
				TEXT("default_tick_interval_seconds"),
				TEXT("tick_interval_seconds")
			},
			NumericGlobalTickIntervalSeconds))
		{
			GlobalTickIntervalSeconds = FMath::Max(0.01f, static_cast<float>(NumericGlobalTickIntervalSeconds));
		}

		const TArray<TSharedPtr<FJsonValue>>* DebuffArray = nullptr;
		if (!RootObject.IsValid() ||
			(!RootObject->TryGetArrayField(TEXT("debuffs"), DebuffArray) &&
			 !RootObject->TryGetArrayField(TEXT("definitions"), DebuffArray)) ||
			!DebuffArray)
		{
			UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Debuff definitions JSON has no debuff array: %s"), *DebuffDefinitionsJsonPath);
			return false;
		}
		JsonRows = *DebuffArray;
	}
	else
	{
		UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Debuff definitions JSON root is not an object or array: %s"), *DebuffDefinitionsJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() ||
			!JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr ||
			!JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Skipping debuff definition row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString DebuffId;
		if (!TunaSweeperDebuffData::TryReadStringField(
			JsonObject,
			{ TEXT("debuff_id"), TEXT("id") },
			DebuffId) ||
			DebuffId.IsEmpty())
		{
			UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Skipping debuff definition row %d: debuff id is missing."), RowIndex);
			continue;
		}

		FTunaSweeperDebuffDefinition Definition;
		Definition.DebuffId = FName(*DebuffId);
		Definition.TickIntervalSeconds = GlobalTickIntervalSeconds;

		FString StringValue;
		if (TunaSweeperDebuffData::TryReadStringField(JsonObject, { TEXT("name_string_key"), TEXT("display_name_key") }, StringValue))
		{
			Definition.NameStringKey = FName(*StringValue);
		}
		if (TunaSweeperDebuffData::TryReadStringField(JsonObject, { TEXT("icon_file_name"), TEXT("icon") }, StringValue))
		{
			Definition.IconFileName = StringValue;
		}

		double NumericValue = 0.0;
		if (TunaSweeperDebuffData::TryReadAnyNumberField(
			JsonObject,
			{
				TEXT("base_apply_chance"),
				TEXT("apply_chance"),
				TEXT("base_chance")
			},
			NumericValue))
		{
			Definition.BaseApplyChance =
				TunaSweeperDataValues::ClampProbabilityValue(FMath::RoundToInt(NumericValue));
		}
		if (TunaSweeperDebuffData::TryReadAnyNumberField(
			JsonObject,
			{ TEXT("duration_seconds"), TEXT("duration") },
			NumericValue))
		{
			Definition.DurationSeconds = static_cast<float>(NumericValue);
		}
		if (TunaSweeperDebuffData::TryReadAnyNumberField(
			JsonObject,
			{ TEXT("tick_interval_seconds"), TEXT("interval_seconds"), TEXT("tick_seconds") },
			NumericValue))
		{
			Definition.TickIntervalSeconds = static_cast<float>(NumericValue);
		}
		if (TunaSweeperDebuffData::TryReadAnyNumberField(
			JsonObject,
			{ TEXT("damage_per_tick"), TEXT("tick_damage") },
			NumericValue))
		{
			Definition.DamagePerTick = static_cast<float>(NumericValue);
		}
		TunaSweeperDebuffData::ParseCameraReaction(JsonObject, Definition.CameraReaction);
		Definition.Normalize();

		if (Definition.DebuffId.IsNone())
		{
			UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Skipping debuff definition row %d: debuff id is invalid."), RowIndex);
			continue;
		}

		if (DebuffDefinitionsById.Contains(Definition.DebuffId))
		{
			UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Duplicate debuff id %s found. The later row will replace the earlier row."), *Definition.DebuffId.ToString());
		}

		DebuffDefinitionsById.Add(Definition.DebuffId, Definition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperDebuffData, Warning, TEXT("Debuff definitions JSON has no valid rows: %s"), *DebuffDefinitionsJsonPath);
	}

	return bHasValidRows;
}

void UTunaSweeperDebuffDataSubsystem::ResetLoadedDebuffData()
{
	DebuffDefinitionsById.Reset();
	GlobalTickIntervalSeconds = 2.0f;
	bDebuffDataLoaded = false;
}

void UTunaSweeperDebuffDataSubsystem::InstallFallbackDefinitions()
{
	FTunaSweeperDebuffDefinition BleedingDefinition;
	BleedingDefinition.DebuffId = TunaSweeperDebuff::BleedingDebuffId();
	BleedingDefinition.NameStringKey = FName(TEXT("ui.debuff.bleeding"));
	BleedingDefinition.IconFileName = TEXT("T_UIIcon_Bandage.uasset");
	BleedingDefinition.BaseApplyChance = 400;
	BleedingDefinition.DurationSeconds = 12.0f;
	BleedingDefinition.TickIntervalSeconds = GlobalTickIntervalSeconds;
	BleedingDefinition.DamagePerTick = 2.0f;
	BleedingDefinition.Normalize();
	DebuffDefinitionsById.Add(BleedingDefinition.DebuffId, BleedingDefinition);
	bDebuffDataLoaded = true;
}

FString UTunaSweeperDebuffDataSubsystem::GetDebuffDefinitionsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperDebuffData::DebuffDefinitionsJsonRelativePath);
}
