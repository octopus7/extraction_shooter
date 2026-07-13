#include "UI/TunaSweeperDebugDisplaySettings.h"

#include "Misc/ConfigCacheIni.h"

namespace TunaSweeperDebugDisplaySettings
{
	namespace
	{
		constexpr const TCHAR* ConfigSection = TEXT("TunaSweeper.DebugDisplaySettings");
		constexpr const TCHAR* DebugDisplayLanguageKey = TEXT("DebugDisplayLanguage");

		ETunaSweeperDebugDisplayLanguage ToDebugDisplayLanguage(int32 ConfigValue)
		{
			return ConfigValue == static_cast<int32>(ETunaSweeperDebugDisplayLanguage::English)
				? ETunaSweeperDebugDisplayLanguage::English
				: ETunaSweeperDebugDisplayLanguage::Korean;
		}
	}

	ETunaSweeperDebugDisplayLanguage GetDebugDisplayLanguage()
	{
		int32 ConfigValue = static_cast<int32>(ETunaSweeperDebugDisplayLanguage::Korean);
		if (GConfig)
		{
			GConfig->GetInt(ConfigSection, DebugDisplayLanguageKey, ConfigValue, GGameUserSettingsIni);
		}

		return ToDebugDisplayLanguage(ConfigValue);
	}

	void SetDebugDisplayLanguage(ETunaSweeperDebugDisplayLanguage DebugDisplayLanguage)
	{
		if (!GConfig)
		{
			return;
		}

		GConfig->SetInt(
			ConfigSection,
			DebugDisplayLanguageKey,
			static_cast<int32>(DebugDisplayLanguage),
			GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}
