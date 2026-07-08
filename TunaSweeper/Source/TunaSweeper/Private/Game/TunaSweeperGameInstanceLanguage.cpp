#include "TunaSweeperGameInstanceShared.h"

void UTunaSweeperGameInstance::SetCurrentTextLanguage(ETunaSweeperItemTextLanguage Language, bool bSaveImmediately)
{
	const bool bLanguageChanged = CurrentTextLanguage != Language;
	CurrentTextLanguage = Language;
	ApplyCurrentLanguageCulture();

	if (bSaveImmediately)
	{
		SaveGlobalLanguageSetting();
	}

	if (bLanguageChanged)
	{
		OnLanguageChanged.Broadcast();
	}
}

FText UTunaSweeperGameInstance::ResolveLocalizedText(FName StringKey, const FText& FallbackText) const
{
	if (StringKey.IsNone())
	{
		return FallbackText;
	}

	if (const UTunaSweeperTextSubsystem* TextSubsystem = GetSubsystem<UTunaSweeperTextSubsystem>())
	{
		return TextSubsystem->ResolveText(StringKey, CurrentTextLanguage, FallbackText);
	}

	return FallbackText;
}

void UTunaSweeperGameInstance::InitializeGlobalLanguageSetting()
{
	ETunaSweeperItemTextLanguage LoadedLanguage = ETunaSweeperItemTextLanguage::English;
	if (LoadGlobalLanguageSetting(LoadedLanguage))
	{
		CurrentTextLanguage = LoadedLanguage;
		ApplyCurrentLanguageCulture();
		return;
	}

	CurrentTextLanguage = DetectDefaultLanguageFromOS();
	ApplyCurrentLanguageCulture();
	SaveGlobalLanguageSetting();
}

bool UTunaSweeperGameInstance::LoadGlobalLanguageSetting(ETunaSweeperItemTextLanguage& OutLanguage) const
{
	if (!GConfig)
	{
		return false;
	}

	FString SavedLanguageCode;
	if (!GConfig->GetString(
		TunaSweeperLanguage::SectionName,
		TunaSweeperLanguage::LanguageKey,
		SavedLanguageCode,
		GGameUserSettingsIni))
	{
		return false;
	}

	return TunaSweeperLanguage::TryParseLanguageCode(SavedLanguageCode, OutLanguage);
}

void UTunaSweeperGameInstance::SaveGlobalLanguageSetting() const
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetString(
		TunaSweeperLanguage::SectionName,
		TunaSweeperLanguage::LanguageKey,
		TunaSweeperLanguage::ToLanguageCode(CurrentTextLanguage),
		GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

ETunaSweeperItemTextLanguage UTunaSweeperGameInstance::DetectDefaultLanguageFromOS() const
{
	ETunaSweeperItemTextLanguage DetectedLanguage = ETunaSweeperItemTextLanguage::English;
	if (TunaSweeperLanguage::TryParseLanguageCode(FPlatformMisc::GetDefaultLanguage(), DetectedLanguage))
	{
		return DetectedLanguage;
	}

	if (TunaSweeperLanguage::TryParseLanguageCode(FPlatformMisc::GetDefaultLocale(), DetectedLanguage))
	{
		return DetectedLanguage;
	}

	return ETunaSweeperItemTextLanguage::English;
}

void UTunaSweeperGameInstance::ApplyCurrentLanguageCulture() const
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		return;
	}
#endif

	FInternationalization::Get().SetCurrentCulture(TunaSweeperLanguage::ToLanguageCode(CurrentTextLanguage));
}

