#include "Game/TunaSweeperGameInstance.h"

#include "Character/TunaSweeperOutfitCatalog.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperOutfitComponent.h"
#include "Engine/World.h"
#include "Game/TunaSweeperSafeSave.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"

namespace
{
TunaSweeperSafeSave::FSaveValidator MakeOutfitUnlockValidator(FName Flavor, FName Distribution)
{
    return [Flavor, Distribution](const USaveGame& Candidate)
    {
        const auto* Save = Cast<UTunaSweeperCosmeticUnlockSaveGame>(&Candidate);
        return Save && Save->SaveVersion == 1 && Save->BuildFlavor == Flavor && Save->DistributionNamespace == Distribution;
    };
}
}

UTunaSweeperOutfitCatalog* UTunaSweeperGameInstance::GetOutfitCatalog() const
{
    return OutfitCatalog.LoadSynchronous();
}

FName UTunaSweeperGameInstance::GetSelectedOutfitId()
{
    EnsureInventoryStateInitialized();
    const FName Normalized = TunaSweeperOutfits::SanitizePersistedOutfitId(SelectedOutfitId);
    const FName Available = IsOutfitUnlocked(Normalized) ? Normalized : FName(TEXT("Maid"));
    if (SelectedOutfitId != Available)
    {
        SelectedOutfitId = Available;
        OnOutfitChanged.Broadcast();
    }
    return SelectedOutfitId;
}

void UTunaSweeperGameInstance::NotifyOutfitRestoreFallback(FName ExpectedOutfitId)
{
    if (SelectedOutfitId == ExpectedOutfitId && SelectedOutfitId != FName(TEXT("Maid")))
    {
        SelectedOutfitId = TEXT("Maid");
        OnOutfitChanged.Broadcast();
    }
}

bool UTunaSweeperGameInstance::TryEquipOutfit(FName OutfitId)
{
    if (!TunaSweeperOutfits::IsSupportedOutfitId(OutfitId) || !IsOutfitUnlocked(OutfitId)) return false;
    EnsureInventoryStateInitialized();
    const auto* Catalog = GetOutfitCatalog();
    const auto* Definition = Catalog ? Catalog->FindOutfit(OutfitId) : nullptr;
    auto* Controller = GetFirstLocalPlayerController();
    auto* Character = Controller && Controller->IsLocalController()
        ? Cast<ATunaSweeperTopDownCharacter>(Controller->GetPawn()) : nullptr;
    auto* Component = Character ? Character->GetOutfitComponent() : nullptr;
    if (!Definition || !Component || Character->IsDead() || Character->IsMountedInVehicle() || Character->IsRolling()) return false;

    // Roll back the visible appearance as well as the saved selection if the verified save fails.
    const FName PreviousId = SelectedOutfitId;
    const FName PreviousAppliedId = Component->GetAppliedOutfitId();
    FTunaSweeperOutfitDefinition PreviousAppearance;
    PreviousAppearance.OutfitId = TEXT("Maid");
    if (const auto* Previous = Catalog->FindOutfit(PreviousAppliedId)) PreviousAppearance = *Previous;
    if (!Component->ApplyOutfit(*Definition)) return false;
    if (OutfitId == PreviousId) return true;
    SelectedOutfitId = OutfitId;
    const auto SaveMode = bPendingBunkerItemStateSave
        ? EUsableQuickSlotSaveMode::PersistRuntime : EUsableQuickSlotSaveMode::PreserveExisting;
    if (!SaveGameStateInternal(SaveMode))
    {
        SelectedOutfitId = PreviousId;
        Component->ApplyOutfit(PreviousAppearance);
        return false;
    }
    bPendingBunkerItemStateSave = false;
    OnOutfitChanged.Broadcast();
    return true;
}

FName UTunaSweeperGameInstance::GetOutfitUnlockDistributionNamespace() const
{
    // Match the existing account-global achievement namespace and cloud-save root.
    FString Distribution;
    GConfig->GetString(TEXT("TunaSweeperAchievements"), TEXT("DistributionNamespace"), Distribution, GEngineIni);
    Distribution.TrimStartAndEndInline();
    return Distribution.IsEmpty() ? FName(TEXT("Local")) : FName(*Distribution);
}

void UTunaSweeperGameInstance::EnsureOutfitUnlocksLoaded()
{
    if (bOutfitUnlocksLoaded) return;
    bOutfitUnlocksLoaded = true;
    UnlockedOutfitIds.Add(TEXT("Maid"));
    const FName Distribution = GetOutfitUnlockDistributionNamespace();
    if (OutfitUnlockSavePath.IsEmpty())
    {
        OutfitUnlockSavePath = FPaths::Combine(TunaSweeperBuildFlavor::GetSaveGameDirectory(),
            FString::Printf(TEXT("CosmeticUnlocks_%s.sav"), *FPaths::MakeValidFileName(Distribution.ToString())));
    }
    const FString PreviousPath = TunaSweeperSafeSave::GetPreviousFilePath(OutfitUnlockSavePath);
    const bool bExisting = FPaths::FileExists(OutfitUnlockSavePath) || FPaths::FileExists(PreviousPath);
    const auto* Save = Cast<UTunaSweeperCosmeticUnlockSaveGame>(TunaSweeperSafeSave::LoadSaveFileWithRecovery(
        OutfitUnlockSavePath, {PreviousPath}, MakeOutfitUnlockValidator(TunaSweeperBuildFlavor::GetName(), Distribution)));
    bOutfitUnlockSaveBlocked = bExisting && !Save;
    if (Save)
    {
        for (FName Id : Save->UnlockedOutfitIds)
            if (TunaSweeperOutfits::IsSupportedOutfitId(Id)) UnlockedOutfitIds.Add(Id);
    }
}

bool UTunaSweeperGameInstance::IsOutfitUnlocked(FName OutfitId)
{
    if (!TunaSweeperOutfits::IsSupportedOutfitId(OutfitId)) return false;
    EnsureOutfitUnlocksLoaded();
    return OutfitId == FName(TEXT("Maid")) || bUnlockAllOutfitsOverride || UnlockedOutfitIds.Contains(OutfitId);
}

bool UTunaSweeperGameInstance::SaveOutfitUnlocks() const
{
    if (bOutfitUnlockSaveBlocked || OutfitUnlockSavePath.IsEmpty()) return false;
    auto* Save = NewObject<UTunaSweeperCosmeticUnlockSaveGame>();
    Save->BuildFlavor = TunaSweeperBuildFlavor::GetName();
    Save->DistributionNamespace = GetOutfitUnlockDistributionNamespace();
    Save->UnlockedOutfitIds = UnlockedOutfitIds.Array();
    Save->UnlockedOutfitIds.Sort([](FName A, FName B) { return A.LexicalLess(B); });
    return TunaSweeperSafeSave::SaveGameFileFailClosed(Save, OutfitUnlockSavePath,
        MakeOutfitUnlockValidator(Save->BuildFlavor, Save->DistributionNamespace));
}

bool UTunaSweeperGameInstance::TryUnlockOutfit(FName OutfitId)
{
    if (!TunaSweeperOutfits::IsSupportedOutfitId(OutfitId)) return false;
    EnsureOutfitUnlocksLoaded();
    // The temporary override must not make a real unlock request a no-op.
    if (UnlockedOutfitIds.Contains(OutfitId)) return true;
    UnlockedOutfitIds.Add(OutfitId);
    if (!SaveOutfitUnlocks())
    {
        UnlockedOutfitIds.Remove(OutfitId);
        return false;
    }
    OnOutfitUnlocksChanged.Broadcast();
    return true;
}
