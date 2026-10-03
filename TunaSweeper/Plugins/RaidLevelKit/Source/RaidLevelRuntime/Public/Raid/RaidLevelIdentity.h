#pragma once
#include "CoreMinimal.h"
#include "Raid/TunaSweeperRaidPlacementAnchor.h"
#include "RaidLevelIdentity.generated.h"

/** Logical profile identity is independent of the physical world package. Game aliases stay in the game adapter. */
USTRUCT(BlueprintType)
struct RAIDLEVELRUNTIME_API FRaidLevelIdentity
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Raid Level") FName PackId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Raid Level") FName MapId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Raid Level") FName LogicalLevelId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Raid Level", meta=(AllowedClasses="/Script/Engine.World")) FSoftObjectPath World;
};

struct RAIDLEVELRUNTIME_API FRaidPlacementDescriptor
{
    int32 PlacementId = 0;
    ETunaSweeperRaidPlacementAnchorKind Kind = ETunaSweeperRaidPlacementAnchorKind::Enemy;
    bool bAllowDuplicatePlacementId = false;
};

/** Callers resolve StringKey in RaidLevelKit.Editor and apply Arguments. No game/localization module is required. */
struct RAIDLEVELRUNTIME_API FRaidLevelValidationIssue
{
    FName StringKey;
    TMap<FString, FString> Arguments;
};

RAIDLEVELRUNTIME_API TArray<FRaidLevelValidationIssue> ValidateRaidPlacementStructure(TConstArrayView<FRaidPlacementDescriptor> Placements);
