#pragma once

#include "CoreMinimal.h"

class ITunaSweeperAchievementPublisher
{
public:
	virtual ~ITunaSweeperAchievementPublisher() = default;

	virtual FName GetPlatformName() const = 0;
	virtual bool IsAvailable() = 0;
	virtual bool QueryUnlockedAchievements(
		TFunction<void(bool bSuccess, TSet<FString> UnlockedPlatformIds)> Completion) = 0;
	virtual bool UnlockAchievement(
		const FString& PlatformAchievementId,
		TFunction<void(bool bSuccess)> Completion) = 0;
};
