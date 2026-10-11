#include "Interaction/TunaSweeperRegionLinkActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

ATunaSweeperRegionLinkActor::ATunaSweeperRegionLinkActor()
{
}
void ATunaSweeperRegionLinkActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshPassage();
}
void ATunaSweeperRegionLinkActor::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::RefreshPassage);
        Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshPassage);
        Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshPassage);
    }
    RefreshPassage();
}
void ATunaSweeperRegionLinkActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->OnWorldProgressChanged.RemoveAll(this);
        Game->OnInventoryStateChanged.RemoveAll(this);
        Game->OnLanguageChanged.RemoveAll(this);
    }
    Super::EndPlay(Reason);
}
bool ATunaSweeperRegionLinkActor::IsPassageUnlocked() const
{
    if (RequiredProgressId.IsNone()) return true;
    const auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    FTunaSweeperWorldProgressSaveData State;
    return Game && Game->TryGetWorldProgressState(RequiredProgressId, State) && State.State == ETunaSweeperWorldProgressState::Completed;
}
bool ATunaSweeperRegionLinkActor::RepairPassage(bool bSaveImmediately)
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game || bRepairing || RequiredProgressId.IsNone() || RepairParts.IsEmpty() || IsPassageUnlocked()) return false;
    TGuardValue<bool> Guard(bRepairing, true);
    const bool bRepaired = Game->TryConsumeInventoryItems(RepairParts, [this, Game]()
    {
        Game->UpdateWorldProgressState(RequiredProgressId, TEXT("world_progress.region_passage"), ETunaSweeperWorldProgressState::Completed, 1, 1, false);
    });
    if (bRepaired && bSaveImmediately) Game->SaveGameState();
    RefreshPassage();
    return bRepaired;
}
void ATunaSweeperRegionLinkActor::RefreshPassage()
{
    if (!InteractableComponent) return;
    const bool bUnlocked = IsPassageUnlocked();
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    FText Label;
    FName Key = TravelStringKey;
    auto Type = ETunaSweeperInteractionType::LevelTravel;
    if (!bUnlocked)
    {
        Type = RepairParts.IsEmpty() ? ETunaSweeperInteractionType::None : ETunaSweeperInteractionType::WorldProgress;
        Key = TEXT("ui.interaction.repair");
        if (Game && !RepairParts.IsEmpty())
        {
            TArray<FText> PartLabels;
            auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
            for (const auto& Part : RepairParts)
            {
                FText Name;
                if (Items) Items->TryGetItemNameText(Part.ItemId, Game->GetCurrentTextLanguage(), Name);
                PartLabels.Add(FText::Format(Game->ResolveLocalizedText(TEXT("ui.passage.part"), FText::GetEmpty()), Name, FText::AsNumber(Part.Quantity)));
            }
            Label = FText::Format(Game->ResolveLocalizedText(TEXT("ui.passage.repair"), FText::GetEmpty()), FText::Join(Game->ResolveLocalizedText(TEXT("ui.passage.separator"), FText::GetEmpty()), PartLabels));
            Key = NAME_None; // Already localized, with item names and quantities formatted above.
        }
    }
    if (Game && Label.IsEmpty()) Label = Game->ResolveLocalizedText(Key, FText::GetEmpty());
    InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(Type, Label, Key);
    InteractableComponent->SetMarkerCompleted(false);
}
bool ATunaSweeperRegionLinkActor::TravelToTargetLevel(APawn* Pawn)
{
    if (bTravelStarted || !IsPassageUnlocked()) return false;
    bTravelStarted = Super::TravelToTargetLevel(Pawn);
    return bTravelStarted;
}
