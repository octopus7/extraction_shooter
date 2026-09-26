#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperSaveDirectory
{
	FString ResolveLocalDirectory(const FString& SavedDirectory, bool bDemo);
	FString ResolveStoveDirectory(const FString& SavedDirectory, bool bPackaged,
		const FString& Channel, bool bDemo, const FString& CloudRoot, const FString& AccountId);
	FString GetStoveDirectory();
}
