#include "Interaction/TunaSweeperNpcBarterComponent.h"
#include "Character/TunaSweeperFacilityNpcActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"

UTunaSweeperNpcBarterComponent::UTunaSweeperNpcBarterComponent()
{
    InteractionType = ETunaSweeperInteractionType::None;
    InteractionDisplayName = FText::GetEmpty();
    InteractionOrder = 10;
}

bool UTunaSweeperNpcBarterComponent::IsUnlocked() const
{
    if (RequiredQuestId <= 0 || InputItemId <= 0 || OutputItemId <= 0 || InputQuantity <= 0 ||
        OutputQuantity <= 0 || OfferStringKey.IsNone()) return false;
    const auto* Npc = Cast<ATunaSweeperFacilityNpcActor>(GetOwner());
    if (!IsValid(Npc) || Npc->IsHidden() || Npc->IsActorBeingDestroyed() || Npc->GetQuestProviderId().IsNone()) return false;
    const auto* Game = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
    const auto* Quest = Quests ? Quests->FindQuestDefinition(RequiredQuestId) : nullptr;
    return Quest && Quest->GetCompletionProviderId() == Npc->GetQuestProviderId() &&
        Quests->GetQuestState(RequiredQuestId) == ETunaSweeperQuestState::RewardCompleted;
}

void UTunaSweeperNpcBarterComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = GetWorld()->GetGameInstance())
        if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
            Quests->OnQuestProgressChanged.AddUObject(this, &ThisClass::RefreshAvailability);
    RefreshAvailability();
}

void UTunaSweeperNpcBarterComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Game = GetWorld()->GetGameInstance())
        if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
            Quests->OnQuestProgressChanged.RemoveAll(this);
    Super::EndPlay(Reason);
}

void UTunaSweeperNpcBarterComponent::RefreshAvailability()
{
    SetInteractionTypeDisplayNameAndStringKey(IsUnlocked() ? ETunaSweeperInteractionType::NpcBarter : ETunaSweeperInteractionType::None,
        FText::GetEmpty(), OfferStringKey);
}

bool UTunaSweeperNpcBarterComponent::TryBarter(APawn* Pawn, bool bSaveImmediately)
{
    if (bExchanging || !IsValid(Pawn) || !Pawn->IsPlayerControlled() || Pawn->GetWorld() != GetWorld() ||
        !IsUnlocked() || !IsWithinInteractionDistance(Pawn)) return false;
    auto* Game = Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance());
    if (!Game || Game->IsCombatTestSession() || Game->IsQuestRewardTransferInProgress()) return false;
    TGuardValue<bool> Guard(bExchanging, true);
    if (Game->CountInventoryItemById(InputItemId) < InputQuantity)
    {
        ShowBubble(ShortageStringKey);
        return true;
    }
    if (!Game->ExchangeCarriedItemBatch(InputItemId, InputQuantity, OutputItemId, OutputQuantity, bSaveImmediately))
        ShowBubble(FailureStringKey);
    return true;
}

void UTunaSweeperNpcBarterComponent::ShowBubble(FName Key)
{
    auto* Game = Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance());
    if (!Game || Key.IsNone()) return;
    if (auto* Speech = Game->GetSubsystem<UTunaSweeperSpeechBubbleSubsystem>())
    {
        const FText Text = Game->ResolveLocalizedText(Key, FText::GetEmpty());
        if (!Text.IsEmpty()) Speech->ShowForActor(Text, GetOwner(), FVector(0, 0, 20),
            ETunaSweeperSpeechBubbleTailDirection::Down, 2.5f, -28.0f);
    }
}
