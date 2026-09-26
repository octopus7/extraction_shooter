#include "Game/TunaSweeperSaveDirectory.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
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
		const FString Root = FPaths::ConvertRelativePathToFull(FPaths::Combine(SavedDirectory, TEXT("SaveGames")));
		const FString Destination = FPaths::Combine(Root, bDemo ? TEXT("Demo") : TEXT("FullGame"));
		// Keep a failed migration pinned too: primary and backup path lookups can happen
		// within one save transaction. Editor flavor changes use independent keys.
		static FCriticalSection DirectoryLock;
		static TMap<FString, FString> SessionDirectories;
		FScopeLock Lock(&DirectoryLock);
		if (const FString* Selected = SessionDirectories.Find(Destination))
		{
			return *Selected;
		}
		if (!bDemo)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			const FString Legacy = FPaths::Combine(Root, TEXT("Main"));
			if (!PlatformFile.DirectoryExists(*Destination) && PlatformFile.DirectoryExists(*Legacy))
			{
				// Win64 MoveFile performs an atomic, same-volume directory rename and refuses
				// an existing destination. Moving the whole tree also preserves recovery files.
				if (!PlatformFile.MoveFile(*Destination, *Legacy))
				{
					UE_LOG(LogTemp, Warning, TEXT("Cannot rename save directory '%s' to '%s'; retaining legacy saves."),
						*Legacy, *Destination);
					SessionDirectories.Add(Destination, Legacy);
					return Legacy;
				}
			}
		}
		SessionDirectories.Add(Destination, Destination);
		return Destination;
	}
}
