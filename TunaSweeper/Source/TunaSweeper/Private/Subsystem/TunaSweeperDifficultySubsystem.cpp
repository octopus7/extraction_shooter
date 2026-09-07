#include "Subsystem/TunaSweeperDifficultySubsystem.h"

#include "Dom/JsonObject.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperDifficulty, Log, All);

namespace TunaSweeperDifficulty
{
	const TCHAR* DefinitionsJsonRelativePath = TEXT("Data/DifficultyDefinitions.json");
}

bool UTunaSweeperDifficultySubsystem::LoadDifficultyData(bool bForceReload)
{
	if (bDifficultyDataLoaded && !bForceReload)
	{
		return true;
	}

	DefinitionsByStage.Reset();
	FString JsonContent;
	const FString JsonPath = FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperDifficulty::DefinitionsJsonRelativePath);
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTunaSweeperDifficulty, Error, TEXT("Failed to read difficulty definitions JSON: %s"), *JsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), Rows))
	{
		UE_LOG(LogTunaSweeperDifficulty, Error, TEXT("Failed to parse difficulty definitions JSON: %s"), *JsonPath);
		return false;
	}

	bool bAllRowsValid = true;
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* ObjectPtr = nullptr;
		if (!Rows[RowIndex].IsValid() || !Rows[RowIndex]->TryGetObject(ObjectPtr) || !ObjectPtr || !ObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperDifficulty, Error, TEXT("Difficulty row %d is not an object."), RowIndex);
			bAllRowsValid = false;
			continue;
		}

		double NumericStage = 0.0;
		double NumericMultiplier = TunaSweeperDataValues::RatioIdentity;
		FString TitleStringKey;
		FString DescriptionStringKey;
		const TSharedPtr<FJsonObject>& Object = *ObjectPtr;
		if (!Object->TryGetNumberField(TEXT("difficulty_stage"), NumericStage) ||
			!Object->TryGetStringField(TEXT("title_string_key"), TitleStringKey) ||
			!Object->TryGetStringField(TEXT("description_string_key"), DescriptionStringKey) ||
			!Object->TryGetNumberField(TEXT("enemy_incoming_damage_multiplier"), NumericMultiplier))
		{
			UE_LOG(LogTunaSweeperDifficulty, Error, TEXT("Difficulty row %d is missing a required field."), RowIndex);
			bAllRowsValid = false;
			continue;
		}

		FTunaSweeperDifficultyDefinition Definition;
		Definition.DifficultyStage = FMath::RoundToInt(NumericStage);
		Definition.TitleStringKey = FName(*TitleStringKey.TrimStartAndEnd());
		Definition.DescriptionStringKey = FName(*DescriptionStringKey.TrimStartAndEnd());
		Definition.EnemyIncomingDamageMultiplier = FMath::RoundToInt(NumericMultiplier);
		if (Definition.DifficultyStage < 1 || Definition.DifficultyStage > 3 ||
			Definition.TitleStringKey.IsNone() || Definition.DescriptionStringKey.IsNone() ||
			Definition.EnemyIncomingDamageMultiplier < 0 || DefinitionsByStage.Contains(Definition.DifficultyStage))
		{
			UE_LOG(LogTunaSweeperDifficulty, Error, TEXT("Difficulty row %d has invalid or duplicate values."), RowIndex);
			bAllRowsValid = false;
			continue;
		}
		DefinitionsByStage.Add(Definition.DifficultyStage, Definition);
	}

	bDifficultyDataLoaded = bAllRowsValid && DefinitionsByStage.Num() == 3;
	if (!bDifficultyDataLoaded)
	{
		UE_LOG(LogTunaSweeperDifficulty, Error, TEXT("Difficulty definitions must contain exactly one valid row for stages 1, 2, and 3."));
		DefinitionsByStage.Reset();
	}
	return bDifficultyDataLoaded;
}

bool UTunaSweeperDifficultySubsystem::TryGetDifficultyDefinition(
	int32 DifficultyStage,
	FTunaSweeperDifficultyDefinition& OutDefinition)
{
	if (LoadDifficultyData(false))
	{
		if (const FTunaSweeperDifficultyDefinition* Definition = DefinitionsByStage.Find(DifficultyStage))
		{
			OutDefinition = *Definition;
			return true;
		}
	}
	OutDefinition = FTunaSweeperDifficultyDefinition();
	return false;
}

int32 UTunaSweeperDifficultySubsystem::GetEnemyIncomingDamageMultiplier(int32 DifficultyStage)
{
	FTunaSweeperDifficultyDefinition Definition;
	return TryGetDifficultyDefinition(DifficultyStage, Definition)
		? Definition.EnemyIncomingDamageMultiplier
		: TunaSweeperDataValues::RatioIdentity;
}

float UTunaSweeperDifficultySubsystem::ScaleEnemyIncomingDamage(float RawDamage, int32 RawMultiplier)
{
	const float ScaledDamage = FMath::Max(0.0f, RawDamage) *
		TunaSweeperDataValues::ToRatioFloat(FMath::Max(0, RawMultiplier));
	return static_cast<float>(FMath::Max(0, FMath::RoundToInt(ScaledDamage)));
}

float UTunaSweeperDifficultySubsystem::ResolveAppliedPlayerDamage(
	float RawDamage,
	int32 DefenseValue,
	bool bEnemyAttributed,
	int32 EnemyIncomingDamageMultiplier)
{
	const float DamageBeforeDefense = bEnemyAttributed
		? ScaleEnemyIncomingDamage(RawDamage, EnemyIncomingDamageMultiplier)
		: FMath::Max(0.0f, RawDamage);
	return FMath::Max(0.0f, DamageBeforeDefense - static_cast<float>(FMath::Max(0, DefenseValue)));
}
