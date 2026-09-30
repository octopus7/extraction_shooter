#include "Raid/RaidLevelIdentity.h"

TArray<FRaidLevelValidationIssue> ValidateRaidPlacementStructure(TConstArrayView<FRaidPlacementDescriptor> Placements)
{
    TArray<FRaidLevelValidationIssue> Issues;
    TMap<int32, TArray<const FRaidPlacementDescriptor*>> Groups;
    auto AddIssue = [&Issues](const TCHAR* Key, int32 Id)
    {
        FRaidLevelValidationIssue& Issue = Issues.AddDefaulted_GetRef();
        Issue.StringKey = Key;
        Issue.Arguments.Add(TEXT("PlacementId"), FString::FromInt(Id));
    };
    for (const FRaidPlacementDescriptor& Placement : Placements)
    {
        if (Placement.PlacementId <= 0)
        {
            AddIssue(TEXT("Validation.InvalidPlacementId"), Placement.PlacementId);
            continue;
        }
        if (Placement.Kind != ETunaSweeperRaidPlacementAnchorKind::Enemy && Placement.Kind != ETunaSweeperRaidPlacementAnchorKind::LootContainer && Placement.Kind != ETunaSweeperRaidPlacementAnchorKind::Memo && Placement.Kind != ETunaSweeperRaidPlacementAnchorKind::AuthoredActor)
        {
            AddIssue(TEXT("Validation.InvalidAnchorKind"), Placement.PlacementId);
        }
        Groups.FindOrAdd(Placement.PlacementId).Add(&Placement);
    }
    for (const auto& Group : Groups)
    {
        if (Group.Value.Num() > 1 && Group.Value.ContainsByPredicate([](const FRaidPlacementDescriptor* Placement)
            { return Placement->Kind != ETunaSweeperRaidPlacementAnchorKind::Enemy || !Placement->bAllowDuplicatePlacementId; }))
        {
            AddIssue(TEXT("Validation.DuplicatePlacementId"), Group.Key);
        }
    }
    return Issues;
}
