#include "Settings/TunaSweeperLanguage.h"

const TCHAR* TunaSweeperLanguage::ToLanguageCode(ETunaSweeperItemTextLanguage Language)
{
	switch (Language)
	{
	case ETunaSweeperItemTextLanguage::Korean: return TEXT("ko");
	case ETunaSweeperItemTextLanguage::Japanese: return TEXT("ja");
	case ETunaSweeperItemTextLanguage::SimplifiedChinese: return TEXT("zh-Hans");
	case ETunaSweeperItemTextLanguage::TraditionalChinese: return TEXT("zh-Hant");
	case ETunaSweeperItemTextLanguage::Russian: return TEXT("ru");
	case ETunaSweeperItemTextLanguage::BrazilianPortuguese: return TEXT("pt-BR");
	default: return TEXT("en");
	}
}

bool TunaSweeperLanguage::TryParseLanguageCode(const FString& Code, ETunaSweeperItemTextLanguage& OutLanguage)
{
	TArray<FString> Parts;
	Code.TrimStartAndEnd().ToLower().Replace(TEXT("_"), TEXT("-")).ParseIntoArray(Parts, TEXT("-"), true);
	if (Parts.IsEmpty()) return false;
	if (Parts[0] == TEXT("pt"))
	{
		if (Parts.Num() > 1 && Parts[1] != TEXT("br")) return false;
		OutLanguage = ETunaSweeperItemTextLanguage::BrazilianPortuguese;
		return true;
	}
	if (Parts[0] == TEXT("zh"))
	{
		// An explicit script takes precedence over a region (for example zh-Hans-HK).
		if (Parts.Contains(TEXT("hant")) || Parts.Contains(TEXT("cht")))
			OutLanguage = ETunaSweeperItemTextLanguage::TraditionalChinese;
		else if (Parts.Contains(TEXT("hans")) || Parts.Contains(TEXT("chs")))
			OutLanguage = ETunaSweeperItemTextLanguage::SimplifiedChinese;
		else
			OutLanguage = Parts.Contains(TEXT("tw")) || Parts.Contains(TEXT("hk")) || Parts.Contains(TEXT("mo"))
				? ETunaSweeperItemTextLanguage::TraditionalChinese : ETunaSweeperItemTextLanguage::SimplifiedChinese;
		return true;
	}
	for (const auto Language : SupportedLanguages)
	{
		if (Parts[0] == ToLanguageCode(Language))
		{
			OutLanguage = Language;
			return true;
		}
	}
	return false;
}
