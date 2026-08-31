#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TunaSweeperAchievementTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperAchievementConditionType : uint8
{
	SpecificEnemyFirstKill UMETA(DisplayName = "Specific Enemy First Kill"),
	TotalEnemyKills UMETA(DisplayName = "Total Enemy Kills"),
	LocationReached UMETA(DisplayName = "Location Reached"),
	QuestRewardClaimed UMETA(DisplayName = "Quest Reward Claimed")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperAchievementDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	FName AchievementId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	ETunaSweeperAchievementConditionType ConditionType =
		ETunaSweeperAchievementConditionType::SpecificEnemyFirstKill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	FName TargetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement", meta = (ClampMin = "1", UIMin = "1"))
	int64 RequiredCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	TMap<FName, FString> PlatformIds;
};

struct TUNASWEEPER_API FTunaSweeperAchievementProgressState
{
	int64 TotalEnemyKills = 0;
	TSet<FName> KilledEnemyIds;
	TSet<FName> ReachedLocationIds;
	TSet<FName> ClaimedQuestIds;
	TSet<FName> UnlockedAchievementIds;
	TSet<FString> ConfirmedPlatformUnlockKeys;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperAchievementSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 SaveVersion = 1;

	UPROPERTY()
	FName BuildFlavor = NAME_None;

	UPROPERTY()
	FName DistributionNamespace = NAME_None;

	UPROPERTY()
	int64 TotalEnemyKills = 0;

	UPROPERTY()
	TArray<FName> KilledEnemyIds;

	UPROPERTY()
	TArray<FName> ReachedLocationIds;

	UPROPERTY()
	TArray<FName> ClaimedQuestIds;

	UPROPERTY()
	TArray<FName> UnlockedAchievementIds;

	UPROPERTY()
	TArray<FString> ConfirmedPlatformUnlockKeys;
};
