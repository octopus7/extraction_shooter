#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
class FJsonObject;
namespace TunaSweeperSaveTool
{
	// Editor-only offline API. No gameplay instance is initialized and reads never recover files.
	TUNASWEEPER_API TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Request);
}
#endif
