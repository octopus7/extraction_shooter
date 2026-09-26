#include "Game/TunaSweeperSteamCloud.h"

#include "Interfaces/OnlineIdentityInterface.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "OnlineSubsystem.h"
#include "Settings/TunaSweeperBuildFlavor.h"

namespace TunaSweeperSteamCloud
{
	FString ResolveDirectory(const FString& SavedDirectory, bool bPackaged,
		const FString& DistributionChannel, bool bDemo, const FString& SteamId)
	{
		if (!bPackaged || !DistributionChannel.TrimStartAndEnd().Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
		{
			return FString();
		}
		// Steam's individual account IDs are decimal uint64 strings. Restrict the path
		// component rather than accepting arbitrary subsystem-provided strings.
		uint64 NumericId = 0;
		bool bValidAccount = !SteamId.IsEmpty() && SteamId.Len() <= 20;
		for (TCHAR Character : SteamId)
		{
			bValidAccount &= Character >= TEXT('0') && Character <= TEXT('9');
		}
		bValidAccount = bValidAccount && LexTryParseString(NumericId, *SteamId) && NumericId != 0;
		const FString AccountDirectory = bValidAccount ? SteamId : TEXT("LocalOnly");
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(SavedDirectory,
			TEXT("SaveGames/Steam"), AccountDirectory, bDemo ? TEXT("Demo") : TEXT("FullGame")));
	}

	FString GetSaveDirectory()
	{
#if WITH_EDITOR
		return FString();
#else
		// Resolve once: losing connectivity must never switch an active game's save root.
		static const FString Directory = []()
		{
			FString Channel;
			if (!GConfig || !GConfig->GetString(TEXT("TunaSweeper.Distribution"),
				TEXT("DistributionChannel"), Channel, GGameIni) ||
				!Channel.TrimStartAndEnd().Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
			{
				return FString();
			}
			FString SteamId;
			if (IOnlineSubsystem* Steam = IOnlineSubsystem::Get(FName(TEXT("STEAM"))))
			{
				const IOnlineIdentityPtr Identity = Steam->GetIdentityInterface();
				const FUniqueNetIdPtr UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
				if (UserId.IsValid() && UserId->IsValid())
				{
					SteamId = UserId->ToString();
				}
			}
			const FString Result = ResolveDirectory(FPaths::ProjectSavedDir(), true, Channel,
				TunaSweeperBuildFlavor::IsDemo(), SteamId);
			if (Result.Contains(TEXT("/LocalOnly/")))
			{
				UE_LOG(LogTemp, Warning, TEXT("Steam identity unavailable; saves remain local for this session: %s"), *Result);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Steam Auto-Cloud save directory: %s"), *Result);
			}
			return Result;
		}();
		return Directory;
#endif
	}
}
