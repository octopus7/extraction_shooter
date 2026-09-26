#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperSaveDirectory
{
	// Pins one directory per saved-root/flavor for the session, preserving Main if rename fails.
	FString ResolveLocalDirectory(const FString& SavedDirectory, bool bDemo);
	FString ResolveStoveDirectory(const FString& SavedDirectory, bool bPackaged,
		const FString& Channel, bool bDemo, const FString& CloudRoot, const FString& AccountId);
	FString GetStoveDirectory();
}
