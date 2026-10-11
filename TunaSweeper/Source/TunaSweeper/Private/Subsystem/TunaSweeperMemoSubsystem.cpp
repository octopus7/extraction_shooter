#include "Subsystem/TunaSweeperMemoSubsystem.h"
#include "Dom/JsonObject.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperMemo, Log, All);

void UTunaSweeperMemoSubsystem::Deinitialize()
{
    ResetLoadedMemoDefinitions();
    Super::Deinitialize();
}

bool UTunaSweeperMemoSubsystem::LoadMemoDefinitions(bool bForceReload)
{
	if (bMemoDefinitionDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedMemoDefinitions();

	FString JsonContent;
	const FString MemoDefinitionsJsonPath = GetMemoDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *MemoDefinitionsJsonPath))
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Failed to read memo definitions JSON: %s"), *MemoDefinitionsJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Failed to parse memo definitions JSON: %s"), *MemoDefinitionsJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo definition row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		double NumericMemoId = INDEX_NONE;
		FString BodyStringKey;
		if (!JsonObject->TryGetNumberField(TEXT("memo_id"), NumericMemoId) ||
			!JsonObject->TryGetStringField(TEXT("body_string_key"), BodyStringKey))
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo definition row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperMemoDefinition Definition;
		Definition.MemoId = static_cast<int32>(NumericMemoId);
		Definition.BodyStringKey = FName(*BodyStringKey.TrimStartAndEnd());
		if (Definition.MemoId <= 0 || Definition.BodyStringKey.IsNone())
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo definition row %d: invalid memo id or text string key."), RowIndex);
			continue;
		}

		MemoDefinitionsById.Add(Definition.MemoId, Definition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Memo definitions JSON has no valid rows: %s"), *MemoDefinitionsJsonPath);
		return false;
	}

	bMemoDefinitionDataLoaded = true;
	return true;
}

bool UTunaSweeperMemoSubsystem::TryGetMemoDefinition(int32 MemoId, FTunaSweeperMemoDefinition& OutDefinition)
{
	if (!LoadMemoDefinitions(false))
	{
		OutDefinition = FTunaSweeperMemoDefinition();
		return false;
	}

	if (const FTunaSweeperMemoDefinition* FoundDefinition = MemoDefinitionsById.Find(MemoId))
	{
		OutDefinition = *FoundDefinition;
		OutDefinition.Title = ResolveMemoTitle(MemoId);
		if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			OutDefinition.Body = TunaGameInstance->ResolveLocalizedText(OutDefinition.BodyStringKey, FText::GetEmpty());
		}
		return true;
	}

	OutDefinition = FTunaSweeperMemoDefinition();
	return false;
}

void UTunaSweeperMemoSubsystem::GetMemoListEntries(TArray<FTunaSweeperMemoListEntry>& OutEntries)
{
	OutEntries.Reset();
	if (!LoadMemoDefinitions(false))
	{
		return;
	}

	TArray<int32> MemoIds;
	MemoDefinitionsById.GenerateKeyArray(MemoIds);
	MemoIds.Sort();

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	for (int32 MemoId : MemoIds)
	{
		const FTunaSweeperMemoDefinition* Definition = MemoDefinitionsById.Find(MemoId);
		if (!Definition)
		{
			continue;
		}

		FTunaSweeperMemoListEntry Entry;
		Entry.MemoId = MemoId;
		Entry.Title = ResolveMemoTitle(MemoId);
		Entry.Body = TunaGameInstance
			? TunaGameInstance->ResolveLocalizedText(Definition->BodyStringKey, FText::GetEmpty())
			: FText::GetEmpty();
		Entry.bAcquired = TunaGameInstance && TunaGameInstance->IsMemoAcquired(MemoId);
		OutEntries.Add(Entry);
	}
}

int32 UTunaSweeperMemoSubsystem::GetFirstAcquiredMemoId()
{
	TArray<FTunaSweeperMemoListEntry> Entries;
	GetMemoListEntries(Entries);
	for (const FTunaSweeperMemoListEntry& Entry : Entries)
	{
		if (Entry.bAcquired)
		{
			return Entry.MemoId;
		}
	}

	return INDEX_NONE;
}

FText UTunaSweeperMemoSubsystem::ResolveMemoTitle(int32 MemoId) const
{
    const auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
    if (!Game || MemoId <= 0) return FText::GetEmpty();
    FFormatNamedArguments Arguments;
    Arguments.Add(TEXT("Number"), FText::FromString(FString::Printf(TEXT("%03d"), MemoId)));
    return FText::Format(Game->ResolveLocalizedText(TEXT("memo.title_format"), FText::GetEmpty()), Arguments);
}

void UTunaSweeperMemoSubsystem::ResetLoadedMemoDefinitions()
{
    MemoDefinitionsById.Reset();
    bMemoDefinitionDataLoaded = false;
}

FString UTunaSweeperMemoSubsystem::GetMemoDefinitionsJsonPath() const
{
    return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/MemoDefinitions.json"));
}
