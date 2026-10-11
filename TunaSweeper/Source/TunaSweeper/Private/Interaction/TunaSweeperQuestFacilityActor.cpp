#include "Interaction/TunaSweeperQuestFacilityActor.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperQuestFacilityActor::ATunaSweeperQuestFacilityActor()
{
	PrimaryActorTick.bCanEverTick = false;
	VisualMesh->SetRelativeTransform(FTransform::Identity);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Piglet(
		TEXT("/Game/Interaction/QuestFacility/SM_QuestFacilityPiglet.SM_QuestFacilityPiglet"));
	VisualMesh->SetStaticMesh(Piglet.Object);
	InteractableComponent->SetRelativeLocation(FVector(0, 0, 75));
	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		ETunaSweeperInteractionType::None, FText::GetEmpty(), NAME_None);
}

void ATunaSweeperQuestFacilityActor::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshQuestState);
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.AddUObject(this, &ThisClass::RefreshQuestState);
	}
	RefreshQuestState();
}

void ATunaSweeperQuestFacilityActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnLanguageChanged.RemoveAll(this);
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.RemoveAll(this);
	}
	Super::EndPlay(Reason);
}

bool ATunaSweeperQuestFacilityActor::IsFacilityUnlocked() const
{
	const auto* Game = GetGameInstance();
	const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	if (!Quests || UnlockConditions.IsEmpty()) return false;
	for (const auto& Condition : UnlockConditions)
	{
		if (Condition.QuestId <= 0 || !Quests->FindQuestDefinition(Condition.QuestId) ||
			!Condition.AllowedStates.Contains(Quests->GetQuestState(Condition.QuestId))) return false;
	}
	return true;
}

void ATunaSweeperQuestFacilityActor::RefreshQuestState()
{
	const bool bUnlocked = IsFacilityUnlocked();
	auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
	const FText Label = bUnlocked && Game && !InteractionStringKey.IsNone()
		? Game->ResolveLocalizedText(InteractionStringKey, FText::GetEmpty()) : FText::GetEmpty();
	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		bUnlocked ? ETunaSweeperInteractionType::QuestFacilityUse : ETunaSweeperInteractionType::None,
		Label, bUnlocked ? InteractionStringKey : NAME_None);
	InteractableComponent->SetObjectiveEventId(bUnlocked ? InteractionObjectiveEventId : NAME_None);
}

bool ATunaSweeperQuestFacilityActor::TryUseFacility(APawn* InstigatorPawn)
{
	if (!IsValid(InstigatorPawn) || InstigatorPawn->GetWorld() != GetWorld() || IsHidden() ||
		bInteractionInProgress || !IsFacilityUnlocked() || !IsWithinInteractionDistance(InstigatorPawn)) return false;
	if (const auto* Player = Cast<ATunaSweeperTopDownCharacter>(InstigatorPawn);
		Player && (Player->IsDead() || Player->IsMountedInVehicle())) return false;
	TGuardValue<bool> Guard(bInteractionInProgress, true);
	if (!ExecuteFacilityInteraction(InstigatorPawn)) return false;
	OnFacilityUsed.Broadcast(InstigatorPawn);
	return true;
}

bool ATunaSweeperQuestFacilityActor::ExecuteFacilityInteraction_Implementation(APawn* InstigatorPawn)
{
	return true;
}
