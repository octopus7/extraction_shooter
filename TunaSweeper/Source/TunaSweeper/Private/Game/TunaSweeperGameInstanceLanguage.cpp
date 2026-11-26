#include "TunaSweeperGameInstanceShared.h"

void UTunaSweeperGameInstance::SetCurrentTextLanguage(ETunaSweeperItemTextLanguage Language, bool bSaveImmediately)
{
	const auto PreviousLanguage = GetCurrentTextLanguage();
	CurrentTextLanguage = Language;
	ApplyCurrentLanguageCulture();

	if (bSaveImmediately)
	{
		SaveGlobalLanguageSetting(Language);
	}

	if (PreviousLanguage != GetCurrentTextLanguage())
	{
		OnLanguageChanged.Broadcast();
	}
}

bool UTunaSweeperGameInstance::HasConfirmedTextLanguage() const
{
	ETunaSweeperItemTextLanguage SavedLanguage = ETunaSweeperItemTextLanguage::English;
	return LoadGlobalLanguageSetting(SavedLanguage);
}

bool UTunaSweeperGameInstance::ConfirmInitialTextLanguage(ETunaSweeperItemTextLanguage Language)
{
	if (!SaveGlobalLanguageSetting(Language))
	{
		return false;
	}
	SetCurrentTextLanguage(Language, false);
	return true;
}

void UTunaSweeperGameInstance::SetTextLanguageOverride(TOptional<ETunaSweeperItemTextLanguage> Language)
{
	const auto PreviousLanguage = GetCurrentTextLanguage();
	TextLanguageOverride = Language;
	if (PreviousLanguage != GetCurrentTextLanguage())
	{
		ApplyCurrentLanguageCulture();
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
		return TextSubsystem->ResolveText(StringKey, GetCurrentTextLanguage(), FallbackText);
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

	// Keep OS detection available; the first-launch flow currently suggests English.
	CurrentTextLanguage = ETunaSweeperItemTextLanguage::English;
	ApplyCurrentLanguageCulture();
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

bool UTunaSweeperGameInstance::SaveGlobalLanguageSetting(ETunaSweeperItemTextLanguage Language) const
{
	const FConfigBranch* Settings = GConfig
		? GConfig->FindBranch(FName(*GGameUserSettingsIni), GGameUserSettingsIni)
		: nullptr;
	if (!Settings || Settings->IniPath.IsEmpty())
	{
		return false;
	}

	using namespace TunaSweeperLanguage;
	const TCHAR* Code = ToLanguageCode(Language);
	FString PreviousCode;
	const bool bHadPreviousCode = GConfig->GetString(SectionName, LanguageKey, PreviousCode, GGameUserSettingsIni);
	GConfig->SetString(SectionName, LanguageKey, Code, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	FConfigFile SavedConfig;
	// The cache accepts a named branch such as GameUserSettings. Read requires its disk path.
	SavedConfig.Read(Settings->IniPath);
	FString SavedCode;
	if (SavedConfig.GetString(SectionName, LanguageKey, SavedCode) && SavedCode == Code)
	{
		return true;
	}
	if (bHadPreviousCode)
	{
		GConfig->SetString(SectionName, LanguageKey, *PreviousCode, GGameUserSettingsIni);
	}
	else
	{
		GConfig->RemoveKey(SectionName, LanguageKey, GGameUserSettingsIni);
	}
	return false;
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

	FInternationalization::Get().SetCurrentCulture(TunaSweeperLanguage::ToLanguageCode(GetCurrentTextLanguage()));
}

