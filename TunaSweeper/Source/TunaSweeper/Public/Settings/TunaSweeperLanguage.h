#pragma once

#include "CoreMinimal.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

namespace TunaSweeperLanguage
{
	inline constexpr const TCHAR* SectionName = TEXT("TunaSweeper.InterfaceSettings");
	inline constexpr const TCHAR* LanguageKey = TEXT("Language");
	inline constexpr ETunaSweeperItemTextLanguage SupportedLanguages[] = {
		ETunaSweeperItemTextLanguage::English,
		ETunaSweeperItemTextLanguage::Korean,
		ETunaSweeperItemTextLanguage::Japanese,
		ETunaSweeperItemTextLanguage::SimplifiedChinese,
		ETunaSweeperItemTextLanguage::TraditionalChinese,
		ETunaSweeperItemTextLanguage::Russian,
		ETunaSweeperItemTextLanguage::BrazilianPortuguese};

	TUNASWEEPER_API const TCHAR* ToLanguageCode(ETunaSweeperItemTextLanguage Language);
	TUNASWEEPER_API bool TryParseLanguageCode(const FString& Code, ETunaSweeperItemTextLanguage& OutLanguage);

	template <typename T>
	FText Resolve(const T& Text, ETunaSweeperItemTextLanguage Language)
	{
		const FText* Translation = &Text.English;
		switch (Language)
		{
		case ETunaSweeperItemTextLanguage::Korean: Translation = &Text.Korean; break;
		case ETunaSweeperItemTextLanguage::Japanese: Translation = &Text.Japanese; break;
		case ETunaSweeperItemTextLanguage::SimplifiedChinese: Translation = &Text.SimplifiedChinese; break;
		case ETunaSweeperItemTextLanguage::TraditionalChinese: Translation = &Text.TraditionalChinese; break;
		case ETunaSweeperItemTextLanguage::Russian: Translation = &Text.Russian; break;
		case ETunaSweeperItemTextLanguage::BrazilianPortuguese: Translation = &Text.BrazilianPortuguese; break;
		default: break;
		}
		return Translation->IsEmpty() ? Text.English : *Translation;
	}
}
