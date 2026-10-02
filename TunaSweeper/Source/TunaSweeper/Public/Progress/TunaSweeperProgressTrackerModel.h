#pragma once
#include "CoreMinimal.h"
#include "TunaSweeperProgressTrackerModel.generated.h"

/** Observations attempted for this save; deliberately not a delivery queue. */
USTRUCT()
struct TUNASWEEPER_API FTunaSweeperProgressTrackerState
{
    GENERATED_BODY()
    UPROPERTY() FGuid RunId;
    UPROPERTY() TSet<FName> AttemptedCheckpoints;
    void StartNewRun();
    bool TryRecordAttempt(FName CheckpointId);
};

namespace TunaSweeperProgressTrackerModel
{
    TUNASWEEPER_API FString SerializeEvent(const FGuid& EventId, const FGuid& PlayerId, const FGuid& RunId,
        const FString& BuildId, const FString& CheckpointId, const FString& Category, double PlaytimeSeconds);
    /** Builds and records a single attempt only when the payload is valid. */
    TUNASWEEPER_API FString PrepareEvent(FTunaSweeperProgressTrackerState& State, const FGuid& PlayerId,
        const FString& BuildId, const FString& CheckpointId, const FString& Category, double PlaytimeSeconds);
    TUNASWEEPER_API FString LocationCheckpoint(FName LocationId);
    TUNASWEEPER_API bool IsTrackedQuest(FName QuestId);
}
