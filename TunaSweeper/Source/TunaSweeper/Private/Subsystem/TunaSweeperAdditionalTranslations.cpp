#include "Subsystem/TunaSweeperAdditionalTranslations.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperTranslations, Log, All);

bool FTunaSweeperAdditionalTranslations::Load(const FString& SourceCsvPath)
{
	ByKey.Reset();
	const FString Path = FPaths::Combine(FPaths::GetPath(SourceCsvPath), TEXT("Translations"), FPaths::GetCleanFilename(SourceCsvPath));
	// Older or untranslated packs remain valid and resolve to their English text.
	if (!FPaths::FileExists(Path)) return true;
	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *Path))
	{
		UE_LOG(LogTunaSweeperTranslations, Error, TEXT("Cannot read translations: %s"), *Path);
		return false;
	}
	FCsvParser Parser(Content);
	const auto& Rows = Parser.GetRows();
	const TCHAR* Header[] = { TEXT("string_key"), TEXT("zh-Hans"), TEXT("zh-Hant"), TEXT("ru"), TEXT("pt-BR") };
	// pt-BR is optional so older narrative translation packs still load.
	if (Rows.IsEmpty() || (Rows[0].Num() != 4 && Rows[0].Num() != UE_ARRAY_COUNT(Header)))
	{
		UE_LOG(LogTunaSweeperTranslations, Error, TEXT("Invalid translation header: %s"), *Path);
		return false;
	}
	const int32 ColumnCount = Rows[0].Num();
	for (int32 Index = 0; Index < ColumnCount; ++Index)
	{
		if (!FString(Rows[0][Index]).TrimStartAndEnd().Equals(Header[Index], ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTunaSweeperTranslations, Error, TEXT("Invalid translation header: %s"), *Path);
			return false;
		}
	}
	for (int32 Index = 1; Index < Rows.Num(); ++Index)
	{
		const auto& Row = Rows[Index];
		if (Row.Num() != ColumnCount || FString(Row[0]).TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogTunaSweeperTranslations, Error, TEXT("Invalid translation row %d: %s"), Index, *Path);
			return false;
		}
		const FName Key(*FString(Row[0]).TrimStartAndEnd());
		FTranslation Translation;
		Translation.SimplifiedChinese = FText::FromString(FString(Row[1]).TrimStartAndEnd());
		Translation.TraditionalChinese = FText::FromString(FString(Row[2]).TrimStartAndEnd());
		Translation.Russian = FText::FromString(FString(Row[3]).TrimStartAndEnd());
		if (ColumnCount == 5)
		{
			Translation.BrazilianPortuguese = FText::FromString(FString(Row[4]).TrimStartAndEnd());
		}
		if (const FTranslation* Previous = ByKey.Find(Key))
		{
			if (!Previous->SimplifiedChinese.EqualTo(Translation.SimplifiedChinese) ||
				!Previous->TraditionalChinese.EqualTo(Translation.TraditionalChinese) ||
				!Previous->Russian.EqualTo(Translation.Russian) ||
				!Previous->BrazilianPortuguese.EqualTo(Translation.BrazilianPortuguese))
			{
				UE_LOG(LogTunaSweeperTranslations, Error, TEXT("Conflicting translation key %s: %s"), *Key.ToString(), *Path);
				return false;
			}
		}
		ByKey.Add(Key, MoveTemp(Translation));
	}
	return true;
}
