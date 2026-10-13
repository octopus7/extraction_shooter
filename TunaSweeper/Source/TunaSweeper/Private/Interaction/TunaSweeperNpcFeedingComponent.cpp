#include "Interaction/TunaSweeperNpcFeedingComponent.h"
#include "Character/TunaSweeperQuestPresenceNpcActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"

UTunaSweeperNpcFeedingComponent::UTunaSweeperNpcFeedingComponent()
{
    InteractionType = ETunaSweeperInteractionType::None;
    InteractionDisplayName = FText::GetEmpty();
    InteractionDisplayNameStringKey = TEXT("ui.interaction.feed");
    InteractionOrder = 10;
}

bool UTunaSweeperNpcFeedingComponent::IsUnlocked() const
{
    if (TunaSweeperBuildFlavor::IsDemo() || RequiredQuestId <= 0 || FoodRewards.IsEmpty()) return false;
    const auto* Npc = Cast<ATunaSweeperQuestPresenceNpcActor>(GetOwner());
    if (!Npc || Npc->PresenceRole != ETunaSweeperQuestNpcPresenceRole::BaseResident || Npc->IsHidden()) return false;
    const auto* Game = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
    return Quests && Quests->FindQuestDefinition(RequiredQuestId) &&
        Quests->GetQuestState(RequiredQuestId) == ETunaSweeperQuestState::RewardCompleted;
}

void UTunaSweeperNpcFeedingComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = GetWorld()->GetGameInstance())
        if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
            Quests->OnQuestProgressChanged.AddUObject(this, &ThisClass::RefreshAvailability);
    RefreshAvailability();
}

void UTunaSweeperNpcFeedingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorld()->GetTimerManager().ClearTimer(HungryTimer);
    if (auto* Game = GetWorld()->GetGameInstance())
        if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
            Quests->OnQuestProgressChanged.RemoveAll(this);
    Super::EndPlay(Reason);
}

void UTunaSweeperNpcFeedingComponent::RefreshAvailability()
{
    SetInteractionTypeDisplayNameAndStringKey(IsUnlocked() ? ETunaSweeperInteractionType::NpcFeeding : ETunaSweeperInteractionType::None,
        FText::GetEmpty(), TEXT("ui.interaction.feed"));
}

bool UTunaSweeperNpcFeedingComponent::TryFeed(APawn* Pawn)
{
    if (bFeeding || !IsValid(Pawn) || !IsUnlocked() || !IsWithinInteractionDistance(Pawn)) return false;
    auto* Game = Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance());
    if (!Game || Game->IsCombatTestSession() || Game->IsQuestRewardTransferInProgress()) return false;
    TGuardValue<bool> Guard(bFeeding, true);
    TArray<int32> Available;
    for (const auto& Pair : FoodRewards)
        if (Game->CountInventoryItemById(Pair.Key) > 0) Available.Add(Pair.Key);
    if (Available.IsEmpty())
    {
        // Repeated presses must not keep postponing the reply indefinitely.
        if (!GetWorld()->GetTimerManager().IsTimerActive(HungryTimer))
        {
            ShowBubble(Pawn, NoFoodStringKey);
            GetWorld()->GetTimerManager().SetTimer(HungryTimer, this, &ThisClass::ShowHungryBubble, 1.0f, false);
        }
        return true;
    }
    const int32 Food = Available[FMath::RandHelper(Available.Num())];
    if (!Game->ExchangeCarriedItemForReward(Food, FoodRewards[Food])) return false;
    GetWorld()->GetTimerManager().ClearTimer(HungryTimer);
    return true;
}

void UTunaSweeperNpcFeedingComponent::ShowHungryBubble()
{
    if (IsUnlocked()) ShowBubble(GetOwner(), HungryStringKey);
}

void UTunaSweeperNpcFeedingComponent::ShowBubble(AActor* Speaker, FName Key)
{
    auto* Game = Cast<UTunaSweeperGameInstance>(GetWorld()->GetGameInstance());
    if (!Game || !IsValid(Speaker) || Key.IsNone()) return;
    auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    auto* Speech = Game->GetSubsystem<UTunaSweeperSpeechBubbleSubsystem>();
    FText Text;
    if (Quests && Speech && Quests->TryGetQuestTextByKey(Key, Game->GetCurrentTextLanguage(), Text))
        Speech->ShowForActor(Text, Speaker, FVector(0, 0, 20), ETunaSweeperSpeechBubbleTailDirection::Down, 2.0f, -28.0f);
}
