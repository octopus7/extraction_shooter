#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

namespace SogSplatEditorUtils
{
	SOGSPLATEDITOR_API const TCHAR* GetDefaultMaterialObjectPath();
	SOGSPLATEDITOR_API UMaterialInterface* EnsureDefaultSogMaterial();
}
