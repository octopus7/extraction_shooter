#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
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
	BunkerRescueReturn UMETA(DisplayName = "Bunker Rescue Return"),
	WarpPointUsed UMETA(DisplayName = "Warp Point Used"),
	ItemSubmitted UMETA(DisplayName = "Item Submitted"),
	ExtractionReturn UMETA(DisplayName = "Extraction Return"),
	LocationReached UMETA(DisplayName = "Location Reached"),
	VehicleLocationReached UMETA(DisplayName = "Vehicle Location Reached"),
	WorldProgressCompleted UMETA(DisplayName = "World Progress Completed"),
	ItemPossessed UMETA(DisplayName = "Item Possessed")
};

UENUM(BlueprintType)
enum class ETunaSweeperQuestPresentationTrigger : uint8
{
	OnAccept UMETA(DisplayName = "On Accept"),
	OnRewardClaim UMETA(DisplayName = "On Reward Claim")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestPresentationStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName SpeakerNameStringKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName DialogueTextStringKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	bool bUseCameraFocus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FVector CameraFocusLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CameraBlendSeconds = 0.75f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestPresentationLineView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText SpeakerName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText DialogueText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	bool bUseCameraFocus = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FVector CameraFocusLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	float CameraBlendSeconds = 0.75f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestTextString
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FName StringKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText Korean;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText English;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText Japanese;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText SimplifiedChinese;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText TraditionalChinese;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText Russian;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText BrazilianPortuguese;
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
	FName TextStringKey = NAME_None;

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
	FName TargetProviderId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName EnemyId = NAME_None;

	// Exact classes only: color variants can inherit another color's Blueprint.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FSoftClassPath> EnemyClassPaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName InteractionEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName InteractionTypeName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName WarpPointId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName TargetWarpPointId = NAME_None;

	// Arrival objectives name one destination each and always require one visit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName LocationId = NAME_None;

	// A persistent world object, credited even when completed before quest acceptance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName WorldProgressId = NAME_None;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestRewardDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest", meta = (ClampMin = "0", UIMin = "0"))
	int32 Coins = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperItemStack> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FName> HousingFacilityUnlocks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FName> WorkbenchRecipeUnlocks;

	// Describes a reward whose gameplay effect is derived from quest completion.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName DisplayStringKey = NAME_None;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	int32 QuestId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName ProviderId = NAME_None;

	// Optional reward NPC override. Acceptance remains assigned to ProviderId.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName CompletionProviderId = NAME_None;

	FName GetCompletionProviderId() const
	{
		return CompletionProviderId.IsNone() ? ProviderId : CompletionProviderId;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName TitleStringKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName DescriptionStringKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FText Description;

	// Presentation only; never creates or changes a completion objective.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName ObjectiveDisplayStringKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Quest")
	FText ObjectiveDisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	bool bAutoTrackOnAccept = true;

	// Explicit handoff quests have no objectives and await reward claim immediately after acceptance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	bool bCompleteOnAccept = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName AutoAcceptAfterScenarioFlag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<int32> RequiredCompletedQuestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperObjectiveDefinition> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FTunaSweeperQuestRewardDefinition Rewards;

	// Runtime metadata for migrating previously submitted NPC items.
	TSet<FName> LegacyNpcSubmissionObjectiveIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperQuestPresentationStep> AcceptPresentationSteps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	TArray<FTunaSweeperQuestPresentationStep> RewardPresentationSteps;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperObjectiveProgressSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest", meta = (ClampMin = "0", UIMin = "0"))
	int32 CurrentCount = 0;

	UPROPERTY()
	int32 SubmittedItemCount = 0;
};

// Original serialized schema retained only for old save migration.
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
struct TUNASWEEPER_API FTunaSweeperNumericQuestProgressSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest")
	int32 QuestId = 0;

	UPROPERTY()
	bool bOwnershipReadinessRecorded = false;

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
