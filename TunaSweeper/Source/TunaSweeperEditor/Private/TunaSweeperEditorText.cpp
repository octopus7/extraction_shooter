#include "TunaSweeperEditorText.h"

#include "Engine/GameInstance.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/StrongObjectPtr.h"

FText TunaSweeperEditorText::Resolve(const TCHAR* Key, ETunaSweeperItemTextLanguage Language)
{
	check(IsInGameThread());
	static TMap<ETunaSweeperItemTextLanguage, TMap<FName, FText>> Cache;
	TMap<FName, FText>& LanguageTexts = Cache.FindOrAdd(Language);
	const FName StringKey(Key);
	if (const FText* CachedText = LanguageTexts.Find(StringKey))
	{
		return *CachedText;
	}

	// EndPlayMap garbage-marks every GameInstance, including editor-only owners.
	// Keep only value text in the cache so no preview UObject survives PIE cleanup.
	TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
	TStrongObjectPtr<UTunaSweeperTextSubsystem> Strings(NewObject<UTunaSweeperTextSubsystem>(Owner.Get()));
	FText Resolved;
	if (Strings->TryGetTextByKey(StringKey, Language, Resolved))
	{
		LanguageTexts.Add(StringKey, Resolved);
		return Resolved;
	}
	return Strings->ResolveText(StringKey, Language, FText::GetEmpty());
}

FText TunaSweeperEditorText::Resolve(const TCHAR* Key)
{
	ETunaSweeperItemTextLanguage Language = ETunaSweeperItemTextLanguage::English;
	TunaSweeperLanguage::TryParseLanguageCode(FInternationalization::Get().GetCurrentCulture()->GetName(), Language);
	return Resolve(Key, Language);
}
