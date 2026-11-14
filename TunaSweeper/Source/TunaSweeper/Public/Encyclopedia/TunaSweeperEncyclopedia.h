#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperEncyclopedia.generated.h"

class UTunaSweeperGameInstance;
class UTexture2D;

UENUM(BlueprintType)
enum class EEncyclopediaCategory : uint8 { Enemy, Weapon, Ammunition, Excluded };

USTRUCT(BlueprintType)
struct FEncyclopediaEnemyDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName Kind;
    UPROPERTY(EditAnywhere) TSoftClassPtr<AActor> EnemyClass;
    UPROPERTY(EditAnywhere) FName NameKey;
    UPROPERTY(EditAnywhere) FName DescriptionKey;
    UPROPERTY(EditAnywhere) FName BehaviorKey;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UTexture2D> Portrait;
};

/** Authored asset, including class-to-kind mapping and portraits. Most-specific class wins. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UEncyclopediaEnemyCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) TArray<FEncyclopediaEnemyDefinition> Enemies;
    FName ResolveKind(const UClass* EnemyClass) const;
    static UEncyclopediaEnemyCatalog* Get();
};

/** View data only: the list view instantiates the saved entry WBP. */
UCLASS()
class TUNASWEEPER_API UEncyclopediaEntryData : public UObject
{
    GENERATED_BODY()
public:
    EEncyclopediaCategory Category = EEncyclopediaCategory::Excluded;
    FName Group;
    FName EnemyKind;
    int32 ItemId = INDEX_NONE;
    ETunaSweeperItemGrade Grade = ETunaSweeperItemGrade::Common;
    bool bUnlocked = false;
    FText Name, Description, Details, GroupName, GradeName;
    FString SortName;
    UPROPERTY() TSoftObjectPtr<UTexture2D> Portrait;
};

namespace TunaSweeperEncyclopedia
{
    TUNASWEEPER_API EEncyclopediaCategory Classify(const FTunaSweeperItemDefinition& Item);
    TUNASWEEPER_API FText Text(UTunaSweeperGameInstance* Game, FName Key);
    TUNASWEEPER_API void BuildEntries(UTunaSweeperGameInstance* Game, UObject* Outer,
        EEncyclopediaCategory Category, TArray<UEncyclopediaEntryData*>& Out);
}
