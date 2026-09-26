#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperSteamCloud
{
	// Empty means this runtime must use its normal local save directory.
	FString ResolveDirectory(const FString& SavedDirectory, bool bPackaged,
		const FString& DistributionChannel, bool bDemo, const FString& SteamId);
	FString GetSaveDirectory();
}
