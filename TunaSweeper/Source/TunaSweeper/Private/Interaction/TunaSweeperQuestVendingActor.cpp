#include "Interaction/TunaSweeperQuestVendingActor.h"

#include "Component/TunaSweeperQuestMarkerComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperQuestVendingActor::ATunaSweeperQuestVendingActor()
{
	PrimaryActorTick.bCanEverTick = false;
	const TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerClass(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::ShopOpen, FText::GetEmpty(),
		MarkerClass, TEXT("ui.interaction.shop_open"));
	InteractableComponent->SetInteractionOrder(1);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	// VisualMesh is inherited from ShopActor and can be replaced in a Blueprint or instance.
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	VisualMesh->SetRelativeScale3D(FVector(0.85f, 0.65f, 1.8f));

	QuestInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("QuestInteractable"));
	QuestInteractableComponent->SetupAttachment(GetRootComponent());
	QuestInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	QuestInteractableComponent->SetInteractionOrder(0);
	QuestInteractableComponent->ConfigureInteractionDefaults(ETunaSweeperInteractionType::Quest,
		FText::GetEmpty(), MarkerClass, TEXT("ui.quest.interaction_name"));

	QuestMarkerComponent = CreateDefaultSubobject<UTunaSweeperQuestMarkerComponent>(TEXT("QuestMarker"));
	QuestMarkerComponent->SetupAttachment(GetRootComponent());
	QuestMarkerComponent->SetMarkerHeight(220.0f);
	QuestMarkerComponent->SetVisibility(false, true);
}

void ATunaSweeperQuestVendingActor::BeginPlay()
{
	Super::BeginPlay();
	if (UGameInstance* Game = GetGameInstance())
	{
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.AddUObject(this, &ATunaSweeperQuestVendingActor::RefreshQuestPresence);
	}
	RefreshQuestPresence();
}

void ATunaSweeperQuestVendingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* Game = GetGameInstance())
	{
		if (auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>())
			Quests->OnQuestProgressChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperQuestVendingActor::IsQuestPresent() const
{
	const UGameInstance* Game = GetGameInstance();
	const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	if (!Quests || !Quests->FindQuestDefinition(FirstMeetingQuestId) || !Quests->FindQuestDefinition(RelocationQuestId))
		return false;
	const ETunaSweeperQuestState RelocationState = Quests->GetQuestState(RelocationQuestId);
	const bool bRelocated = RelocationState == ETunaSweeperQuestState::RewardCompleted;
	if (PresenceRole == ETunaSweeperQuestVendingRole::BaseResident) return bRelocated;
	return !bRelocated &&
		(Quests->GetQuestState(FirstMeetingQuestId) != ETunaSweeperQuestState::Available ||
		Quests->CanAcceptQuest(FirstMeetingQuestId));
}

bool ATunaSweeperQuestVendingActor::IsShopAvailable() const
{
	return PresenceRole == ETunaSweeperQuestVendingRole::BaseResident && IsQuestPresent();
}

int32 ATunaSweeperQuestVendingActor::ResolveQuestId() const
{
	if (!IsQuestPresent()) return 0;
	const UGameInstance* Game = GetGameInstance();
	const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	int32 QuestId = 0;
	return Quests && Quests->TryResolveQuestForProvider(QuestProviderId, FirstMeetingQuestId, QuestId) ? QuestId : 0;
}

void ATunaSweeperQuestVendingActor::RefreshQuestPresence()
{
	const bool bPresent = IsQuestPresent();
	const bool bShopAvailable = IsShopAvailable();
	SetActorHiddenInGame(!bPresent);
	SetActorEnableCollision(bPresent);
	QuestInteractableComponent->SetVisibility(bPresent, true);
	QuestInteractableComponent->SetComponentTickEnabled(bPresent);
	InteractableComponent->SetVisibility(bShopAvailable, true);
	InteractableComponent->SetComponentTickEnabled(bShopAvailable);

	const UGameInstance* Game = GetGameInstance();
	const auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	const int32 QuestId = bPresent ? ResolveQuestId() : 0;
	const bool bShowNotice = Quests && QuestId > 0 &&
		((Quests->GetQuestState(QuestId) == ETunaSweeperQuestState::Available && Quests->CanAcceptQuest(QuestId)) ||
		Quests->GetQuestState(QuestId) == ETunaSweeperQuestState::RewardAvailable);
	QuestMarkerComponent->SetVisibility(bShowNotice, true);
}
