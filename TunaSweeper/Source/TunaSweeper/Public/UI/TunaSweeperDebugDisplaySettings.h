#pragma once

#include "CoreMinimal.h"

/**
 * Language used by developer-facing labels.  This is intentionally separate from
 * the game UI language so future debug tools can share the same preference.
 */
enum class ETunaSweeperDebugDisplayLanguage : uint8
{
	Korean = 0,
	English = 1,
};

namespace TunaSweeperDebugDisplaySettings
{
	TUNASWEEPER_API ETunaSweeperDebugDisplayLanguage GetDebugDisplayLanguage();
	TUNASWEEPER_API void SetDebugDisplayLanguage(ETunaSweeperDebugDisplayLanguage DebugDisplayLanguage);
}
