#include "Character/TunaSweeperOutfitCatalog.h"

bool TunaSweeperOutfits::IsSupportedOutfitId(FName OutfitId)
{
    static const FName Ids[] = {TEXT("Maid"), TEXT("SchoolUniform"), TEXT("MechanicOutfit"),
        TEXT("Sportswear"), TEXT("BunnyPajamas"), TEXT("AdventurerOutfit"), TEXT("Raincoat")};
    for (FName Id : Ids) if (OutfitId == Id) return true;
    return false;
}

FName TunaSweeperOutfits::SanitizePersistedOutfitId(FName OutfitId)
{
    return IsSupportedOutfitId(OutfitId) ? OutfitId : FName(TEXT("Maid"));
}

const FTunaSweeperOutfitDefinition* UTunaSweeperOutfitCatalog::FindOutfit(FName OutfitId) const
{
    if (!TunaSweeperOutfits::IsSupportedOutfitId(OutfitId)) return nullptr;
    return Outfits.FindByPredicate([OutfitId](const FTunaSweeperOutfitDefinition& Definition)
    {
        return Definition.OutfitId == OutfitId;
    });
}
