#pragma once

#include "CoreMinimal.h"

class UTunaSweeperGameInstance;

namespace TunaSweeperUiText
{
	TUNASWEEPER_API FText ResolveUiText(
		const UTunaSweeperGameInstance* TunaGameInstance,
		const TCHAR* StringKey,
		const TCHAR* Fallback);
}
