#pragma once

#include "CoreMinimal.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperQuestTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperQuestState : uint8
{
	Available UMETA(DisplayName = "Available"),
	Accepted UMETA(DisplayName = "Accepted"),
	RewardAvailable UMETA(DisplayName = "Reward Available"),
	RewardCompleted UMETA(DisplayName = "Reward Completed")
};

UENUM(BlueprintType)
enum class ETunaSweeperObjectiveType : uint8
{
	LevelTravel UMETA(DisplayName = "Level Travel"),
	ItemAcquired UMETA(DisplayName = "Item Acquired"),
	EnemyKilled UMETA(DisplayName = "Enemy Killed"),
	InteractionCompleted UMETA(DisplayName = "Interaction Completed"),
	BunkerRescueReturn UMETA(DisplayName = "Bunker Rescue Return")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperObjectiveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	ETunaSweeperObjectiveType Type = ETunaSweeperObjectiveType::LevelTravel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest", meta = (ClampMin = "1", UIMin = "1"))
	int32 RequiredCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName SourceLevelName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName TargetLevelName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName EnemyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName InteractionEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName InteractionTypeName = NAME_None;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestRewardDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest", meta = (ClampMin = "0", UIMin = "0"))
	int32 Coins = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperItemStack> Items;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName ProviderId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	bool bAutoTrackOnAccept = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FName> RequiredCompletedQuestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperObjectiveDefinition> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FTunaSweeperQuestRewardDefinition Rewards;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperObjectiveProgressSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest", meta = (ClampMin = "0", UIMin = "0"))
	int32 CurrentCount = 0;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestProgressSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	ETunaSweeperQuestState State = ETunaSweeperQuestState::Available;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperObjectiveProgressSaveData> ObjectiveProgress;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperObjectiveProgressView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FName ObjectiveId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	int32 CurrentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	int32 RequiredCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	bool bCompleted = false;
};
