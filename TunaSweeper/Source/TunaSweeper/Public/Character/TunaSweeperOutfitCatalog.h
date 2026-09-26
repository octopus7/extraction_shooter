#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperOutfitCatalog.generated.h"

class USkeletalMesh;
class UTexture2D;

namespace TunaSweeperOutfits
{
    TUNASWEEPER_API bool IsSupportedOutfitId(FName OutfitId);
    TUNASWEEPER_API FName SanitizePersistedOutfitId(FName OutfitId);
}

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperOutfitDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit")
    FName OutfitId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit")
    FName DisplayNameStringKey;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit")
    TSoftObjectPtr<UTexture2D> Thumbnail;

    /** Maid uses the player's original cached body and skirt. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit")
    TSoftObjectPtr<USkeletalMesh> BodyMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit")
    TSoftObjectPtr<USkeletalMesh> ClothingMesh;
};

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperOutfitCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfits")
    TArray<FTunaSweeperOutfitDefinition> Outfits;

    const FTunaSweeperOutfitDefinition* FindOutfit(FName OutfitId) const;
};
