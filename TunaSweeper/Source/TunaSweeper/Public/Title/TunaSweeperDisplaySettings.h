#pragma once

#include "CoreMinimal.h"

class UGameUserSettings;

namespace TunaSweeperDisplaySettings
{
	/** Replaces an unsupported exclusive-fullscreen resolution with the largest supported mode. */
	TUNASWEEPER_API bool ClampUnsupportedFullscreenResolution(UGameUserSettings& GameUserSettings);
}
