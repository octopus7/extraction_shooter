#include "Interaction/TunaSweeperEthanolFacilityActor.h"

#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"

void ATunaSweeperEthanolFacilityActor::BeginPlay()
{
    // Older saves may have claimed the repair quest before facilities recorded world completion.
    // Explicit saved facility state always wins, including an incomplete repair.
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>(); Game && !ProgressObjectId.IsNone() && RepairQuestId > 0)
    {
        Game->GetInventorySlots(); // Load the existing save before deciding whether a record is missing.
        FTunaSweeperWorldProgressSaveData Existing;
        auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
        if (!Game->TryGetWorldProgressState(ProgressObjectId, Existing) && Quests &&
            Quests->GetQuestState(RepairQuestId) == ETunaSweeperQuestState::RewardCompleted)
            Game->CommitPowerFacilityRepair(ProgressObjectId, {}, false, true);
    }
    Super::BeginPlay();
}

void ATunaSweeperEthanolFacilityActor::RefreshFacilityState()
{
    Super::RefreshFacilityState();
    if (!IsRepaired()) return;
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    Interactable->SetInteractionTypeDisplayNameAndStringKey(ETunaSweeperInteractionType::WorldProgress,
        Game ? Game->ResolveLocalizedText(ProductionStringKey, FText::GetEmpty()) : FText::GetEmpty(), ProductionStringKey);
    Interactable->SetObjectiveEventId(NAME_None);
    Interactable->SetMarkerCompleted(false);
    UTexture2D* Icon = nullptr;
    FTunaSweeperItemDefinition InputDefinition;
    if (auto* Items = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
        Items && Items->TryGetItemDefinition(ProductionInputItemId, InputDefinition))
    {
        const FString IconPath = Items->BuildItemIconObjectPath(InputDefinition);
        if (!IconPath.IsEmpty()) Icon = LoadObject<UTexture2D>(nullptr, *IconPath);
    }
    Interactable->SetInteractionRequirementPreview(Icon, ProductionInputQuantity, ProductionInputQuantity > 0);
}

bool ATunaSweeperEthanolFacilityActor::Produce(APawn* InstigatorPawn, bool bSaveImmediately)
{
    RefreshFacilityState();
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game || !IsRepaired() || bProductionInProgress) return false;
    TGuardValue<bool> ProductionGuard(bProductionInProgress, true);
    if (ProductionInputItemId > 0 && ProductionInputQuantity > 0 &&
        Game->CountInventoryItemById(ProductionInputItemId) < ProductionInputQuantity)
    {
        if (auto* Speech = Game->GetSubsystem<UTunaSweeperSpeechBubbleSubsystem>(); Speech && InstigatorPawn)
            Speech->ShowForActor(Game->ResolveLocalizedText(MissingMaterialStringKey, FText::GetEmpty()),
                InstigatorPawn, FVector(0, 0, 110), ETunaSweeperSpeechBubbleTailDirection::Down, 2.0f);
        return false;
    }
    const bool bProduced = Game->ProduceFacilityBatch(ProgressObjectId, ProductionInputItemId, ProductionInputQuantity,
        ProductionOutputItemId, ProductionOutputQuantity, bSaveImmediately);
    RefreshFacilityState();
    return bProduced;
}
