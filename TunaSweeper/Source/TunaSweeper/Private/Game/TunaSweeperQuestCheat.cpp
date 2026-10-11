#include "Inventory/TunaSweeperQuestCheat.h"
#if WITH_EDITOR
#include "Inventory/TunaSweeperSaveGame.h"

FString TunaSweeperQuestCheat::Apply(UTunaSweeperSaveGame& Save,
    const TArray<FTunaSweeperQuestDefinition>& Definitions, int32 QuestId,
    bool bIncludeTarget, TArray<int32>& OutCompletedIds)
{
    OutCompletedIds.Reset();
    TMap<int32, const FTunaSweeperQuestDefinition*> Catalog;
    for (const auto& Definition : Definitions)
    {
        if (Definition.QuestId <= 0 || Catalog.Contains(Definition.QuestId)) return TEXT("quest_catalog_invalid");
        Catalog.Add(Definition.QuestId, &Definition);
    }
    if (QuestId <= 0 || !Catalog.Contains(QuestId)) return TEXT("quest_not_found");
    // Iterative DFS avoids stack overflow for long authored chains.
    struct FVisit { int32 Id; bool bExit; };
    TArray<FVisit> Stack = {{QuestId, false}};
    TSet<int32> Visiting, Visited;
    TArray<int32> Plan;
    while (!Stack.IsEmpty())
    {
        const FVisit Visit = Stack.Pop(EAllowShrinking::No);
        if (Visit.bExit)
        {
            Visiting.Remove(Visit.Id);
            Visited.Add(Visit.Id);
            if (bIncludeTarget || Visit.Id != QuestId) Plan.Add(Visit.Id);
            continue;
        }
        if (Visited.Contains(Visit.Id)) continue;
        if (Visiting.Contains(Visit.Id)) return TEXT("quest_cycle");
        const auto* Found = Catalog.Find(Visit.Id);
        if (!Found) return TEXT("quest_prerequisite_missing");
        Visiting.Add(Visit.Id);
        Stack.Add({Visit.Id, true});
        for (int32 Parent : (*Found)->RequiredCompletedQuestIds) Stack.Add({Parent, false});
    }
    Save.MigrateLegacyQuestIds();
    for (int32 Id : Plan)
    {
        // Canonicalize the edited IDs even when an old save contains duplicates.
        Save.NumericQuestProgressStates.RemoveAll([Id](const auto& Row) { return Row.QuestId == Id; });
        auto& Progress = Save.NumericQuestProgressStates.AddDefaulted_GetRef();
        Progress.QuestId = Id;
        Progress.State = ETunaSweeperQuestState::RewardCompleted;
        for (const auto& Objective : Catalog[Id]->Objectives)
        {
            auto& Count = Progress.ObjectiveProgress.AddDefaulted_GetRef();
            Count.ObjectiveId = Objective.ObjectiveId;
            Count.CurrentCount = FMath::Max(1, Objective.RequiredCount);
        }
        if (Save.NumericTrackedQuestId == Id) Save.NumericTrackedQuestId = 0;
    }
    OutCompletedIds = MoveTemp(Plan);
    return {};
}
#endif
