#pragma once
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "Quest/TunaSweeperQuestTypes.h"
class UTunaSweeperSaveGame;
namespace TunaSweeperQuestCheat
{
    struct FPrerequisiteIssue
    {
        int32 QuestId = 0;
        int32 PrerequisiteId = 0;
        FString Code;
        ETunaSweeperQuestState PrerequisiteState = ETunaSweeperQuestState::Available;
    };
    struct FPrerequisiteAudit
    {
        TArray<FPrerequisiteIssue> Issues;
        TArray<int32> FixQuestIds;
        bool bCanFix = false;
    };
    // Read-only audit of the ancestor chains of started quests in a migrated save.
    TUNASWEEPER_API FString Audit(const UTunaSweeperSaveGame& Save,
        const TArray<FTunaSweeperQuestDefinition>& Definitions, FPrerequisiteAudit& OutAudit);
    TUNASWEEPER_API FString Repair(UTunaSweeperSaveGame& Save,
        const TArray<FTunaSweeperQuestDefinition>& Definitions, TArray<int32>& OutCompletedIds);
    // Validates the complete ancestor graph before touching any save state.
    TUNASWEEPER_API FString Apply(UTunaSweeperSaveGame& Save,
        const TArray<FTunaSweeperQuestDefinition>& Definitions, int32 QuestId,
        bool bIncludeTarget, TArray<int32>& OutCompletedIds);
}
#endif
