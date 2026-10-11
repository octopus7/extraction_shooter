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
	QuestRewardClaimed UMETA(DisplayName = "Quest Reward Claimed"),
	Reserved,
	GameplayEvent,
	AllFacilitiesRestored,
	ExtractionHealth,
	TimedExtraction,
	CurrentItemQuantity
};

// These are confirmed gameplay facts, never interaction requests or quest rewards.
UENUM(BlueprintType)
enum class ETunaSweeperAchievementEvent : uint8
{
	CharacterJoined,
	FacilityRelocated,
	FacilityUsed,
	FastTravelUsed,
	WeaponFired,
	VehicleCollisionKill,
	EndingReached
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperAchievementDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	FName AchievementId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	FName NameStringKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	ETunaSweeperAchievementEvent EventType = ETunaSweeperAchievementEvent::CharacterJoined;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	TArray<FName> RequiredFacilityIds;

	// Optional exact package paths for actual map entry. Empty until approved by authoring.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	TArray<FName> MapPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	int32 TargetItemId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	ETunaSweeperAchievementConditionType ConditionType =
		ETunaSweeperAchievementConditionType::SpecificEnemyFirstKill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	FName TargetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Achievement")
	int32 TargetQuestId = 0;

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
	TSet<int32> ClaimedQuestIds;
	TSet<FName> UnlockedAchievementIds;
	TSet<FString> ConfirmedPlatformUnlockKeys;
	TSet<FString> GameplayFacts;
};

// Never serialized: inventory/facilities belong to the current gameplay slot.
struct TUNASWEEPER_API FTunaSweeperAchievementSnapshot
{
	TMap<int32, int64> OwnedItemCounts;
	TSet<FName> RestoredFacilityIds;
	bool bSuccessfulExtraction = false;
	int32 RaidKills = 0;
	double RaidSeconds = 0.0;
	float Health = 0.0f;
	float MaxHealth = 0.0f;
};

struct TUNASWEEPER_API FTunaSweeperAchievementRaidState
{
	bool bActive = false;
	int32 Kills = 0;
	double StartedAt = 0.0;
	void Begin(double Now) { *this = {}; bActive = FMath::IsFinite(Now) && Now >= 0; StartedAt = Now; }
	void Reset() { *this = {}; }
	void RecordKill() { if (bActive && Kills < MAX_int32) ++Kills; }
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
	TArray<int32> NumericClaimedQuestIds;

	UPROPERTY()
	TArray<FName> UnlockedAchievementIds;

	UPROPERTY()
	TArray<FString> ConfirmedPlatformUnlockKeys;

	UPROPERTY()
	TArray<FString> GameplayFacts;
};
