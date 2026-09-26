#include "Game/TunaSweeperSaveDirectory.h"

#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Platform/TunaSweeperStove.h"
#include "Platform/TunaSweeperStoveCloudPolicy.h"
#include "Settings/TunaSweeperBuildFlavor.h"

namespace TunaSweeperSaveDirectory
{
	FString ResolveStoveDirectory(const FString& SavedDirectory, bool bPackaged,
		const FString& Channel, bool bDemo, const FString& CloudRoot, const FString& AccountId)
	{
		if (!bPackaged || !Channel.TrimStartAndEnd().Equals(TEXT("Stove"), ESearchCase::IgnoreCase))
		{
			return FString();
		}
		const TCHAR* FlavorDirectory = bDemo ? TEXT("Demo") : TEXT("FullGame");
		if (TunaSweeperStove::IsUsableCloudSaveRoot(*CloudRoot))
		{
			return FPaths::Combine(CloudRoot, FlavorDirectory);
		}
		bool bValidAccount = !AccountId.IsEmpty();
		for (TCHAR Character : AccountId)
		{
			bValidAccount &= Character >= TEXT('0') && Character <= TEXT('9');
		}
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(SavedDirectory,
			TEXT("SaveGames/Stove/LocalOnly"), bValidAccount ? AccountId : TEXT("UnknownAccount"), FlavorDirectory));
	}

	FString GetStoveDirectory()
	{
#if WITH_EDITOR
		return FString();
#else
		static const FString Directory = []()
		{
			FString Channel;
			if (!GConfig || !GConfig->GetString(TEXT("TunaSweeper.Distribution"),
				TEXT("DistributionChannel"), Channel, GGameIni) ||
				!Channel.TrimStartAndEnd().Equals(TEXT("Stove"), ESearchCase::IgnoreCase))
			{
				return FString();
			}
			FString CloudRoot;
			FString AccountId;
			TunaSweeperStove::TryGetCloudSaveRoot(CloudRoot);
			TunaSweeperStove::TryGetSaveAccountId(AccountId);
			return ResolveStoveDirectory(FPaths::ProjectSavedDir(), true, Channel,
				TunaSweeperBuildFlavor::IsDemo(), CloudRoot, AccountId);
		}();
		return Directory;
#endif
	}

	FString ResolveLocalDirectory(const FString& SavedDirectory, bool bDemo)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			SavedDirectory, TEXT("SaveGames"), bDemo ? TEXT("Demo") : TEXT("FullGame")));
	}
}
