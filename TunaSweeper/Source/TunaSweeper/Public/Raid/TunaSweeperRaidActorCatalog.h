#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperRaidActorCatalog.generated.h"

/** Trusted, cooked game data only. No external-pack deserializer consumes this schema. */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperRaidActorProperty
{
    GENERATED_BODY()
    /** None addresses the actor; otherwise the final component's exact object name. */
    UPROPERTY(EditAnywhere) FName ComponentName;
    UPROPERTY(EditAnywhere) FName PropertyName;
    UPROPERTY(EditAnywhere) FString Value;
};
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperRaidActorTarget
{
    GENERATED_BODY()
    /** Exactly one of PlacementId or EnvironmentActorName is required. */
    UPROPERTY(EditAnywhere) int32 PlacementId = 0;
    UPROPERTY(EditAnywhere) FName EnvironmentActorName;
    UPROPERTY(EditAnywhere) FName ComponentName;
};
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperRaidActorReference
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName ComponentName;
    UPROPERTY(EditAnywhere) FName PropertyName;
    /** INDEX_NONE for a scalar object property, otherwise an existing object-array element. */
    UPROPERTY(EditAnywhere) int32 ArrayIndex = INDEX_NONE;
    UPROPERTY(EditAnywhere) FTunaSweeperRaidActorTarget Target;
};
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperRaidActorProfile
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName ProfileId;
    UPROPERTY(EditAnywhere) TSoftClassPtr<AActor> ActorClass;
    /** Original source actor name is a save identity, never silently uniquified. */
    UPROPERTY(EditAnywhere) FName ActorName;
    UPROPERTY(EditAnywhere) TArray<FTunaSweeperRaidActorProperty> Properties;
    /** Cooker-visible dependencies of asset references encoded in Properties.Value. */
    UPROPERTY(EditAnywhere) TArray<TSoftObjectPtr<UObject>> ReferencedAssets;
    UPROPERTY(EditAnywhere) TArray<FTunaSweeperRaidActorReference> References;
    UPROPERTY(EditAnywhere) bool bHasAttachment = false;
    UPROPERTY(EditAnywhere) FTunaSweeperRaidActorTarget AttachmentParent;
    UPROPERTY(EditAnywhere) FName AttachmentSocket;
};
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperRaidActorPlacement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) int32 PlacementId = 0;
    UPROPERTY(EditAnywhere) FName ProfileId;
};
UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperRaidActorCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    /** Exact physical long package name (PIE prefix stripped); never a logical alias. */
    UPROPERTY(EditAnywhere) FName MapId;
    UPROPERTY(EditAnywhere) TArray<FTunaSweeperRaidActorPlacement> Placements;
    UPROPERTY(EditAnywhere) TArray<FTunaSweeperRaidActorProfile> Profiles;
};