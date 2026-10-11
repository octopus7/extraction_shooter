#pragma once
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "Quest/TunaSweeperQuestTypes.h"
class UTunaSweeperSaveGame;
namespace TunaSweeperQuestCheat
{
    // Validates the complete ancestor graph before touching any save state.
    TUNASWEEPER_API FString Apply(UTunaSweeperSaveGame& Save,
        const TArray<FTunaSweeperQuestDefinition>& Definitions, int32 QuestId,
        bool bIncludeTarget, TArray<int32>& OutCompletedIds);
}
#endif
