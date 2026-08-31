#include "Achievement/TunaSweeperOnlineAchievementPublisher.h"

#include "Achievement/TunaSweeperAchievementPublisher.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"

namespace
{
	class FTunaSweeperOnlineAchievementPublisher final : public ITunaSweeperAchievementPublisher
	{
	public:
		FTunaSweeperOnlineAchievementPublisher()
		{
			RefreshInterfaces();
		}

		virtual FName GetPlatformName() const override
		{
			return PlatformName;
		}

		virtual bool IsAvailable() override
		{
			return RefreshInterfaces();
		}

		virtual bool QueryUnlockedAchievements(
			TFunction<void(bool bSuccess, TSet<FString> UnlockedPlatformIds)> Completion) override
		{
			if (!RefreshInterfaces())
			{
				return false;
			}

			const IOnlineAchievementsPtr AchievementsCopy = Achievements;
			const FUniqueNetIdPtr UserIdCopy = UserId;
			AchievementsCopy->QueryAchievements(
				*UserIdCopy,
				FOnQueryAchievementsCompleteDelegate::CreateLambda(
					[AchievementsCopy, UserIdCopy, Completion = MoveTemp(Completion)](
						const FUniqueNetId&,
						const bool bSuccess) mutable
					{
						TSet<FString> UnlockedIds;
						if (bSuccess)
						{
							TArray<FOnlineAchievement> CachedAchievements;
							if (AchievementsCopy->GetCachedAchievements(*UserIdCopy, CachedAchievements) !=
								EOnlineCachedResult::Success)
							{
								Completion(false, MoveTemp(UnlockedIds));
								return;
							}

							for (const FOnlineAchievement& Achievement : CachedAchievements)
							{
								if (Achievement.Progress >= 100.0)
								{
									UnlockedIds.Add(Achievement.Id);
								}
							}
						}
						Completion(bSuccess, MoveTemp(UnlockedIds));
					}));
			return true;
		}

		virtual bool UnlockAchievement(
			const FString& PlatformAchievementId,
			TFunction<void(bool bSuccess)> Completion) override
		{
			if (PlatformAchievementId.IsEmpty() || !RefreshInterfaces())
			{
				return false;
			}

			FOnlineAchievementsWriteRef WriteObject =
				MakeShared<FOnlineAchievementsWrite, ESPMode::ThreadSafe>();
			WriteObject->SetFloatStat(PlatformAchievementId, 100.0f);
			Achievements->WriteAchievements(
				*UserId,
				WriteObject,
				FOnAchievementsWrittenDelegate::CreateLambda(
					[Completion = MoveTemp(Completion)](const FUniqueNetId&, bool bSuccess) mutable
					{
						Completion(bSuccess);
					}));
			return true;
		}

	private:
		bool RefreshInterfaces()
		{
			IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
			PlatformName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName() : NAME_None;
			Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
			Achievements = OnlineSubsystem ? OnlineSubsystem->GetAchievementsInterface() : nullptr;
			UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
			return !PlatformName.IsNone() && Identity.IsValid() && Achievements.IsValid() && UserId.IsValid();
		}

		FName PlatformName = NAME_None;
		IOnlineIdentityPtr Identity;
		IOnlineAchievementsPtr Achievements;
		FUniqueNetIdPtr UserId;
	};
}

TSharedPtr<ITunaSweeperAchievementPublisher> MakeTunaSweeperOnlineAchievementPublisher()
{
	return MakeShared<FTunaSweeperOnlineAchievementPublisher>();
}
