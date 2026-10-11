#pragma once

#include "CoreMinimal.h"
#include "Settings/TunaSweeperLanguage.h"

namespace TunaSweeperEditorText
{
	FText Resolve(const TCHAR* Key);
	FText Resolve(const TCHAR* Key, ETunaSweeperItemTextLanguage Language);
}
