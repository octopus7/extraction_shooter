#pragma once

#include "CoreMinimal.h"

// Supplements one source CSV. Keeping the path beside its source prevents
// narrative packs with identical keys from using another pack's translations.
class FTunaSweeperAdditionalTranslations
{
public:
	bool Load(const FString& SourceCsvPath);

	template <typename T>
	void Apply(FName Key, T& Text) const
	{
		if (const FTranslation* Translation = ByKey.Find(Key))
		{
			Text.SimplifiedChinese = Translation->SimplifiedChinese;
			Text.TraditionalChinese = Translation->TraditionalChinese;
			Text.Russian = Translation->Russian;
			Text.BrazilianPortuguese = Translation->BrazilianPortuguese;
		}
	}

private:
	struct FTranslation
	{
		FText SimplifiedChinese;
		FText TraditionalChinese;
		FText Russian;
		FText BrazilianPortuguese;
	};
	TMap<FName, FTranslation> ByKey;
};
