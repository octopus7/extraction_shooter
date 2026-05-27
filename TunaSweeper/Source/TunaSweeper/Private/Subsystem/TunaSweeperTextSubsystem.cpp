#include "Subsystem/TunaSweeperTextSubsystem.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperText, Log, All);

namespace TunaSweeperText
{
	const TCHAR* TextStringsCsvRelativePath = TEXT("Data/UITextStrings.csv");

	FString GetCsvCell(const TArray<const TCHAR*>& Row, int32 CellIndex)
	{
		return Row.IsValidIndex(CellIndex)
			? FString(Row[CellIndex]).TrimStartAndEnd()
			: FString();
	}
}

bool UTunaSweeperTextSubsystem::LoadTextData(bool bForceReload) const
{
	if (bTextDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedTextData();
	bTextDataLoaded = LoadTextStringsCsv();
	if (!bTextDataLoaded)
	{
		ResetLoadedTextData();
	}
	return bTextDataLoaded;
}

bool UTunaSweeperTextSubsystem::TryGetTextByKey(
	FName StringKey,
	ETunaSweeperItemTextLanguage Language,
	FText& OutText) const
{
	if (StringKey.IsNone() || !EnsureTextDataLoaded())
	{
		OutText = FText::GetEmpty();
		return false;
	}

	const FTunaSweeperLocalizedTextString* TextString = TextStringsByKey.Find(StringKey);
	if (!TextString)
	{
		OutText = FText::GetEmpty();
		return false;
	}

	switch (Language)
	{
	case ETunaSweeperItemTextLanguage::Korean:
		OutText = TextString->Korean;
		break;
	case ETunaSweeperItemTextLanguage::Japanese:
		OutText = TextString->Japanese;
		break;
	case ETunaSweeperItemTextLanguage::English:
	default:
		OutText = TextString->English;
		break;
	}

	return !OutText.IsEmpty();
}

FText UTunaSweeperTextSubsystem::ResolveText(
	FName StringKey,
	ETunaSweeperItemTextLanguage Language,
	const FText& FallbackText) const
{
	FText ResolvedText;
	if (TryGetTextByKey(StringKey, Language, ResolvedText))
	{
		return ResolvedText;
	}

	return FallbackText.IsEmpty() && !StringKey.IsNone()
		? FText::FromString(StringKey.ToString())
		: FallbackText;
}

bool UTunaSweeperTextSubsystem::EnsureTextDataLoaded() const
{
	return bTextDataLoaded || LoadTextData(false);
}

bool UTunaSweeperTextSubsystem::LoadTextStringsCsv() const
{
	FString CsvContent;
	const FString TextStringsCsvPath = GetTextStringsCsvPath();
	if (!FFileHelper::LoadFileToString(CsvContent, *TextStringsCsvPath))
	{
		UE_LOG(LogTunaSweeperText, Error, TEXT("Failed to read UI text strings CSV: %s"), *TextStringsCsvPath);
		return false;
	}

	FCsvParser CsvParser(CsvContent);
	const FCsvParser::FRows& Rows = CsvParser.GetRows();
	if (Rows.Num() < 2)
	{
		UE_LOG(LogTunaSweeperText, Error, TEXT("UI text strings CSV has no data rows: %s"), *TextStringsCsvPath);
		return false;
	}

	const TArray<const TCHAR*>& HeaderRow = Rows[0];
	const bool bHeaderIsValid =
		TunaSweeperText::GetCsvCell(HeaderRow, 0).Equals(TEXT("string_key"), ESearchCase::IgnoreCase) &&
		TunaSweeperText::GetCsvCell(HeaderRow, 1).Equals(TEXT("ko"), ESearchCase::IgnoreCase) &&
		TunaSweeperText::GetCsvCell(HeaderRow, 2).Equals(TEXT("en"), ESearchCase::IgnoreCase) &&
		TunaSweeperText::GetCsvCell(HeaderRow, 3).Equals(TEXT("ja"), ESearchCase::IgnoreCase);
	if (!bHeaderIsValid)
	{
		UE_LOG(LogTunaSweeperText, Error, TEXT("UI text strings CSV header must be string_key,ko,en,ja: %s"), *TextStringsCsvPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<const TCHAR*>& Row = Rows[RowIndex];
		if (Row.Num() < 4)
		{
			UE_LOG(LogTunaSweeperText, Warning, TEXT("Skipping UI text row %d: expected 4 columns."), RowIndex);
			continue;
		}

		const FString StringKey = TunaSweeperText::GetCsvCell(Row, 0);
		const FString Korean = TunaSweeperText::GetCsvCell(Row, 1);
		const FString English = TunaSweeperText::GetCsvCell(Row, 2);
		const FString Japanese = TunaSweeperText::GetCsvCell(Row, 3);
		if (StringKey.IsEmpty() || Korean.IsEmpty() || English.IsEmpty() || Japanese.IsEmpty())
		{
			UE_LOG(LogTunaSweeperText, Warning, TEXT("Skipping UI text row %d: required cell is empty."), RowIndex);
			continue;
		}

		FTunaSweeperLocalizedTextString TextString;
		TextString.StringKey = FName(*StringKey);
		TextString.Korean = FText::FromString(Korean);
		TextString.English = FText::FromString(English);
		TextString.Japanese = FText::FromString(Japanese);

		if (TextStringsByKey.Contains(TextString.StringKey))
		{
			UE_LOG(LogTunaSweeperText, Warning, TEXT("Duplicate UI text string key %s found. The later row will replace the earlier row."), *StringKey);
		}

		TextStringsByKey.Add(TextString.StringKey, TextString);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperText, Error, TEXT("UI text strings CSV has no valid rows: %s"), *TextStringsCsvPath);
	}

	return bHasValidRows;
}

void UTunaSweeperTextSubsystem::ResetLoadedTextData() const
{
	TextStringsByKey.Reset();
	bTextDataLoaded = false;
}

FString UTunaSweeperTextSubsystem::GetTextStringsCsvPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperText::TextStringsCsvRelativePath);
}
