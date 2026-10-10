#include "Character/TunaSweeperQuestPresenceNpcActor.h"
#include "Component/TunaSweeperQuestMarkerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperQuestPresenceNpcActor::ATunaSweeperQuestPresenceNpcActor()
{
	BodyMesh->SetStaticMesh(nullptr);
	BodyMesh->SetRelativeTransform(FTransform::Identity);
	HeadMesh->SetStaticMesh(nullptr);
	BodyCollision->SetCapsuleSize(30.0f, 32.0f);
	BodyCollision->SetRelativeLocation(FVector(0, 0, 32));
	QuestInteractableComponent->SetRelativeLocation(FVector(0, 0, 60));
	QuestMarkerComponent->SetMarkerHeight(85.0f);
}

bool ATunaSweeperQuestPresenceNpcActor::EvaluatePresence(
	ETunaSweeperQuestNpcPresenceRole Role, bool bKnownQuest, bool bCanAccept, ETunaSweeperQuestState State)
{
	if (!bKnownQuest) return false;
	if (Role == ETunaSweeperQuestNpcPresenceRole::FieldOffer)
		return State == ETunaSweeperQuestState::Available && bCanAccept;
	return Role == ETunaSweeperQuestNpcPresenceRole::BaseResident && State == ETunaSweeperQuestState::RewardCompleted;
}

void ATunaSweeperQuestPresenceNpcActor::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* Game = GetGameInstance())
	{
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.AddUObject(this, &ATunaSweeperQuestPresenceNpcActor::RefreshQuestPresence);
	}
	RefreshQuestPresence();
}

void ATunaSweeperQuestPresenceNpcActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* Game = GetGameInstance())
	{
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperQuestPresenceNpcActor::RefreshQuestPresence()
{
	const UGameInstance* Game = GetGameInstance();
	const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	const bool bPresent = Quests && EvaluatePresence(PresenceRole,
		Quests->FindQuestDefinition(PresenceQuestId) != nullptr,
		Quests->CanAcceptQuest(PresenceQuestId), Quests->GetQuestState(PresenceQuestId));
	SetActorHiddenInGame(!bPresent);
	SetActorEnableCollision(bPresent);
	// Interaction subsystem excludes hidden owners. Also clear markers immediately.
	QuestInteractableComponent->SetVisibility(bPresent, true);
	QuestInteractableComponent->SetComponentTickEnabled(bPresent);
	if (bPresent) RefreshQuestNoticeVisibility();
	else QuestMarkerComponent->SetVisibility(false, true);
}
