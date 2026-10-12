#include "Inventory/TunaSweeperQuestCheat.h"
#if WITH_EDITOR
#include "Inventory/TunaSweeperSaveGame.h"

FString TunaSweeperQuestCheat::Audit(const UTunaSweeperSaveGame& Save,
    const TArray<FTunaSweeperQuestDefinition>& Definitions, FPrerequisiteAudit& OutAudit)
{
    OutAudit = {};
    TMap<int32, const FTunaSweeperQuestDefinition*> Catalog;
    for (const auto& Definition : Definitions)
    {
        if (Definition.QuestId <= 0 || Catalog.Contains(Definition.QuestId)) return TEXT("quest_catalog_invalid");
        Catalog.Add(Definition.QuestId, &Definition);
    }
    TMap<int32, ETunaSweeperQuestState> States;
    for (const auto& Progress : Save.NumericQuestProgressStates) States.Add(Progress.QuestId, Progress.State);
    auto StateOf = [&](int32 Id) { const auto* State = States.Find(Id); return State ? *State : ETunaSweeperQuestState::Available; };
    TSet<FString> Reported;
    TSet<int32> ToFix, Visiting, Visited;
    bool bInvalidGraph = false;
    auto Issue = [&](int32 Id, int32 ParentId, const TCHAR* Code)
    {
        const FString Key = FString::Printf(TEXT("%d:%d:%s"), Id, ParentId, Code);
        if (Reported.Contains(Key)) return;
        Reported.Add(Key);
        OutAudit.Issues.Add({Id, ParentId, Code, StateOf(ParentId)});
    };
    struct FVisit { int32 Id; int32 FromId; bool bExit; };
    for (const auto& Entry : States)
    {
        if (Entry.Value == ETunaSweeperQuestState::Available) continue;
        TArray<FVisit> Stack = {{Entry.Key, 0, false}};
        while (!Stack.IsEmpty())
        {
            const auto Visit = Stack.Pop(EAllowShrinking::No);
            if (Visit.bExit) { Visiting.Remove(Visit.Id); Visited.Add(Visit.Id); continue; }
            const auto* Definition = Catalog.Find(Visit.Id);
            if (!Definition)
            {
                Issue(Visit.FromId ? Visit.FromId : Visit.Id, Visit.FromId ? Visit.Id : 0,
                    Visit.FromId ? TEXT("quest_prerequisite_missing") : TEXT("quest_not_found"));
                bInvalidGraph = true;
                continue;
            }
            if (Visit.FromId && StateOf(Visit.Id) != ETunaSweeperQuestState::RewardCompleted)
            {
                Issue(Visit.FromId, Visit.Id, TEXT("quest_prerequisite_incomplete"));
                ToFix.Add(Visit.Id);
            }
            if (Visiting.Contains(Visit.Id))
            {
                Issue(Visit.FromId, Visit.Id, TEXT("quest_cycle"));
                bInvalidGraph = true;
                continue;
            }
            if (Visited.Contains(Visit.Id)) continue;
            Visiting.Add(Visit.Id);
            Stack.Add({Visit.Id, Visit.FromId, true});
            for (int32 ParentId : (*Definition)->RequiredCompletedQuestIds) Stack.Add({ParentId, Visit.Id, false});
        }
    }
    OutAudit.Issues.Sort([](const auto& A, const auto& B)
    {
        if (A.QuestId != B.QuestId) return A.QuestId < B.QuestId;
        if (A.PrerequisiteId != B.PrerequisiteId) return A.PrerequisiteId < B.PrerequisiteId;
        return A.Code < B.Code;
    });
    if (!bInvalidGraph) { OutAudit.FixQuestIds = ToFix.Array(); OutAudit.FixQuestIds.Sort(); }
    OutAudit.bCanFix = !bInvalidGraph && !OutAudit.FixQuestIds.IsEmpty();
    return {};
}

FString TunaSweeperQuestCheat::Repair(UTunaSweeperSaveGame& Save,
    const TArray<FTunaSweeperQuestDefinition>& Definitions, TArray<int32>& OutCompletedIds)
{
    OutCompletedIds.Reset();
    FPrerequisiteAudit Report;
    const FString Error = Audit(Save, Definitions, Report);
    if (!Error.IsEmpty()) return Error;
    if (!Report.Issues.IsEmpty() && !Report.bCanFix) return TEXT("quest_audit_unfixable");
    // Validate the entire batch first, then mutate only missing prerequisites.
    for (int32 Id : Report.FixQuestIds)
    {
        const auto* Definition = Definitions.FindByPredicate([Id](const auto& D) { return D.QuestId == Id; });
        Save.NumericQuestProgressStates.RemoveAll([Id](const auto& Row) { return Row.QuestId == Id; });
        auto& Progress = Save.NumericQuestProgressStates.AddDefaulted_GetRef();
        Progress.QuestId = Id;
        Progress.State = ETunaSweeperQuestState::RewardCompleted;
        for (const auto& Objective : Definition->Objectives)
        {
            auto& Count = Progress.ObjectiveProgress.AddDefaulted_GetRef();
            Count.ObjectiveId = Objective.ObjectiveId;
            Count.CurrentCount = FMath::Max(1, Objective.RequiredCount);
        }
        if (Save.NumericTrackedQuestId == Id) Save.NumericTrackedQuestId = 0;
    }
    OutCompletedIds = MoveTemp(Report.FixQuestIds);
    return {};
}

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
